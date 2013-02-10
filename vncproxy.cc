#include <string>
#include <map>
#include <set>

#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <pthread.h>

#include <sqlite3.h>

#include "d3des.h"
#include "utils.h"
#include "marshal.h"
#include "polling.h"

using namespace std;
using namespace rpc;

bool global_stop_flag = false;
sqlite3 *global_db;
pthread_mutex_t global_m = PTHREAD_MUTEX_INITIALIZER;

class EndPoint: public Pollable {
    PollMgr* poll_;
    int fd_;
    EndPoint* peer_;
    string forward_key_;

    bool leader_;
    Marshal buf_;
    pthread_mutex_t m_;
    bool enabled_;

    static pthread_mutex_t all_tie_leaders_m;
    static multimap<string, EndPoint*> all_tie_leaders;

public:
    EndPoint(PollMgr* pmgr, int fd)
    : poll_(pmgr), fd_(fd), peer_(NULL), leader_(false), enabled_(false) {
        Pthread_mutex_init(&m_, NULL);
    }

    ~EndPoint() {
        Pthread_mutex_destroy(&m_);
    }

    void tie(EndPoint* o, const string& forward_key) {
        this->forward_key_ = forward_key;
        this->leader_ = true;
        this->peer_ = o;
        o->peer_ = this;

        Pthread_mutex_lock(&all_tie_leaders_m);
        all_tie_leaders.insert(make_pair(forward_key_, this));
        Pthread_mutex_unlock(&all_tie_leaders_m);
    }

    void handle_read() {
        if (!enabled_) {
            return;
        }
        Pthread_mutex_lock(&peer_->m_);
        int cnt = peer_->buf_.read_from_fd(fd_);
        if (cnt > 0) {
            poll_->update_mode(peer_, Pollable::READ | Pollable::WRITE);
        }
        Pthread_mutex_unlock(&peer_->m_);
        //Log::debug("read (fd=%d): cnt=%d", fd_, cnt);
    }

    void handle_write() {
        if (!enabled_) {
            return;
        }
        Pthread_mutex_lock(&m_);
        int cnt = buf_.write_to_fd(fd_);
        if (buf_.content_size_gt(0)) {
            poll_->update_mode(this, Pollable::READ | Pollable::WRITE);
        } else {
            poll_->update_mode(this, Pollable::READ);
        }
        Pthread_mutex_unlock(&m_);
        //Log::debug("write (fd=%d): cnt=%d", fd_, cnt);
    }

    int fd() {
        return fd_;
    }

    void shutdown() {
        close(fd_);
        poll_->remove(this);
        if (leader_) {
            peer_->shutdown();

            Pthread_mutex_lock(&all_tie_leaders_m);
            for (multimap<string, EndPoint*>::iterator it = all_tie_leaders.lower_bound(forward_key_); it != all_tie_leaders.upper_bound(forward_key_); ++it) {
                if (it->second == this) {
                    all_tie_leaders.erase(it);
                    break;
                }
            }
            Pthread_mutex_unlock(&all_tie_leaders_m);

            Log::info("shutdown: client_fd=%d, remote_fd=%d", fd_, peer_->fd_);
        }
        this->release();
        //Log::info("shutdown %d", fd_);
    }

    void handle_error() {
        if (!enabled_) {
            return;
        }
        //Log::error("error: fd=%d", fd_);
        if (leader_) {
            shutdown();
        } else {
            // peer is leader
            peer_->shutdown();
        }
    }

    int poll_mode() {
        return Pollable::READ | Pollable::WRITE;
    }

    void ready() {
        enabled_ = true;
    }

    static void cleanup(const set<string>& valid_forward_keys) {
        list<EndPoint*> outlier;
        Pthread_mutex_lock(&all_tie_leaders_m);
        for (map<string, EndPoint*>::iterator it = all_tie_leaders.begin(); it != all_tie_leaders.end(); ++it) {
            if (valid_forward_keys.find(it->first) == valid_forward_keys.end()) {
                outlier.push_back(it->second);
            }
        }
        Pthread_mutex_unlock(&all_tie_leaders_m);

        for (list<EndPoint*>::iterator it = outlier.begin(); it != outlier.end(); ++it) {
            (*it)->shutdown();
        }
    }
};
multimap<string, EndPoint*> EndPoint::all_tie_leaders;
pthread_mutex_t EndPoint::all_tie_leaders_m = PTHREAD_MUTEX_INITIALIZER;

// make sure that writes to fd1 will be read from fd2, and vice versa
// both fd1 & fd2 should be nonblocking
void tie_fd(const string& forward_key, PollMgr* poll, int fd1, int fd2) {
    EndPoint* ep1 = new EndPoint(poll, fd1);
    EndPoint* ep2 = new EndPoint(poll, fd2);

    ep1->tie(ep2, forward_key);

    poll->add(ep1);
    poll->add(ep2);

    ep1->ready();
    ep2->ready();
}

int connect_to(const char* addr) {
    int sock;
    string addr_str(addr);
    int idx = addr_str.find(":");
    if (idx == string::npos) {
        Log::error("connect_to(): bad connect address: %s", addr);
        errno = EINVAL;
        return -1;
    }
    string host = addr_str.substr(0, idx);
    string port = addr_str.substr(idx + 1);

    struct addrinfo hints, *result, *rp;
    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_family = AF_INET; // ipv4
    hints.ai_socktype = SOCK_STREAM; // tcp

    int r = getaddrinfo(host.c_str(), port.c_str(), &hints, &result);
    if (r != 0) {
        Log::error("connect_to(): getaddrinfo(): %s", gai_strerror(r));
        return -1;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock == -1) {
            continue;
        }

        const int yes = 1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

        if (::connect(sock, rp->ai_addr, rp->ai_addrlen) == 0) {
            break;
        }
        ::close(sock);
        sock = -1;
    }
    freeaddrinfo(result);

    if (rp == NULL) {
        // failed to connect
        Log::error("connect_to(): connect(): %s", strerror(errno));
        return -1;
    }

    return sock;
}

int bind_on(const char* bind_addr, struct addrinfo **result, struct addrinfo **rp) {
    int server_sock = -1;

    string addr(bind_addr);
    int idx = addr.find(":");
    if (idx == string::npos) {
        Log::error("bind_on(): bad bind address: %s", bind_addr);
        errno = EINVAL;
        return -1;
    }
    string host = addr.substr(0, idx);
    string port = addr.substr(idx + 1);

    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));

    hints.ai_family = AF_INET; // ipv4
    hints.ai_socktype = SOCK_STREAM; // tcp
    hints.ai_flags = AI_PASSIVE; // server side

    int r = getaddrinfo((host == "0.0.0.0") ? NULL : host.c_str(), port.c_str(), &hints, result);
    if (r != 0) {
        Log::error("bind_on(): getaddrinfo(): %s", gai_strerror(r));
        return -1;
    }

    for (*rp = *result; *rp != NULL; *rp = (*rp)->ai_next) {
        server_sock = socket((*rp)->ai_family, (*rp)->ai_socktype, (*rp)->ai_protocol);
        if (server_sock == -1) {
            continue;
        }

        const int yes = 1;
        setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

        if (bind(server_sock, (*rp)->ai_addr, (*rp)->ai_addrlen) == 0) {
            break;
        }
        close(server_sock);
        server_sock = -1;
    }

    if (rp == NULL) {
        // failed to bind
        Log::error("bind_on(): bind(): %s", strerror(errno));
        freeaddrinfo(*result);
        return -1;
    }

    verify(server_sock >= 0);

    // about backlog: http://www.linuxjournal.com/files/linuxjournal.com/linuxjournal/articles/023/2333/2333s2.html
    const int backlog = SOMAXCONN;
    verify(listen(server_sock, backlog) == 0);

    return server_sock;
}

struct vnc_auth_info {
    unsigned char* challenge;
    unsigned char* response;
    bool matched;
    string forward_key;
    string dest_addr;
    bool has_dest_passwd;
    string dest_passwd;

    vnc_auth_info(unsigned char* chal, unsigned char* resp)
            : challenge(chal), response(resp), matched(false), has_dest_passwd(false) {
    }
};

// locked by global_m
int vnc_auth_callback(void* cb_args, int columns, char** values, char** column_names) {
    vnc_auth_info* auth_info = (vnc_auth_info *) cb_args;

    if (auth_info->matched) {
        return 0;
    }

    unsigned char auth_key[8];
    unsigned char expected_response[16];
    memset(auth_key, 0, 8);
    memcpy(auth_key, values[0], strlen(values[0]));
    rfbDesKey(auth_key, EN0);

    for (int i = 0; i < 16; i += 8) {
        rfbDes(auth_info->challenge + i, expected_response + i);
    }

    if (memcmp(auth_info->response, expected_response, 16) == 0) {
        auth_info->matched = true;
        auth_info->forward_key = values[0];
        auth_info->dest_addr = values[1];
        if (values[2] != NULL) {
            auth_info->has_dest_passwd = true;
            auth_info->dest_passwd = values[2];
        }
    }

    return 0;
}

class VncOperator: public Runnable {
    PollMgr* poll_;
    int clnt_;
public:
    VncOperator(PollMgr* pmgr, int clnt)
            : poll_(pmgr), clnt_(clnt) {
    }

    void run() {
        // ensure clnt socket is in blocking mode, so we can easily read/write full messages
        verify(set_nonblocking(clnt_, false) == 0);

        // vnc hand shake, only support protocol version 3.8
        if (send(clnt_, "RFB 003.008\n", 12, MSG_WAITALL) < 0) {
            Log::error("error communicating with client");
            close(clnt_);
            return;
        }

        char buf[16];
        if (recv(clnt_, buf, 12, MSG_WAITALL) < 0) {
            Log::error("error communicating with client");
            close(clnt_);
            return;
        }

        buf[12] = '\0';
        //Log::debug("client protocol: %s", buf);
        if (strcmp(buf, "RFB 003.008\n") != 0) {
            Log::info("client protocol not supported: %s", buf);
            close(clnt_);
            return;
        }

        // request passwd from client, which is used as redirect hint
        buf[0] = 1; // 1 security type available
        buf[1] = 2; // use VNC auth
        if (send(clnt_, buf, 2, MSG_WAITALL) < 0) {
            Log::error("error communicating with client");
            close(clnt_);
            return;
        }

        if (recv(clnt_, buf, 1, MSG_WAITALL) < 0) {
            Log::error("error communicating with client");
            close(clnt_);
            return;
        }

        //Log::info("client security type: 0x%x", buf[0]);

        // challenge client for passwd
        unsigned char challenge[16], response[16];
        for (int i = 0; i < sizeof(challenge); i++) {
            challenge[i] = rand() & 0xFF;
        }
        if (send(clnt_, challenge, sizeof(challenge), MSG_WAITALL) < 0) {
            Log::error("error communicating with client");
            close(clnt_);
            return;
        }
        if (recv(clnt_, response, sizeof(response), MSG_WAITALL) < 0) {
            Log::error("error communicating with client");
            close(clnt_);
            return;
        }

        Pthread_mutex_lock(&global_m);

        vnc_auth_info auth_info(challenge, response);
        char* errmsg = NULL;
        int r = sqlite3_exec(global_db, "select forward_key, dest_addr, dest_passwd from vncproxy", vnc_auth_callback, &auth_info, &errmsg);

        Pthread_mutex_unlock(&global_m);

        if (r != SQLITE_OK) {
            Log::error("encountered sqlite error: %s", errmsg);
            sqlite3_free(errmsg);
            close(clnt_);
            return;
        }

        if (!auth_info.matched) {
            // tell client auth failed
            Log::info("client authentication failed");
            int32_t fail = 1;
            memcpy(buf, &fail, sizeof(fail));
            send(clnt_, buf, sizeof(fail), MSG_WAITALL);
            close(clnt_);
            return;
        }
        // no need to reply 'pass', leave this to remote side

        Log::info("forward client_fd=%d to: %s", clnt_, auth_info.dest_addr.c_str());

        // now connect to remote vnc server
        int remote_fd = connect_to(auth_info.dest_addr.c_str());
        if (remote_fd < 0) {
            Log::error("error communicating with remote server");
            close(clnt_);
            return;
        }

        // recv "RFB 003.008\n"
        if (recv(remote_fd, buf, 12, MSG_WAITALL) < 0 || memcmp(buf, "RFB 003.008\n", 12) != 0) {
            Log::error("error communicating with remote server");
            close(remote_fd);
            close(clnt_);
            return;
        }

        // tell server to use protocol version 3.8
        if (send(remote_fd, "RFB 003.008\n", 12, MSG_WAITALL) < 0) {
            Log::error("error communicating with remote server");
            close(remote_fd);
            close(clnt_);
            return;
        }

        // recv server auth type
        if (recv(remote_fd, buf, 1, MSG_WAITALL) < 0) {
            Log::error("error communicating with remote server");
            close(remote_fd);
            close(clnt_);
            return;
        }

        int auth_types = buf[0];
        if (recv(remote_fd, buf, auth_types, MSG_WAITALL) < 0) {
            Log::error("error communicating with remote server");
            close(remote_fd);
            close(clnt_);
            return;
        }

        bool support_none_auth = false;
        bool support_vnc_auth = false;
        for (int i = 0; i < auth_types; i++) {
            if (buf[i] == 1) {
                support_none_auth = true;
            }
            if (buf[i] == 2) {
                support_vnc_auth = true;
            }
        }

        if (support_none_auth) {
            if (send(remote_fd, "\1", 1, MSG_WAITALL) < 0) {
                Log::error("error communicating with remote server");
                close(remote_fd);
                close(clnt_);
                return;
            }
        } else if (support_vnc_auth && auth_info.has_dest_passwd) {
            if (send(remote_fd, "\2", 1, MSG_WAITALL) < 0) {
                Log::error("error communicating with remote server");
                close(remote_fd);
                close(clnt_);
                return;
            }

            // get challenge
            if (recv(remote_fd, challenge, sizeof(challenge), MSG_WAITALL) < 0) {
                Log::error("error communicating with remote server");
                close(remote_fd);
                close(clnt_);
                return;
            }

            // calculate response, use global_m to protect rfbDes internal variables
            Pthread_mutex_lock(&global_m);

            unsigned char auth_key[8];
            memset(auth_key, 0, 8);
            memcpy(auth_key, &(auth_info.dest_passwd[0]), auth_info.dest_passwd.length());
            rfbDesKey(auth_key, EN0);

            for (int i = 0; i < 16; i += 8) {
                rfbDes(challenge + i, response + i);
            }

            Pthread_mutex_unlock(&global_m);

            // send response
            if (send(remote_fd, response, sizeof(response), MSG_WAITALL) < 0) {
                Log::error("error communicating with remote server");
                close(remote_fd);
                close(clnt_);
                return;
            }

        } else {
            // auth type not supported
            Log::error("remote server authentication methods not supported");
            close(remote_fd);
            close(clnt_);
            return;
        }

        Log::info("vnc forwarding established between client_fd=%d, remote_fd=%d", clnt_, remote_fd);

        // tie the fd up, need nonblocking mode
        verify(set_nonblocking(clnt_, true) == 0);
        verify(set_nonblocking(remote_fd, true) == 0);
        tie_fd(auth_info.forward_key, poll_, clnt_, remote_fd);
    }
};

void do_stop(int sig) {
    global_stop_flag = true;
    Log::info("got signal %d, will stop", sig);
}

int collect_forward_key_callback(void* cb_args, int columns, char** values, char** column_names) {
    set<string>* valid_forward_keys = (set<string>*) cb_args;
    valid_forward_keys->insert(values[0]);
    return 0;
}

void* cleanup_thread(void *) {
    while (!global_stop_flag) {
        set<string> valid_forward_keys;
        char* errmsg = NULL;

        Pthread_mutex_lock(&global_m);
        int r = sqlite3_exec(global_db, "select forward_key from vncproxy", collect_forward_key_callback, &valid_forward_keys, &errmsg);
        Pthread_mutex_unlock(&global_m);

        if (r != SQLITE_OK) {
            Log::error("encountered sqlite error: %s", errmsg);
            sqlite3_free(errmsg);
        } else {
            EndPoint::cleanup(valid_forward_keys);
        }

        sleep(1);
    }
    pthread_exit(NULL);
    return NULL;
}

void print_help(char* argv[]) {
    printf("usage: %s <host:port> [proxy-db='vncproxy.sqlite3']\n", argv[0]);
    printf("\n");
    printf("the proxy-db should have following schema:\n");
    printf("vncproxy(forward_key varchar(8) primary key, dest_addr text not null, dest_passwd varchar(8))\n");
}

int main(int argc, char* argv[]) {

    // check for -h and --help
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_help(argv);
            exit(0);
        }
    }

    if (argc < 2) {
        print_help(argv);
        exit(1);
    }

    srand(getpid());

    signal(SIGPIPE, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

    signal(SIGINT, do_stop);
    signal(SIGQUIT, do_stop);

    const char* bind_addr = argv[1];
    Log::info("bind address: %s", bind_addr);
    char* db_fn = "vncproxy.sqlite3";
    if (argc >= 3) {
        db_fn = argv[2];
    }
    Log::info("proxy db file: %s", db_fn);

    int r = sqlite3_open(db_fn, &global_db);
    if (r != 0) {
        Log::fatal("cannot open db file '%s': %s", db_fn, sqlite3_errmsg(global_db));
        sqlite3_close(global_db);
        exit(1);
    }

    verify(sqlite3_exec(global_db, "create table if not exists vncproxy(forward_key varchar(8) primary key, dest_addr text not null, dest_passwd varchar(8))", NULL, NULL, NULL) == 0);

    struct addrinfo *result, *rp;
    int server_sock = bind_on(bind_addr, &result, &rp);
    if (server_sock < 0) {
        exit(1);
    }
    verify(set_nonblocking(server_sock, true) == 0);

    PollMgr* poll = new PollMgr;
    ThreadPool* thpool = new ThreadPool;

    pthread_t cleanup_th;
    Pthread_create(&cleanup_th, NULL, cleanup_thread, NULL);

    fd_set fds;
    while (!global_stop_flag) {
        FD_ZERO(&fds);
        FD_SET(server_sock, &fds);

        timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 50 * 1000; // 0.05 sec
        int fdmax = server_sock;

        int n_ready = select(fdmax + 1, &fds, NULL, NULL, &tv);
        if (n_ready == 0) {
            continue;
        }
        if (global_stop_flag) {
            break;
        }

        int clnt_socket = accept(server_sock, rp->ai_addr, &rp->ai_addrlen);
        if (clnt_socket >= 0) {
            Log::info("got new client connection, fd: %d", clnt_socket);
            thpool->run_async(new VncOperator(poll, clnt_socket));
        }
    }

    Log::info("doing final cleanup");
    Pthread_join(cleanup_th, NULL);

    delete thpool;
    poll->release();
    freeaddrinfo(result);
    sqlite3_close(global_db);
    Log::info("cleanup finished, quit now");

    return 0;
}

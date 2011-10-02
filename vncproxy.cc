#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>
#include <map>

#include <ctype.h>
#include <pthread.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>

#include "config.h"

#include "d3des.h"
#include "sockutils.h"
#include "zbuf.h"

using namespace std;

static const char* g_sock_fpath = "/tmp/vncproxy.sock";

static pthread_mutex_t mapping_mtx = PTHREAD_MUTEX_INITIALIZER;
static map<string, string> g_mapping;

static pthread_t vnc_server_tid;
static bool vnc_stop_flag = false;

static inline bool str_eq(const char* str1, const char* str2) {
    return strcmp(str1, str2) == 0;
}

static bool str_starts_with(const char* str, const char* head) {
    int i;
    for (i = 0; str[i] != '\0' && head[i] != '\0'; i++) {
        if (str[i] != head[i]) {
            return false;
        }
    }
    return head[i] == '\0';
}

static void die(int ret_code) {
    unlink(g_sock_fpath);
    exit(ret_code);
}

static void print_version() {
    printf("vncproxy version %s\n", VERSION);
}


static void print_help() {
    printf("usage: vncproxy start HOST:PORT\n");
    printf("           Start vncproxy service, will serve on HOST:PORT\n");
    printf("\n");
    printf("       vncproxy stop\n");
    printf("           Stop currently running vncproxy service\n");
    printf("\n");
    printf("       vncproxy restart HOST:PORT\n");
    printf("           Restart currently running vncproxy, will serve on HOST:PORT\n");
    printf("\n");
    printf("       vncproxy list\n");
    printf("           List all current mappings\n");
    printf("\n");
    printf("       vncproxy add NAME HOST:PORT\n");
    printf("           Add a mapping named NAME to HOST:PORT\n");
    printf("\n");
    printf("       vncproxy remove NAME\n");
    printf("           Remove a mapping named NAME\n");
    printf("\n");
}


static inline int send_message(int sock, const char* msg) {
    int write_cnt = 0;
    int msg_len = strlen(msg);
    while (write_cnt < msg_len) {
        int cnt = write(sock, msg + write_cnt, msg_len - write_cnt);
        if (cnt < 0) {
            return -1;
        }
        write_cnt += cnt;
    }
    return 0;
}

static void socket_forwarding(int sock1, int sock2) {
    unsigned char buf[8192];
    
    zbuf buf12 = zbuf_new();
    zbuf buf21 = zbuf_new();
    
    fd_set master_fds;
    FD_ZERO(&master_fds);
    FD_SET(sock1, &master_fds);
    FD_SET(sock2, &master_fds);
    
    int fdmax = sock1;
    if (sock2 > fdmax) {
        fdmax = sock2;
    }
    
    for (;;) {
        fd_set error_fds = master_fds;
        fd_set read_fds = master_fds;
        fd_set write_fds;
        
        FD_ZERO(&write_fds);
        if (zbuf_size(buf12) > 0) {
            FD_SET(sock2, &write_fds);
        }
        if (zbuf_size(buf21) > 0) {
            FD_SET(sock1, &write_fds);
        }
        
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 1000 * 10;
        
        int ret = select(fdmax + 1, &read_fds, &write_fds, &error_fds, &tv);
        if (ret < 0) {
            printf("ERROR: socket forwarding error!\n");
            break;
        } else if (ret == 0) {
            // do nothing
        } else {
            if (FD_ISSET(sock1, &error_fds) || FD_ISSET(sock2, &error_fds)) {
                printf("ERROR: error on forwarding socket!\n");
                exit(1);
            }
            if (FD_ISSET(sock1, &read_fds)) {
                int cnt = read(sock1, buf, sizeof(buf));
                if (cnt < 0) {
                    printf("ERROR: network IO failed!\n");
                    exit(1);
                } else if (cnt == 0) {
                    break;
                }
                zbuf_append(buf12, buf, cnt);
            }
            if (FD_ISSET(sock2, &read_fds)) {
                int cnt = read(sock2, buf, sizeof(buf));
                if (cnt < 0) {
                    printf("ERROR: network IO failed!\n");
                    exit(1);
                } else if (cnt == 0) {
                    break;
                }
                zbuf_append(buf21, buf, cnt);
            }
            if (FD_ISSET(sock1, &write_fds)) {
                int peek_cnt = zbuf_peek(buf21, buf, sizeof(buf));
                if (peek_cnt > 0) {
                    int write_cnt = write(sock1, buf, peek_cnt);
                    if (write_cnt < 0) {
                        printf("ERROR: network IO failed!\n");
                        exit(1);
                    } else if (write_cnt == 0) {
                        break;
                    }
                    zbuf_discard(buf21, write_cnt);
                }
            }
            if (FD_ISSET(sock2, &write_fds)) {
                int peek_cnt = zbuf_peek(buf12, buf, sizeof(buf));
                if (peek_cnt > 0) {
                    int write_cnt = write(sock2, buf, peek_cnt);
                    if (write_cnt < 0) {
                        printf("ERROR: network IO failed!\n");
                        exit(1);
                    } else if (write_cnt == 0) {
                        break;
                    }
                    zbuf_discard(buf12, write_cnt);                    
                }
            }
        }
    }
    
    zbuf_delete(buf12);
    zbuf_delete(buf21);
}

static void handle_vnc_client(int client_sock) {
    pid_t pid = fork();
    if (pid < 0) {
        printf("ERROR: failed to fork() new process!\n");
        die(1);
    } else if (pid > 0) {
        // close socket in parent process
        close(client_sock);
        return;
    }
    
    // begin VNC forwarding service
    // do not use die() from now on, since we are working in forked child process,
    // not the master process which holds the control socket
    
    // handshake on VNC version, only support 3.8
    if (send_message(client_sock, "RFB 003.008\n") < 0) {
        printf("ERROR: network IO failed!\n");
        exit(1);
    }
    
    char buf[8192];
    if (recv(client_sock, buf, 12, MSG_WAITALL) < 0) {
        printf("ERROR: network IO failed!\n");
        exit(1);
    }
    
    buf[12] = '\0';
    printf("INFO: client using protocol: %s", buf);
    if (!str_eq(buf, "RFB 003.008\n")) {
        printf("ERROR: client protocol not supported!\n");
        exit(1);
    }
    
    // request password from client, which is actually the mapping name
    buf[0] = 1; // 1 security type
    buf[1] = 2; // use VNC auth
    if (send(client_sock, buf, 2, MSG_WAITALL) < 0) {
        printf("ERROR: network IO failed!\n");
        exit(1);
    }
    
    // get reply from client
    if (recv(client_sock, buf, 1, MSG_WAITALL) < 0) {
        printf("ERROR: network IO failed!\n");
        exit(1);
    }
    
    printf("INFO: client security type: 0x%x\n", (int) buf[0]);
    
    srand(time(NULL) + getpid());
    
    // challenge the client for password
    unsigned char challenge[16], response[16];
    for (int i = 0; i < sizeof(challenge); i++) {
        challenge[i] = rand() & 0xFF;
    }
    if (send(client_sock, challenge, sizeof(challenge), MSG_WAITALL) < 0) {
        printf("ERROR: network IO failed!\n");
        exit(1);
    }
    if (recv(client_sock, response, sizeof(response), MSG_WAITALL) < 0) {
        printf("ERROR: network IO failed!\n");
        exit(1);
    }
    
    bool auth_ok = false;
    string dest;
    
    // TODO auth, and forwarding
    pthread_mutex_lock(&mapping_mtx);
    for (map<string, string>::iterator it = g_mapping.begin(); it != g_mapping.end(); ++it) {
        string name = it->first;
        unsigned char expected_response[16];
        unsigned char auth_key[8];
        int i;
        for (i = 0; i < 8 && i < name.length(); i++) {
            auth_key[i] = name[i];
        }
        while (i < 8) {
            auth_key[i] = '\0';
            i++;
        }
        rfbDesKey(auth_key, EN0);
        
        for (i = 0; i < 16; i += 8) {
          rfbDes(challenge + i, expected_response + i);
        }
        
        if (memcmp(response, expected_response, 16) == 0) {
            printf("INFO: client mapped to '%s'\n", dest.c_str());
            auth_ok = true;
            dest = it->second;
            break;
        }
    }
    pthread_mutex_unlock(&mapping_mtx);
    
    if (auth_ok) {
        printf("INFO: connecting to '%s'\n", dest.c_str());
        char* host = strdup(dest.c_str());
        int port = -1;
        int idx;
        for (idx = 0; host[idx] != '\0'; idx++) {
            if (host[idx] == ':') {
                port = atoi(host + idx + 1);
                host[idx] = '\0';
                break;
            }
        }
        int vnc_sock = tcpclntsock(host, port);
        
        if (recv(vnc_sock, buf, 12, MSG_WAITALL) < 0) {
            printf("ERROR: network IO failed!\n");
            exit(1);
        }
        
        if (send_message(vnc_sock, "RFB 003.008\n") < 0) {
            printf("ERROR: network IO failed!\n");
            exit(1);
        }
        
        if (recv(vnc_sock, buf, 1, MSG_WAITALL) < 0) {
            printf("ERROR: network IO failed!\n");
            exit(1);
        }
        
        int auth_type_count = buf[0];
        bool support_none_auth = false;
        printf("INFO: real VNC server has %d types of authentication\n", auth_type_count);
        if (recv(vnc_sock, buf, auth_type_count, MSG_WAITALL) < 0) {
            printf("ERROR: network IO failed!\n");
            exit(1);
        }
        
        for (int i = 0; i < auth_type_count; i++) {
            if (buf[i] == 1) {
                support_none_auth = true;
                break;
            }
        }
        
        if (support_none_auth) {
            printf("INFO: real VNC server supports 'none' authentication\n");
        } else {
            printf("ERROR: real VNC server do NOT supports 'none' authentication!\n");
            exit(1);
        }
        
        // tell real VNC server to use none auth
        if (send_message(vnc_sock, "\1") < 0) {
            printf("ERROR: network IO failed!\n");
            exit(1);
        }
        
        // now start forwarding
        printf("INFO: start VNC forwarding\n");
        
        socket_forwarding(vnc_sock, client_sock);
        
    } else {
        const char* failure_msg = "client authentication failed!";
        printf("ERROR: %s\n", failure_msg);

        // well, be careful about the endians
        buf[0] = 0;
        buf[1] = 0;
        buf[2] = 0;
        buf[3] = 1; // indicate failure
        buf[4] = 0;
        buf[5] = 0;
        buf[6] = 0;
        buf[7] = strlen(failure_msg);
        strcpy((char *) (buf + 8), failure_msg);
        
        if (send(client_sock, buf, strlen(failure_msg) + 8, MSG_WAITALL) < 0) {
            printf("ERROR: network IO failed!\n");
            exit(1);
        }
    }
    
    // quit forked VNC forwarding service
    exit(0);
}

static void* vnc_server(void* arg) {
    signal(SIGCHLD, SIG_IGN);
    
    int svrsock = *(int *) arg;
    
    fd_set fdset;
    int fdmax = svrsock;  // so far, it's this one
    
    FD_ZERO(&fdset);
    FD_SET(svrsock, &fdset);
    
    while (vnc_stop_flag == false) {
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 10 * 1000;  // 10ms timeout
        fd_set accept_fdset = fdset;
        int ret = select(fdmax + 1, &accept_fdset, NULL, NULL, &tv);
        
        if (ret < 0) {
            if (errno == EINTR) {
                // interrupted syscall, no problem
                continue;
            } else {
                printf("ERROR: failed in select() call, %s!\n", strerror(errno));
                break;
            }
        } else if (ret == 0) {
            // timeout, do nothing
        } else {
            int fd;
            for (fd = 0; fd <= fdmax; fd++) {
                if (FD_ISSET(fd, &accept_fdset) && fd == svrsock) {
                    struct sockaddr_in clnt;
                    socklen_t in_len = sizeof(clnt);
                    int client_sock = accept(svrsock, (struct sockaddr *) &clnt, &in_len);
                    if (client_sock < 0) {
                        printf("ERROR: failed to accept client request, error: %s!\n", strerror(errno));
                    } else {
                        printf("INFO: got client, file descriptor = %d\n", client_sock);
                        handle_vnc_client(client_sock);
                    }
                }
            }
        }
    }
    
    close(svrsock);
}


static inline int start_vnc_server(const char* host_port) {
    char* host = strdup(host_port);
    int port = -1;
    int idx;
    for (idx = 0; host[idx] != '\0'; idx++) {
        if (host[idx] == ':') {
            port = atoi(host + idx + 1);
            host[idx] = '\0';
            break;
        }
    }
    if (port < 0) {
        printf("ERROR: bad HOST:PORT parameter!\n");
        return EINVAL;
    }
    printf("INFO: starting vncproxy server, host='%s', port=%d\n", host, port);
    
    int svrsock = tcpsvrsock(host, port);
    if (svrsock < 0) {
        return EACCES;
    } else {
        return pthread_create(&vnc_server_tid, NULL, vnc_server, &svrsock);
    }
}


static inline void stop_vnc_server() {
    vnc_stop_flag = true;
    pthread_join(vnc_server_tid, NULL);
}


static inline bool server_is_alive() {
    int sock = unixclntsock(g_sock_fpath);
    if (sock < 0) {
        return false;
    } else {
        close(sock);
        return true;
    }
}

static int start_server(const char* host_port) {
    if (server_is_alive()) {
        printf("%s", "ERROR: vncproxy server already running!\n");
        return EBUSY;
    }
    
    if (start_vnc_server(host_port) != 0) {
        return EACCES;
    }
    
    // start control server
    int ctl_sock = unixsvrsock(g_sock_fpath);
    
    struct sockaddr_un clnt;
    socklen_t un_len = sizeof(clnt);
    char req[8192], buf[8192];
    
    // accept control requests, run service until 'stop' request is received
    for (;;) {
        int sock = accept(ctl_sock, (struct sockaddr *) &clnt, &un_len);
        
        // handle control requests here
        int total_cnt = 0;
        for (;;) {
            int cnt = read(sock, req + total_cnt, sizeof(req) - total_cnt - 1);
            if (cnt <= 0) {
                break;
            }
            total_cnt += cnt;
        }
        req[total_cnt] = '\0';
        
        if (str_starts_with(req, "list\n")) {
            printf("%s", "INFO: got 'list' request\n");
            
            pthread_mutex_lock(&mapping_mtx);
            for (map<string, string>::iterator it = g_mapping.begin(); it != g_mapping.end(); ++it) {
                sprintf(buf, "%s --> %s\n", it->first.c_str(), it->second.c_str());
                send_message(sock, buf);
            }
            pthread_mutex_unlock(&mapping_mtx);
        
        } else if (str_starts_with(req, "add\n")) {
            
            string name, dest;
            int idx = strlen("add\n");
            while (req[idx] != '\n' && req[idx] != '\0') {
                name += req[idx];
                idx++;
            }
            idx++;
            while (req[idx] != '\n' && req[idx] != '\0') {
                dest += req[idx];
                idx++;
            }
            
            // TODO check if valid name
            bool valid_name = true;
            if (name.length() > 8 || name.length() == 0) {
                valid_name = false; // maximum 8 bytes name length
            }
            for (int i = 0; i < name.length(); i++) {
                if (isgraph(name[i])) {
                    
                }
            }
            
            printf("INFO: got 'add' request, name='%s', dest='%s'\n", name.c_str(), dest.c_str());
            if (!valid_name) {
                printf("ERROR: invalid mapping name! Only letters, numeric digits and punctuation characters are allowed. The mapping name must not be empty and contains at most 8 characters.\n");
                send_message(sock, "ERROR: invalid mapping name! Only letters, numeric digits and punctuation characters are allowed. The mapping name must not be empty and contains at most 8 characters.\n");
                
            } else {
                pthread_mutex_lock(&mapping_mtx);
                if (g_mapping.find(name) != g_mapping.end()) {
                    send_message(sock, "ERROR: mapping already exists!\n");
                } else {
                    g_mapping[name] = dest;
                    send_message(sock, "INFO: mapping added\n");
                }
                pthread_mutex_unlock(&mapping_mtx);
            }

        } else if (str_starts_with(req, "remove\n")) {
            
            string name;
            int idx = strlen("remove\n");
            while (req[idx] != '\n' && req[idx] != '\0') {
                name += req[idx];
                idx++;
            }
            
            printf("INFO: got 'remove' request, name='%s'\n", name.c_str());

            pthread_mutex_lock(&mapping_mtx);
            map<string, string>::iterator it = g_mapping.find(name);
            if (it != g_mapping.end()) {
                g_mapping.erase(it);
                send_message(sock, "INFO: mapping removed\n");
            } else {
                send_message(sock, "ERROR: mapping not found!\n");
            }
            pthread_mutex_unlock(&mapping_mtx);
            
        } else if (str_starts_with(req, "stop\n")) {
            printf("%s", "INFO: got 'stop' request\n");
            send_message(sock, "INFO: vncproxy is now shutting down...\n");
            break;
            
        } else {
            printf("WARN: request not understood:\n----\n%s----\n", req);
        }
        
        close(sock);
    }
    
    stop_vnc_server();
    
    unlink(g_sock_fpath);
    return 0;
}

static int connect_to_server() {
    int sock = unixclntsock(g_sock_fpath);
    if (sock < 0) {
        printf("%s", "ERROR: failed to connect to vncproxy server! Please check if vncproxy is running.\n");
    }
    return sock;
}

static void echo_reply(int sock) {
    char buf[8192];
    for (;;) {
        int cnt = recv(sock, buf, sizeof(buf) - 1, MSG_WAITALL);
        if (cnt > 0) {
            printf("%s", buf);
        } else {
            break;
        }
    }
}


static int stop_server() {
    int sock = connect_to_server();
    if (sock < 0) {
        return ENOTCONN;
    }
    
    send_message(sock, "stop\n");
    shutdown(sock, SHUT_WR);
    
    echo_reply(sock);
    close(sock);
    return 0;
}


static int list_mapping() {
    int sock = connect_to_server();
    if (sock < 0) {
        return ENOTCONN;
    }
    
    send_message(sock, "list\n");
    shutdown(sock, SHUT_WR);
    
    echo_reply(sock);
    close(sock);
    return 0;
}


static int add_mapping(const char* name, const char* dest) {
    int sock = connect_to_server();
    if (sock < 0) {
        return ENOTCONN;
    }
    
    send_message(sock, "add\n");
    send_message(sock, name);
    send_message(sock, "\n");
    send_message(sock, dest);
    send_message(sock, "\n");
    shutdown(sock, SHUT_WR);
    
    echo_reply(sock);
    close(sock);
    return 0;
}



static int remove_mapping(const char* name) {
    int sock = connect_to_server();
    if (sock < 0) {
        return ENOTCONN;
    }
    
    send_message(sock, "remove\n");
    send_message(sock, name);
    send_message(sock, "\n");
    shutdown(sock, SHUT_WR);
    
    echo_reply(sock);
    close(sock);
    return 0;
}



int main(int argc, char* argv[]) {
    int ret = 0;
    if (argc <= 1) {
        print_help();
        exit(1);
    }
    
    for (int i = 0; i < argc; i++) {
        if (str_eq(argv[i], "-h") || str_eq(argv[i], "--help") || str_eq(argv[i], "help")) {
            print_help();
            exit(0);
            
        } else if (str_eq(argv[i], "--version")) {
            print_version();
            exit(0);
        }
    }
    
    char* cmd = argv[1];
    if (str_eq(cmd, "start")) {
        if (argc < 3) {
            print_help();
            exit(1);
        }
        
        const char* host_port = argv[2];
        ret = start_server(host_port);
        
    } else if (str_eq(cmd, "stop")) {
        stop_server();
        
    } else if (str_eq(cmd, "restart")) {
        if (argc < 3) {
            print_help();
            exit(1);
        }
        
        const char* host_port = argv[2];
        stop_server();
        ret = start_server(host_port);
        
    } else if (str_eq(cmd, "list")) {
        list_mapping();
        
    } else if (str_eq(cmd, "add")) {
        if (argc < 4) {
            print_help();
            exit(1);
        }
        
        const char* name = argv[2];
        const char* dest = argv[3];
        add_mapping(name, dest);
        
    } else if (str_eq(cmd, "remove")) {
        if (argc < 3) {
            print_help();
            exit(1);
        }
        
        const char* name = argv[2];
        remove_mapping(name);
        
    } else {
        printf("vncproxy: '%s' is not a command. See 'vncproxy --help'.\n", cmd);
        
    }
    
    return ret;
}

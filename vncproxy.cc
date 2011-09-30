#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <string>
#include <map>

#include <ctype.h>
#include <pthread.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "config.h"

#include "d3des.h"
#include "sockutils.h"

using namespace std;

static const char* g_sock_fpath = "/tmp/vncproxy.sock";

static pthread_mutex_t mtx_mapping = PTHREAD_MUTEX_INITIALIZER;
static map<string, string> g_mapping;


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
    printf("       vncproxy restart [-b HOST] [-p PORT]\n");
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

static bool server_is_alive() {
    int sock = unixclntsock(g_sock_fpath);
    if (sock < 0) {
        return false;
    } else {
        close(sock);
        return true;
    }
}


static inline void send_message(int sock, const char* msg) {
    write(sock, msg, strlen(msg));
}

static int start_server() {
    if (server_is_alive()) {
        printf("%s", "ERROR: vncproxy server already running!\n");
        return EBUSY;
    }
    
    // TODO start VNC redirecting service
    
    
    // start control server
    int ctl_sock = unixsvrsock(g_sock_fpath);
    
    struct sockaddr_in clnt;
    socklen_t in_len = sizeof(clnt);
    char req[8192], buf[8192];
    
    // accept control requests, run service until 'stop' request is received
    for (;;) {
        int sock = accept(ctl_sock, (struct sockaddr *) &clnt, &in_len);
        
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
            
            pthread_mutex_lock(&mtx_mapping);
            for (map<string, string>::iterator it = g_mapping.begin(); it != g_mapping.end(); ++it) {
                sprintf(buf, "%s --> %s\n", it->first.c_str(), it->second.c_str());
                send_message(sock, buf);
            }
            pthread_mutex_unlock(&mtx_mapping);
        
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
                pthread_mutex_lock(&mtx_mapping);
                if (g_mapping.find(name) != g_mapping.end()) {
                    send_message(sock, "ERROR: mapping already exists!\n");
                } else {
                    g_mapping[name] = dest;
                    send_message(sock, "INFO: mapping added\n");
                }
                pthread_mutex_unlock(&mtx_mapping);
            }

        } else if (str_starts_with(req, "remove\n")) {
            
            string name;
            int idx = strlen("remove\n");
            while (req[idx] != '\n' && req[idx] != '\0') {
                name += req[idx];
                idx++;
            }
            
            printf("INFO: got 'remove' request, name='%s'\n", name.c_str());

            pthread_mutex_lock(&mtx_mapping);
            map<string, string>::iterator it = g_mapping.find(name);
            if (it != g_mapping.end()) {
                g_mapping.erase(it);
                send_message(sock, "INFO: mapping removed\n");
            } else {
                send_message(sock, "ERROR: mapping not found!\n");
            }
            pthread_mutex_unlock(&mtx_mapping);
            
        } else if (str_starts_with(req, "stop\n")) {
            printf("%s", "INFO: got 'stop' request\n");
            send_message(sock, "INFO: vncproxy is now shutting down...\n");
            break;
            
        } else {
            printf("WARN: request not understood:\n----\n%s----\n", req);
        }
        
        close(sock);
    }
    
    // TODO stop VNC redirecting service
    
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
        start_server();
        
    } else if (str_eq(cmd, "stop")) {
        stop_server();
        
    } else if (str_eq(cmd, "restart")) {
        stop_server();
        start_server();
        
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
    
    return 0;
}

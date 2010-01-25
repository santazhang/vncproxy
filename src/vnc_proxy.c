#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/un.h>
#include <pthread.h>
#include <errno.h>

#include "xstr.h"
#include "xnet.h"
#include "xvec.h"
#include "xmemory.h"
#include "xcrypto.h"
#include "xutils.h"
#include "xlog.h"

#include "vnc_auth/d3des.h"

typedef struct {
  xstr dest_host;
  int dest_port;
  xstr old_passwd;
  xstr new_passwd;
} vnc_map;

// globally shared data
xvec vnc_mapping;
pthread_mutex_t vnc_mapping_mutex = PTHREAD_MUTEX_INITIALIZER;

void vnc_mapping_free(void* ptr) {
  vnc_map* map = (vnc_map *) ptr;

  xstr_delete(map->dest_host);
  xstr_delete(map->old_passwd);
  xstr_delete(map->new_passwd);
  xfree(map);
}

// helper function for debugging
void print_bytes(char *buf, int size) {
  int i;
  printf("Binary: ");
  for (i = 0; i < size; i++) {
    printf("%02X ", ((int) buf[i]) & 0xFF);
  }
  printf("\n");
  printf("ASCII:  ");
  for (i = 0; i < size; i++) {
    printf(" %c ", buf[i]);
  }
  printf("\n");
}

static void vnc_proxy_acceptor(xsocket client_xs, void* args) {
  const int buf_len = 8192;
  const int challenge_size = 16;
  unsigned char* buf = xmalloc_ty(buf_len, unsigned char);
  unsigned char* challenge = xmalloc_ty(challenge_size, unsigned char);
  unsigned char* response = xmalloc_ty(challenge_size, unsigned char);
  unsigned char* expected_response = xmalloc_ty(challenge_size, unsigned char);
  unsigned char auth_key[8];
  unsigned char real_server_auth_key[8];
  int i;
  int cnt;
  xbool has_error = XFALSE;
  xbool proxy_auth_passed = XFALSE;
  xstr vnc_host = xstr_new();
  int vnc_port = 5901;

  xlog_info("got new client!\n");

  // handshake on VNC version, only support 3.8
  if (has_error == XFALSE && xsocket_write(client_xs, "RFB 003.008\n", 12) == -1) {
    xlog_error("cannot write to client, errno = %d : %s\n", errno, strerror(errno));
    has_error = XTRUE;
  }

  if (has_error == XFALSE && xsocket_read(client_xs, buf, 12) != 12) {
    xlog_error("cannot read from client\n");
    has_error = XTRUE;
  }

  buf[12] = '\0';
  xlog_info("client version info: %s\n", buf);

  if (has_error == XFALSE && xcstr_startwith_cstr((char *) buf, "RFB 003.008") == XFALSE) {
    xlog_error("client vnc version not supported!\n");
    has_error = XTRUE;
  }

  if (has_error == XFALSE) {
    // tell client the security types
    buf[0] = 1; // 1 security type
    buf[1] = 2; // vnc auth
    xsocket_write(client_xs, buf, 2);
  }

  if (has_error == XFALSE && xsocket_read(client_xs, buf, 1) == -1) {
    xlog_error("cannot read from client\n");
    has_error = XTRUE;
  }

  xlog_info("client choose security type %d\n", (int) buf[0]);

  // make random challenge
  for (i = 0; i < challenge_size; i++) {
    challenge[i] = (unsigned char) (rand() & 0xFF);
  }
  xsocket_write(client_xs, challenge, challenge_size);
  xsocket_read(client_xs, response, challenge_size);

  pthread_mutex_lock(&vnc_mapping_mutex);
  for (i = 0; i < xvec_size(vnc_mapping); i++) {
    int j;
    vnc_map* mapping = xvec_get(vnc_mapping, i);
    const char* pwd = xstr_get_cstr(mapping->new_passwd);
    if (xstr_len(mapping->new_passwd) == 0) {
      // ignore proxies with empty new passwd
      continue;
    }
    for (j = 0; j < 8 && j < xstr_len(mapping->new_passwd); j++) {
      auth_key[j] = (unsigned char) pwd[j];
    }
    while (j < 8) {
      auth_key[j] = '\0';
      j++;
    }
    rfbDesKey(auth_key, EN0);

    for (j = 0; j < challenge_size; j += 8) {
      rfbDes(challenge + j, expected_response + j);
    }

    printf("comparing for auth:\n");
    printf("[info] client:\n");
    print_bytes((char *) response, challenge_size);
    printf("[info] expected:\n");
    print_bytes((char *) expected_response, challenge_size);

    if (memcmp(response, expected_response, challenge_size) == 0) {
      const char* oldpwd = xstr_get_cstr(mapping->old_passwd);
      proxy_auth_passed = XTRUE;
      xstr_set_cstr(vnc_host, xstr_get_cstr(mapping->dest_host));
      vnc_port = mapping->dest_port;
      for (j = 0; j < 8 && j < xstr_len(mapping->old_passwd); j++) {
        real_server_auth_key[j] = (unsigned char) oldpwd[j];
      }
      while (j < 8) {
        real_server_auth_key[j] = '\0';
        j++;
      }
      xlog_info("client was mapped to %s:%d\n", xstr_get_cstr(vnc_host), vnc_port);
      break;
    }
  }
  pthread_mutex_unlock(&vnc_mapping_mutex);

  if (proxy_auth_passed != XTRUE) {
    char* failure_message = "authentication failed";
    xlog_info("auth failed\n");

    // well, be careful about the endians
    buf[0] = 0;
    buf[1] = 0;
    buf[2] = 0;
    buf[3] = 1; // indicate failure
    buf[4] = 0;
    buf[5] = 0;
    buf[6] = 0;
    buf[7] = (unsigned char) strlen(failure_message);
    strcpy((char *) (buf + 8), failure_message);
    xlog_info("going to write the following error message:\n");
    print_bytes((char *) buf, buf[7] + 8);
    xsocket_write(client_xs, buf, buf[7] + 8);
    has_error = XTRUE;

    xfree(vnc_host);
  } else {
    // try to connect to real server
    xsocket vnc_xs;
    int auth_type_count = 0;
    xbool has_vnc_auth = XFALSE;
    xbool has_none_auth = XFALSE;

    vnc_xs = xsocket_new(vnc_host, vnc_port);
    xlog_info("connecting real VNC server %s:%d\n", xstr_get_cstr(vnc_host), vnc_port);
    //printf("[info] socket ptr is %p\n", vnc_xs);
    if (xsocket_connect(vnc_xs) != XSUCCESS) {
      xlog_info("failed to connect to real VNC server!\n");
    } else {
      xlog_info("connected to VNC server!\n");
    }

    // handshake vnc version
    cnt = xsocket_read(vnc_xs, buf, 12);
    xsocket_write(vnc_xs, "RFB 003.008\n", 12);
   
    xsocket_read(vnc_xs, buf, 1);
    auth_type_count = buf[0];
    xlog_info("real VNC server has %d types of auth\n", auth_type_count);
    xsocket_read(vnc_xs, buf, auth_type_count);

    for (i = 0; i < auth_type_count; i++) {
      if (buf[i] == 1) {
        // no auth
        has_none_auth = XTRUE;
        xlog_info("real VNC server supports none auth\n");
      } else if (buf[i] == 2) {
        // vnc auth
        has_vnc_auth = XTRUE;
        xlog_info("real VNC server supports vnc auth\n");
      }
    }

    if (has_none_auth == XTRUE) {
      // tell server to use none_auth
      xsocket_write(vnc_xs, "\1", 1);
  
    } else if (has_vnc_auth == XTRUE) {
      buf[0] = (unsigned char) 2;
      xsocket_write(vnc_xs, buf, 1);  // tell vnc server: use vnc auth
      xsocket_read(vnc_xs, challenge, challenge_size);

      rfbDesKey(real_server_auth_key, EN0);
      printf("auth on real server with keys:\n");
      print_bytes((char *) real_server_auth_key, 8);

      for (i = 0; i < challenge_size; i += 8) {
        rfbDes(challenge + i, response + i);
      }

      xsocket_write(vnc_xs, response, challenge_size);  // auth for the real vnc server
      cnt = xsocket_read(vnc_xs, buf, buf_len);

      // check success
      if (buf[3] != 0) {
        // TODO failure
        xlog_error("auth failed on real vnc server\n");
        has_error = XTRUE;
      } else {
        xlog_info("passed auth on real vnc server\n");
      }

      // redirect auth information to client
      xsocket_write(client_xs, buf, cnt);
    }

    if (has_error == XFALSE) {
      // start forwarding
      xlog_info("start forwarding...\n");
      xsocket_shortcut(client_xs, vnc_xs);
      xlog_info("client disconnected\n"); // TODO never reach this line?
    }

    xsocket_delete(vnc_xs);
  }

  xfree(buf);
  xfree(challenge);
  xfree(response);
  xfree(expected_response);
}

static xsuccess start_vnc_proxy_server(xstr bind_addr, int port) {
  int backlog = 10;
  xsuccess ret;
  xserver xs = xserver_new(bind_addr, port, backlog, vnc_proxy_acceptor, XUNLIMITED, 'p', NULL);
  if (xs == NULL) {
    xlog_error("in start_vnc_proxy_server(): failed to init xserver!\n");
    ret = XFAILURE;
  } else {
    xlog_info("VNC proxy server running on %s:%d\n", xstr_get_cstr(bind_addr), port);
    ret = xserver_serve(xs);
    xstr_delete(bind_addr);
  }
  return ret;
}

static void parse_vnc_mapping(char* str, vnc_map* mapping) {
  int i;
  if (xcstr_startwith_cstr(str, "dest_host=")) {
    for (i = 10; str[i] != '\0' && str[i] != '\r' && str[i] != '\n'; i++) {
      xstr_append_char(mapping->dest_host, str[i]);
    }
    if (str[i] != '\0') {
      parse_vnc_mapping(str + i, mapping);  // recursive call
    }
  } else if (xcstr_startwith_cstr(str, "dest_port=")) {
    xstr num_val = xstr_new();
    for (i = 10; str[i] != '\0' && str[i] != '\r' && str[i] != '\n'; i++) {
      xstr_append_char(num_val, str[i]);
    }
    mapping->dest_port = atoi(xstr_get_cstr(num_val));
    xstr_delete(num_val);
    if (str[i] != '\0') {
      parse_vnc_mapping(str + i, mapping);  // recursive call
    }
  } else if (xcstr_startwith_cstr(str, "new_passwd=")) {
    for (i = 11; str[i] != '\0' && str[i] != '\r' && str[i] != '\n'; i++) {
      xstr_append_char(mapping->new_passwd, str[i]);
    }
    if (str[i] != '\0') {
      parse_vnc_mapping(str + i, mapping);  // recursive call
    }
  } else if (xcstr_startwith_cstr(str, "old_passwd=")) {
    for (i = 11; str[i] != '\0' && str[i] != '\r' && str[i] != '\n'; i++) {
      xstr_append_char(mapping->old_passwd, str[i]);
    }
    if (str[i] != '\0') {
      parse_vnc_mapping(str + i, mapping);  // recursive call
    }
  } else {
    xbool reached_eol = XFALSE;
    for (i = 0; str[i] != '\0'; i++) {
      if (reached_eol == XFALSE) {
        if (str[i] == '\r' || str[i] == '\n') {
          reached_eol = XTRUE;
        }
      } else {
        // reached_eol == XTRUE
        if (str[i] != '\r' || str[i] != '\n') {
          break;
        }
      }
    }
    if (str[i] != '\0') {
      parse_vnc_mapping(str + i, mapping);  // recursive call
    }
  }
}

static void* ipc_server(void* args) {
  int port = *(int *) args;
  struct sockaddr_un local, remote;
  int sock_fd;
  xstr sock_fn = xstr_new();
  int len;
  int backlog = 5;
  int buf_len = 8192;
  char* buf = xmalloc_ty(buf_len, char);
  int cnt;

  xstr_printf(sock_fn, "vnc_proxy.%d.sock", port);
  if ((sock_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
    xlog_error("in opening Unix socket: ipc_server()");
    exit(1);
  }
  local.sun_family = AF_UNIX;
  strcpy(local.sun_path, xstr_get_cstr(sock_fn));
  unlink(local.sun_path); // make sure the file will be removed when this process exit
  len = strlen(local.sun_path) + sizeof(local.sun_family);
  if (bind(sock_fd, (struct sockaddr *) &local, len) < 0) {
    xlog_error("failed to bind socket in ipc_server()");
    exit(1);
  }

  if (listen(sock_fd, backlog) < 0) {
    xlog_error("failed to listen in ipc_server()");
    exit(1);
  }

  for (;;) {
    int client_sockfd;
    socklen_t t = sizeof(remote);
    xlog_info("waiting for ipc connection...\n");
    if ((client_sockfd = accept(sock_fd, (struct sockaddr *) &remote, &t)) < 0) {
      perror("failed to accept");
      exit(1);
    }
    xlog_info("got client ipc connection!\n");

    // no need to split a thread for client connection, since there won't be multiple vnc_proxy_ctl running

    strcpy(buf, "This is vnc_proxy\r\n");
    send(client_sockfd, buf, strlen(buf) + 1, 0);
    cnt = recv(client_sockfd, buf, buf_len, 0);
    xlog_info("got message from client: '%s', size=%d\n", buf, cnt);

    strcpy(buf, "OK\r\n");
    send(client_sockfd, buf, strlen(buf) + 1, 0);

    // NOTE use locks when handling vnc_mapping, if not LIST action
    cnt = recv(client_sockfd, buf, buf_len, 0);
    xlog_info("got command from client: '%s', size=%d\n", buf, cnt);

    if (xcstr_startwith_cstr(buf, "list") == XTRUE) {
      // no need to lock the mapping
      int i;
      xstr msg = xstr_new();
      xlog_info("below is the list of all vnc mapping\n");
      xstr_printf(msg, "%d\n", xvec_size(vnc_mapping));
      for (i = 0; i < xvec_size(vnc_mapping); i++) {
        vnc_map* mapping = xvec_get(vnc_mapping, i);
        xstr_printf(msg, "dest_host=%s\n", xstr_get_cstr(mapping->dest_host));
        xstr_printf(msg, "dest_port=%d\n", mapping->dest_port);
        xstr_printf(msg, "old_passwd=%s\n", xstr_get_cstr(mapping->old_passwd));
        xstr_printf(msg, "new_passwd=%s\n", xstr_get_cstr(mapping->new_passwd));
        xstr_printf(msg, "\n");
      }
      printf("%s\n", xstr_get_cstr(msg));
      printf("[info] end of vnc listing\n");
      send(client_sockfd, xstr_get_cstr(msg), xstr_len(msg), 0);

      xstr_delete(msg);

    } else {
      int i;
      vnc_map* new_mapping = xmalloc_ty(1, vnc_map);
      new_mapping->dest_host = xstr_new();
      new_mapping->dest_port = -1;
      new_mapping->new_passwd = xstr_new();
      new_mapping->old_passwd = xstr_new();
      parse_vnc_mapping(buf, new_mapping);

      pthread_mutex_lock(&vnc_mapping_mutex);

      if (xcstr_startwith_cstr(buf, "add") == XTRUE) {
        xstr reply_msg = xstr_new();
        xstr_set_cstr(reply_msg, "success\r\n");

        if (xstr_len(new_mapping->new_passwd) == 0) {
          xstr_set_cstr(reply_msg, "new password not given, this proxy mapping will be ignored!\r\n");
          xlog_warning("%s", xstr_get_cstr(reply_msg));
        } else {
          // warn if there is alread a mapping with same new passwd (first 8 bytes)
          for (i = 0; i < xvec_size(vnc_mapping); i++) {
            vnc_map* mapping = xvec_get(vnc_mapping, i);
            int j;
            const char* new_pwd = xstr_get_cstr(new_mapping->new_passwd);
            const char* existing_pwd = xstr_get_cstr(mapping->new_passwd);
            for (j = 0; j < 8 && new_pwd[j] != '\0' && existing_pwd[j] != '\0'; j++) {
              if (new_pwd[j] != existing_pwd[j]) {
                break;
              }
            }
            if (new_pwd[j] == existing_pwd[j] && new_pwd[j] == '\0') {
              xstr_set_cstr(reply_msg, "same passwd (first 8 bytes) already exists!\r\n");
              xlog_warning("%s", xstr_get_cstr(reply_msg));
            }
          }
        }
        xvec_push_back(vnc_mapping, new_mapping);
        send(client_sockfd, xstr_get_cstr(reply_msg), xstr_len(reply_msg) + 1, 0);
        xstr_delete(reply_msg);
      } else if (xcstr_startwith_cstr(buf, "del.dest") == XTRUE) {
        xstr reply_msg = xstr_new();
        int del_count = 0;
        if (new_mapping->dest_port > 0) {
          // dest port given
          for (i = xvec_size(vnc_mapping) - 1; i >= 0; i--) {
            // removing from the back ends
            vnc_map* mapping = xvec_get(vnc_mapping, i);
            if (strcmp(xstr_get_cstr(mapping->dest_host), xstr_get_cstr(new_mapping->dest_host)) == 0 && mapping->dest_port == new_mapping->dest_port) {
              xvec_remove(vnc_mapping, i);
              del_count++;
            }
          }
        } else {
          // dest port not given, delete all mapping with same host
          for (i = xvec_size(vnc_mapping) - 1; i >= 0; i--) {
            vnc_map* mapping = xvec_get(vnc_mapping, i);
            if (strcmp(xstr_get_cstr(mapping->dest_host), xstr_get_cstr(new_mapping->dest_host)) == 0) {
              xvec_remove(vnc_mapping, i);
              del_count++;
            }
          }
        }
        xstr_printf(reply_msg, "%d proxy(s) deleted\r\n", del_count);
        send(client_sockfd, xstr_get_cstr(reply_msg), xstr_len(reply_msg), 0);
        xstr_delete(reply_msg);
      } else if (xcstr_startwith_cstr(buf, "del.newpasswd") == XTRUE) {
        int del_count = 0;
        xstr reply_msg = xstr_new();
        for (i = xvec_size(vnc_mapping) - 1; i >= 0; i--) {
          // removing from the back ends
          vnc_map* mapping = xvec_get(vnc_mapping, i);
          if (strcmp(xstr_get_cstr(mapping->new_passwd), xstr_get_cstr(new_mapping->new_passwd)) == 0) {
            xvec_remove(vnc_mapping, i);
            del_count++;
          }
        }
        xstr_printf(reply_msg, "%d proxy(s) deleted!\r\n", del_count);
        send(client_sockfd, xstr_get_cstr(reply_msg), xstr_len(reply_msg), 0);
        xstr_delete(reply_msg);
      } else {
        // failure
        strcpy(buf, "failure: command not known\r\n");
        send(client_sockfd, buf, strlen(buf) + 1, 0);
      }

      pthread_mutex_unlock(&vnc_mapping_mutex);
    }

    xlog_info("client disconnected\n");
    close(client_sockfd);
  }
  
  xfree(buf);
  xfree(args);
  xstr_delete(sock_fn);
  return NULL;
}

static void start_ipc_server(int port) {
  pthread_t tid;
  int* port_copy = xmalloc_ty(1, int);
  *port_copy = port;
  if (pthread_create(&tid, NULL, ipc_server, (void *) port_copy) < 0) {
    perror("error in pthread_create()");
  }
}

int main(int argc, char* argv[]) {
  xstr bind_addr = xstr_new();
  int port = 5900;
  int i;
  xbool ask_for_help = XFALSE;

  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      ask_for_help = XTRUE;
    }
  }

  if (ask_for_help) {
    printf("usage: vnc_proxy [-b bind_addr] [-p bind_port]\n");
    printf("       bind_addr is default to 0.0.0.0\n");
    printf("       bind_port is default to 5900\n");
    printf("\n");
    printf("You need to use vnc_proxy_ctl to change proxy settings\n");
    exit(0);
  }

  srand(time(NULL));
  xstr_set_cstr(bind_addr, "0.0.0.0");

  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-p") == 0) {
      if (i + 1 < argc) {
        // TODO check if not number
        sscanf(argv[i + 1], "%d", &port);
      } else {
        printf("error in cmdline args: '-p' must be followed by port number!\n");
        exit(1);
      }
    } else if (xcstr_startwith_cstr(argv[i], "--port=")) {
      // TODO check if not number
      sscanf(argv[i] + 7, "%d", &port);
    } else if (strcmp(argv[i], "-b") == 0) {
      if (i + 1 < argc) {
        // TODO check ip address format
        xstr_set_cstr(bind_addr, argv[i + 1]);
      } else {
        printf("error in cmdline args: '-b' must be followed by bind address!\n");
        exit(1);
      }
    } else if (xcstr_startwith_cstr(argv[i], "--bind=")) {
      // TODO check ip address format
      xstr_set_cstr(bind_addr, argv[i] + 7);
    }
  }

  printf("If you need help, use: 'vnc_proxy -h' or 'vnc_proxy --help'\n");

  // TODO prepare the vnc mapping hash table
  vnc_mapping = xvec_new(vnc_mapping_free);

  // ipc server is started in a new thread
  start_ipc_server(port);

  return start_vnc_proxy_server(bind_addr, port);
}


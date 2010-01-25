#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <errno.h>

#include "xnet.h"
#include "xmemory.h"
#include "xutils.h"
#include "xlog.h"

// if this is set to 1, then xserver.serve will be profiled
#define PROFILE_XSERVER 1


/**
  @brief
    Implementation of xsocket object, hidden from users.
*/
struct xsocket_impl {
  xstr host;  ///< @brief The host address where the xsocket is bound to.
  int port; ///< @brief The port where the xsocket is bound to.
  struct sockaddr_in addr;  ///< @brief The socket address of this xsocket.
  int sockfd; ///< @brief The file descriptor bound to this xsocket.
};

/**
  @brief
    Implementation of xserver object, hidden from users.
*/
struct xserver_impl {
  xsocket sock; ///< @brief The xsocket object, containing the xserver's service address & port number.
  int backlog;  ///< @brief The backlog parameter for listen() function.
  xserver_acceptor acceptor;  ///< @brief The acceptor call-back function.
  int serv_count; ///< @brief How many rounds of service will be given.
  char serv_mode; ///< @brief Service mode: 'b' blocking, 'p' new process, 't' new thread
  void* args; ///< @brief Additional arguments for acceptor function.
};

// host will be deleted when deleting xsocked
xsocket xsocket_new(xstr host, int port) {
  xsocket xsock = xmalloc_ty(1, struct xsocket_impl);
  xsock->host = host;
  xsock->port = port;
  if (xinet_get_sockaddr(xstr_get_cstr(host), port, &(xsock->addr)) != XSUCCESS) {
    xsocket_delete(xsock);
    xsock = NULL;
  } else {
    xsock->sockfd = socket(AF_INET, SOCK_STREAM, 0);
  }
  return xsock;
}

int xsocket_get_socket_fd(xsocket xs) {
  return xs->sockfd;
}

const char* xsocket_get_host_cstr(xsocket xs) {
  return xstr_get_cstr(xs->host);
}

int xsocket_get_port(xsocket xs) {
  return ntohs(xs->port);
}

int xsocket_write(xsocket xs, const void* data, int len) {
  if (len != 0) {
    return write(xs->sockfd, data, len);
  } else {
    return 0;
  }
}

int xsocket_read(xsocket xs, void* buf, int max_len) {
  if (max_len != 0) {
    return read(xs->sockfd, buf, max_len);
  } else {
    return 0;
  }
}

xsuccess xsocket_connect(xsocket xs) {
  if (connect(xs->sockfd, (struct sockaddr *)&(xs->addr), sizeof(xs->addr)) < 0) {
    return XFAILURE;
  } else {
    return XSUCCESS;
  }
}

void xsocket_delete(xsocket xs) {
  // do not use shutdown here
  // otherwise deleting xsocket in main process will also
  // close it in child process (when serving in multi-process)
  close(xs->sockfd);
  xstr_delete(xs->host);
  xfree(xs);
}

void xsocket_shortcut(xsocket xs1, xsocket xs2) {
  fd_set r_set;
  fd_set w_set;
  fd_set ex_set;
  struct timeval time_out;
  int maxfdp1 = 0;
  const int buf_len = 8192;
  char* buf = xmalloc_ty(buf_len, char);
  int cnt;

  if (xs1->sockfd + 1 > maxfdp1) {
    maxfdp1 = xs1->sockfd + 1;
  }
  if (xs2->sockfd + 1 > maxfdp1) {
    maxfdp1 = xs2->sockfd + 1;
  }

  time_out.tv_sec = 0;
  time_out.tv_usec = 50 * 1000; // 50msec time out

  for (;;) {
    xbool has_activity = XFALSE;  // whether some data was sent in this round
    FD_ZERO(&r_set);
    FD_ZERO(&w_set);
    FD_ZERO(&ex_set);
    FD_SET(xs1->sockfd, &r_set);
    FD_SET(xs1->sockfd, &w_set);
    FD_SET(xs1->sockfd, &ex_set);
    FD_SET(xs2->sockfd, &w_set);
    FD_SET(xs2->sockfd, &r_set);
    FD_SET(xs1->sockfd, &ex_set);

    if (select(maxfdp1, &r_set, &w_set, &ex_set, &time_out) == -1) {
      xlog_debug("[info] select < 0\n");
      break;
    }

    if (FD_ISSET(xs1->sockfd, &r_set) && FD_ISSET(xs2->sockfd, &w_set)) {
      cnt = xsocket_read(xs1, buf, buf_len);
      if (cnt > 0) {
        if (xsocket_write(xs2, buf, cnt) <= 0) {
          // TODO is "<=" correct?
          break;
        }
        has_activity = XTRUE;
      } else if (cnt == 0) {
        break;  // is this correct?
      } else if (cnt < 0) {
        xlog_debug("[info] sock cnt < 0\n");
        break;
      }
    }
    if (FD_ISSET(xs2->sockfd, &r_set) && FD_ISSET(xs1->sockfd, &w_set)) {
      cnt = xsocket_read(xs2, buf, buf_len);
      if (cnt > 0) {
        if (xsocket_write(xs1, buf, cnt) < 0) {
          // TODO is "<=" correct?
          break;
        }
        has_activity = XTRUE;
      } else if (cnt == 0) {
        break;
      } else if (cnt < 0) {
        xlog_debug("[info] sock cnt < 0\n");
        break;
      }
    }

    if (FD_ISSET(xs1->sockfd, &ex_set) || FD_ISSET(xs2->sockfd, &ex_set)) {
      xlog_debug("[info] sock exception encountered!\n");
      break;
    }

    if (has_activity == XFALSE) {
      // if no data was sent, sleep for a very short time, prevent high CPU usage
      usleep(1);
    }
  }
  xfree(buf);
}

static void xserver_delete(xserver xs) {
  xsocket_delete(xs->sock);
  xfree(xs);
}

xserver xserver_new(xstr host, int port, int backlog, xserver_acceptor acceptor, int serv_count, char serv_mode, void* args) {
  xserver xs = xmalloc_ty(1, struct xserver_impl);

#if PROFILE_XSERVER == 1
  struct timeval start_time, end_time;
  gettimeofday(&start_time, NULL);
#endif  // PROFILE_XSERVER

  xs->sock = xsocket_new(host, port);
  xs->backlog = backlog;
  xs->acceptor = acceptor;
  xs->serv_count = serv_count;
  xs->serv_mode = serv_mode;
  xs->args = args;
  if (bind(xs->sock->sockfd, (struct sockaddr *) &(xs->sock->addr), sizeof(struct sockaddr)) < 0) {
    perror("error in bind()");
    xserver_delete(xs);
    xs = NULL;
  }
  if (xs != NULL && listen(xs->sock->sockfd, xs->backlog) < 0) {
    perror("error in listen()");
    xserver_delete(xs);
    xs = NULL;
  }
#if PROFILE_XSERVER == 1
  gettimeofday(&end_time, NULL);
  xlog_info("[prof] creating an xserver took %d, %d (sec, usec)\n", (int) (end_time.tv_sec - start_time.tv_sec), (int) (end_time.tv_usec - start_time.tv_usec));
#endif  // PROFILE_XSERVER
  return xs;
}

static void* acceptor_wrapper(void* pthread_arg) {
  void** arglist = (void **) pthread_arg;
  xserver xserver = arglist[0];
  xsocket client_xs = arglist[1];
  void* args = arglist[2];

  xserver->acceptor(client_xs, args);

  xfree(pthread_arg);
  xsocket_delete(client_xs);

  xmem_usage(stdout);
  return NULL;
}


xsuccess xserver_serve(xserver xs) {
  int serv_count = 0;
  xsuccess ret = XSUCCESS;

  // prevent zombie processes
  if (xs->serv_mode == 'p' || xs->serv_mode == 'P') {
    signal(SIGCHLD, SIG_IGN);
  }

  while (xs->serv_count == XUNLIMITED || serv_count < xs->serv_count) {
    struct sockaddr_in client_addr;
    socklen_t sin_size = sizeof(client_addr);
    int client_sockfd = accept(xs->sock->sockfd, (struct sockaddr *) &client_addr, &sin_size);
    if (client_sockfd < 0) {
      xlog_error("in xserver accept(): errno=%d : %s\n", errno, strerror(errno));
      xlog_info("params: xs->sock->sockfd = %d\n", xs->sock->sockfd);
      continue;
    }
    xsocket client_xs = xmalloc_ty(1, struct xsocket_impl); // xfree'd in acceptor wrapper

#if PROFILE_XSERVER == 1
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);
#endif  // #if PROFILE_XSERVER == 1

    client_xs->sockfd = client_sockfd;
    client_xs->addr = client_addr;
    client_xs->port = ntohs(client_addr.sin_port);
    client_xs->host = xstr_new();
    xstr_set_cstr(client_xs->host, inet_ntoa(client_addr.sin_addr));

    serv_count++;

    if (xs->serv_mode == 't' || xs->serv_mode == 'T') {
      // serve in new thread
      pthread_t tid;
      void** arglist = xmalloc_ty(3, void *);  // will be xfree'd in acceptor wrapper
      arglist[0] = xs;
      arglist[1] = client_xs;
      arglist[2] = xs->args;
      if (pthread_create(&tid, NULL, acceptor_wrapper, (void *) arglist) < 0) {
        perror("error in pthread_create()");
        // TODO handle error creating new thread
        ret = XFAILURE;
        break;
      }
    } else if (xs->serv_mode == 'p' || xs->serv_mode == 'P') {
      pid_t pid = fork();
      if (pid < 0) {
        perror("error in forking new process");
        ret = XFAILURE;
        break;
      } else if (pid == 0) {
        // child process

        // begin monitoring mem usage in service process
        xmem_reset_counter();

#if PROFILE_XSERVER == 1
        gettimeofday(&end_time, NULL);
        xlog_info("[prof] accepting a client in xserver_xserve took %d, %d (sec, usec)\n", (int) (end_time.tv_sec - start_time.tv_sec), (int) (end_time.tv_usec - start_time.tv_usec));
#endif  // #if PROFILE_XSERVER == 1

        xs->acceptor(client_xs, xs->args);
        
        if (xmem_usage(stdout) != 0) {
          xlog_warning("[xdk] possible memory leak in xserver's service process!\n");
        }
        xsocket_delete(client_xs);
        // exit child process
        exit(0);

      } else {
        // parent process
        
        // tricky here, reset rand seed, prevent sub process from rand value collisions
        srand(pid);

        // release client handle in this process
        // also release allocated memory resource, make xmem monitor happy
        xsocket_delete(client_xs);
      }
    } else {
      // blocking
      xs->acceptor(client_xs, xs->args);
      xsocket_delete(client_xs);
    }
  }
  xserver_delete(xs); // self destroy
  return ret;
}

int xserver_get_port(xserver xs) {
  return xs->sock->port;
}


const char* xserver_get_ip_cstr(xserver xs) {
  return inet_ntoa(xs->sock->addr.sin_addr);
}


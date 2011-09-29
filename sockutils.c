#include <netinet/in.h>
#include <netdb.h>
#include <sys/un.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

#include "config.h"
#include "sockutils.h"


static int zinet_get_sockaddr(const char* host, int port, struct sockaddr_in* addr) {
  in_addr_t a;
  if (host == NULL) {
    return -1;
  }
  bzero(addr, sizeof(*addr));
  addr->sin_family = AF_INET;
  a = inet_addr(host);
  if (a != INADDR_NONE) {
    addr->sin_addr.s_addr = a;
  } else {
    struct hostent* hp = gethostbyname(host);
    if (hp == 0 || hp->h_length != 4) {
      return -1;
    }
  }
  addr->sin_port = htons(port);
  return 0;
}



int tcpsvrsock(const char* host, int port) {
  int sock;
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("failed to create socket");
    return -1;
  }

  sockreuseaddr(sock);

  struct sockaddr_in sa;
  if (zinet_get_sockaddr(host, port, &sa) != 0) {
    perror("failed to get socket address");
    close(sock);
    return -1;
  }
  if (bind(sock, (struct sockaddr *) &sa, sizeof(struct sockaddr_in)) < 0) {
    perror("failed to bind socket");
    close(sock);
    return -1;
  }
  const int back_log = 100; // TODO config?
  if (listen(sock, back_log) < 0) {
    perror("failed to listen to socket");
    close(sock);
    return -1;
  }
  return sock;
}


int tcpclntsock(const char* host, int port) {
  int sock;
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("failed to create socket");
    return -1;
  }

  struct sockaddr_in sin;
  struct hostent *resolv = gethostbyname(host);
  if (host == NULL) {
    perror("failed to resolve host");
    close(sock);
    return -1;
  }
  memcpy(&sin.sin_addr.s_addr, resolv->h_addr, resolv->h_length);
  sin.sin_family = AF_INET;
  sin.sin_port = htons(port);
  if (connect(sock, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
    perror("failed to connect to remote");
    close(sock);
    return -1;
  }
  return sock;
}


int unixsvrsock(const char* sock_fpath) {
  int sock;
  if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
    perror("failed to create socket");
    return -1;
  }

  sockreuseaddr(sock);

  struct sockaddr_un local;
  local.sun_family = AF_UNIX;
  strcpy(local.sun_path, sock_fpath);
  unlink(local.sun_path); // make sure the file will be removed when this process exit
  int sock_sz = strlen(local.sun_path) + sizeof(local.sun_family);
  if (bind(sock, (struct sockaddr *) &local, sock_sz) < 0) {
    perror("failed to bind socket");
    close(sock);
    return -1;
  }
  const int backlog = 100;  // TODO config?
  if (listen(sock, backlog) < 0) {
    perror("failed to listen to socket");
    close(sock);
    return -1;
  }
  return sock;
}


int unixclntsock(const char* sock_fpath) {
  struct sockaddr_un mg_sock;
  int sock_fd;
  if ((sock_fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
    perror("failed to create UNIX socket");
    return -1;
  }

  mg_sock.sun_family = AF_UNIX;
  strcpy(mg_sock.sun_path, sock_fpath);
  int sock_sz = strlen(mg_sock.sun_path) + sizeof(mg_sock.sun_family);
  if (connect(sock_fd, (struct sockaddr *) &mg_sock, sock_sz) < 0) {
    perror("failed to connect to socket");
    close(sock_fd);
    return -1;
  }
  return sock_fd;
}

static int setnonblocking(int sock) {
#ifdef O_NONBLOCK
  int flags = fcntl(sock, F_GETFL, 0);
  if (flags == -1) {
    perror("socknonblock(): fcntl GET failed");
    return -1;
  }
  if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1) {
    perror("socknonblock(): fcntl SET failed");
    return -1;
  } else {
    return 0;
  }
#else
  int yes = 1;
  if (ioctl(sock, FIONBIO, &yes) == -1) {
    perror("socknonblock(): ioctl failed");
    return -1;
  } else {
    return 0;
  }
#endif
}

int socknonblock(int sock) {
  return setnonblocking(sock);
}

int sockreuseaddr(int sock) {
  int yes = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*) &yes, sizeof(int)) == -1) {
    perror("sockreuseaddr failed");
    return -1;
  } else {
    return 0;
  }
}

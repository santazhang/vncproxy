#ifndef SOCKUTILS_H
#define SOCKUTILS_H

#ifdef __cplusplus
extern "C" {
#endif

int tcpsvrsock(const char* host, int port);

int tcpclntsock(const char* host, int port);

int unixsvrsock(const char* sock_fpath);

int unixclntsock(const char* sock_fpath);

int socknonblock(int sock_fd);

int sockreuseaddr(int sock_fd);

#ifdef __cplusplus
}
#endif

#endif

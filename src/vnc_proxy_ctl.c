#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>

#include "xdef.h"
#include "xmemory.h"
#include "xutils.h"
#include "xstr.h"

// global socket file name. if it is not NULL, then we could just use it, otherwise we look for socket file in
// working dir.
char* g_sock_fn_cstr = NULL;

// whether we should show minimal info for "list" action
xbool g_quiet_listing = XTRUE;

int connect_server() {
  int sockfd = -1;
  DIR* p_dir = opendir(".");
  struct dirent* p_dirent;
  struct stat st;
  xstr sock_fn = xstr_new();
  struct sockaddr_un remote;
  int len;
  int cnt;
  int buf_len = 8192;
  char* buf = xmalloc_ty(buf_len, char);

  if (g_sock_fn_cstr == NULL) {
    // socket file not given in cmdline, find it in working dir
    while ((p_dirent = readdir(p_dir)) != NULL) {
      lstat(p_dirent->d_name, &st);
      if (S_ISSOCK(st.st_mode)) {
        printf("[info] found socket %s\n", p_dirent->d_name);
        if (xstr_len(sock_fn) > 0) {
          printf("[error] multiple socket found!\n");
          exit(1);
        } else {
          xstr_set_cstr(sock_fn, p_dirent->d_name);
          // don't break now, we have to check if there are multiple sock files.
        }
      }
    }
  } else {
    // socket file already given, use it directly
    xstr_set_cstr(sock_fn, g_sock_fn_cstr);
  }

  if ((sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
    perror("failed to create socket\n");
    exit(1);
  }

  remote.sun_family = AF_UNIX;
  strcpy(remote.sun_path, xstr_get_cstr(sock_fn));
  len = strlen(remote.sun_path) + sizeof(remote.sun_family);
  if (connect(sockfd, (struct sockaddr *) &remote, len) < 0) {
    perror("failed to connect to server\n");
    exit(1);
  }

  //printf("[info] connected\n");

  cnt = recv(sockfd, buf, buf_len, 0);
  //printf("[info] got message from server: '%s', size=%d\n", buf, cnt);

  strcpy(buf, "This is vnc_proxy_ctl\r\n");
  send(sockfd, buf, strlen(buf) + 1, 0);

  // recv the "OK"
  recv(sockfd, buf, buf_len, 0);

  closedir(p_dir);

  xfree(buf);
  xstr_delete(sock_fn);
  return sockfd;
}

xsuccess add_vnc_proxy(xstr vnc_host, int vnc_port, const char* new_passwd, const char* old_passwd) {
  xsuccess ret = XFAILURE;
  int sockfd = connect_server();
  printf("[info] add, dest=%s:%d, new_pass=%s, old_pass=%s\n", xstr_get_cstr(vnc_host), vnc_port, new_passwd, old_passwd);
  if (sockfd > 0) {
    int buf_len = 8192;
    char* buf = xmalloc_ty(buf_len, char);
    int cnt;
    xstr msg = xstr_new();
    xstr_printf(msg, "add\r\ndest_host=%s\r\ndest_port=%d\r\nold_passwd=%s\r\nnew_passwd=%s\r\n", xstr_get_cstr(vnc_host), vnc_port, old_passwd, new_passwd);
    printf("[info] sending to server:\n%s\n", xstr_get_cstr(msg));
    send(sockfd, xstr_get_cstr(msg), xstr_len(msg) + 1, 0);
    cnt = recv(sockfd, buf, buf_len, 0);
    buf[cnt] = '\0';
    printf("[info] recv'ed from server: '%s'\n", buf);

    xstr_delete(msg);
    xfree(buf);
    close(sockfd);
  }
  return ret;
}

xsuccess del_vnc_proxy_by_passwd(const char* new_passwd) {
  xsuccess ret = XFAILURE;
  int sockfd = connect_server();
  printf("[info] del, new_pass=%s\n", new_passwd);
  if (sockfd > 0) {
    int buf_len = 8192;
    char* buf = xmalloc_ty(buf_len, char);
    int cnt;
    xstr msg = xstr_new();
    xstr_printf(msg, "del.newpasswd\r\nnew_passwd=%s\r\n", new_passwd);
    printf("[info] sending to server:\n%s\n", xstr_get_cstr(msg));
    send(sockfd, xstr_get_cstr(msg), xstr_len(msg) + 1, 0);
    cnt = recv(sockfd, buf, buf_len, 0);
    buf[cnt] = '\0';
    printf("[info] recv'ed from server: '%s'\n", buf);

    xfree(buf);
    xstr_delete(msg);
    close(sockfd);
  }
  return ret;
}

xsuccess del_vnc_proxy_by_dest(xstr vnc_host, int vnc_port) {
  xsuccess ret = XFAILURE;
  int sockfd = connect_server();
  if (vnc_port > 0) {
    printf("[info] del, dest=%s:%d\n", xstr_get_cstr(vnc_host), vnc_port);
  } else {
    printf("[info] del, dest=%s:%%\n", xstr_get_cstr(vnc_host));
  }
  if (sockfd > 0) {
    int buf_len = 8192;
    char* buf = xmalloc_ty(buf_len, char);
    int cnt;
    xstr msg = xstr_new();
    if (vnc_port > 0) {
      xstr_printf(msg, "del.dest\r\ndest_host=%s\r\ndest_port=%d\r\n", xstr_get_cstr(vnc_host), vnc_port);
    } else {
      xstr_printf(msg, "del.dest\r\ndest_host=%s\r\n", xstr_get_cstr(vnc_host));
    }
    printf("[info] sending to server:\n%s\n", xstr_get_cstr(msg));
    send(sockfd, xstr_get_cstr(msg), xstr_len(msg) + 1, 0);
    cnt = recv(sockfd, buf, buf_len, 0);
    buf[cnt] = '\0';
    printf("[info] recv'ed from server: '%s'\n", buf);
    xfree(buf);
    xstr_delete(msg);
    close(sockfd);
  }
  return ret;
}

void list_vnc_proxy() {
  int sockfd = connect_server();
  if (g_quiet_listing == XFALSE) {
    printf("[info] list\n");
  }
  if (sockfd > 0) {
    int buf_len = 8192;
    char* buf = xmalloc_ty(buf_len, char);
    int cnt;
    xstr msg = xstr_new();
    
    strcpy(buf, "list\r\n");
    send(sockfd, buf, strlen(buf) + 1, 0);

    while ((cnt = recv(sockfd, buf, buf_len - 1, 0)) > 0) {
      buf[cnt] = '\0';
      xstr_append_cstr(msg, buf);
    }

    if (g_quiet_listing == XFALSE) {
      printf("[info] recv'ed from server:\n");
    }
    printf("%s\n", xstr_get_cstr(msg));

    close(sockfd);
    xstr_delete(msg);
    xfree(buf);
  }
}

int main(int argc, char* argv[]) {
  int i;
  xstr dest_addr = xstr_new();
  xstr vnc_host = xstr_new();
  int vnc_port = -1;
  xstr new_passwd = xstr_new();
  xstr old_passwd = xstr_new();
  xbool ask_for_help = XFALSE;

  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      ask_for_help = XTRUE;
    }
  }
  if (argc == 1 || ask_for_help == XTRUE) {
    printf("usage: vnc_proxy_ctl add [-p <new password>|--password=<new password>] [-op <old password>|--old-password=<old password>] -d <dest>|--dest=<dest> [-s socket_fn]\n");
    printf("       vnc_proxy_ctl del [-p <new password>|--password=<new password>] [-d <dest>|--dest=<dest_ip>[:dest_port]] [-s socket_fn]\n");
    printf("       vnc_proxy_ctl list [-s socket_fn] [-v]\n");
    printf("       'socket_fn' is the path to the socket file of vnc_proxy.\n");
    printf("       for 'list' action, add '-v' to show more info.\n");
    exit(0);
  }
    
  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-p") == 0) {
      if (i + 1 < argc) {
        xstr_set_cstr(new_passwd, argv[i + 1]);
      } else {
        printf("error in cmdline args: '-p' must be followed by new password!\n");
        exit(1);
      }
    } else if (xcstr_startwith_cstr(argv[i], "--password=")) {
      xstr_set_cstr(new_passwd, argv[i] + 11);
    } else if (strcmp(argv[i], "-op") == 0) {
      if (i + 1 < argc) {
        xstr_set_cstr(old_passwd, argv[i + 1]);
      } else {
        printf("error in cmdline args: '-op' must be followed by old password!\n");
        exit(1);
      }
    } else if (xcstr_startwith_cstr(argv[i], "--old-password=")) {
      xstr_set_cstr(old_passwd, argv[i] + 15);
    } else if (strcmp(argv[i], "-d") == 0) {
      if (i + 1 < argc) {
        xstr_set_cstr(dest_addr, argv[i + 1]);
      } else {
        printf("error in cmdline args: '-d' must be followed by destination address!\n");
        exit(1);
      }
    } else if (xcstr_startwith_cstr(argv[i], "--dest=")) {
      xstr_set_cstr(dest_addr, argv[i] + 7);
    } else if (strcmp(argv[i], "-s") == 0) {
      if (i + 1 < argc) {
        g_sock_fn_cstr = argv[i + 1];
      } else {
        printf("error in cmdline args: '-s' must be followed by socket file path!\n");
        exit(1);
      }
    } else if (strcmp(argv[i], "-v") == 0) {
      g_quiet_listing = XFALSE;
    }
  }

  if (xstr_len(dest_addr) != 0) {
    const char* dest_cstr = xstr_get_cstr(dest_addr);
    xbool has_dest_port = XFALSE;
    int split_index = 0;
    for (i = 0; dest_cstr[i] != '\0'; i++) {
      if (dest_cstr[i] == ':') {
        has_dest_port = XTRUE;
        split_index = i;
        break;
      }
    }
    if (has_dest_port == XTRUE) {
      vnc_port = atoi(dest_cstr + split_index + 1);
      for (i = 0; i < split_index; i++) {
        xstr_append_char(vnc_host, dest_cstr[i]);
      }
    } else if (strcmp(argv[1], "del") == 0) {
      vnc_port = -1;
      xstr_set_cstr(vnc_host, xstr_get_cstr(dest_addr));
    } else {
      printf("[error] dest port not given! dest addr is like 'ip:port'!\n");
      exit(1);
    }
  }

  if (strcmp(argv[1], "add") == 0) {
    if (xstr_len(vnc_host) == 0) {
      printf("[error] dest addr not given!\n");
    } else {
      add_vnc_proxy(vnc_host, vnc_port, xstr_get_cstr(new_passwd), xstr_get_cstr(old_passwd));
    }
  } else if (strcmp(argv[1], "del") == 0) {
    if (xstr_len(vnc_host) != 0) {
      del_vnc_proxy_by_dest(vnc_host, vnc_port);
    } else if (xstr_len(new_passwd) != 0) {
      del_vnc_proxy_by_passwd(xstr_get_cstr(new_passwd));
    } else {
      printf("[error] wrong parameters for 'del' action!\n");
    }
  } else if (strcmp(argv[1], "list") == 0) {
    list_vnc_proxy();
  } else {
    printf("[error] unknown action '%s'\n", argv[1]);
  }

  xstr_delete(vnc_host);
  xstr_delete(new_passwd);
  xstr_delete(old_passwd);
  xstr_delete(dest_addr);

  return 0;
}


#include <stdio.h>
#include <string.h>

#include "xstr.h"
#include "xnet.h"
#include "xmemory.h"
#include "xutils.h"

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

// returns: 0x3003 or 0x3007 or 0x3008
// or return -1 on failure
static int get_server_version(xsocket xs) { char buf[12];
  int cnt = xsocket_read(xs, buf, 12);
  print_bytes(buf, 12);
  if (cnt < 0) {
    return -1;
  } else if (xcstr_startwith_cstr(buf, "RFB 003.003") == XTRUE) {
    return 0x3003;
  } else if (xcstr_startwith_cstr(buf, "RFB 003.007") == XTRUE) {
    return 0x3007;
  } else if (xcstr_startwith_cstr(buf, "RFB 003.008") == XTRUE) {
    return 0x3008;
  } else {
    return -1;
  }
}

static void exchange_server_security_types(xsocket xs) {
  char number;
  char* buf;
  int cnt = xsocket_read(xs, &number, 1);
  int i;
  xbool has_vnc_auth = XFALSE;
  print_bytes(&number, 1);
  printf("Number of security options: %d\n", (int) number);
  // if number == 0, then connection failed
  buf = xmalloc_ty(((int) number), char);
  xsocket_read(xs, buf, (int) number);
  print_bytes(buf, (int) number);
  for (i = 0; i < (int) number; i++) {
    if (buf[i] == 0) {
      printf("%d: invalid\n", buf[i]);
    } else if (buf[i] == 1) {
      printf("%d: none\n", buf[i]);
    } else if (buf[i] == 2) {
      printf("%d vnc auth\n", buf[i]);
      has_vnc_auth = XTRUE;
    } else {
      printf("%d other\n", buf[i]);
    }
  }
  if (has_vnc_auth) {
    char vnc_auth_char = (char) 2;
    xsocket_write(xs, &vnc_auth_char, 1);
  }
  xfree(buf);
}


static void do_vnc_auth(xsocket xs) {
  char challenge[16];
  char response[16];
  xsocket_read(xs, challenge, 16);
  printf("Got challenge:\n");
  print_bytes(challenge, 16);
}

// the main service function
// xs: a connectted xsocket
// do not delete it inside this function
void vnc_proxy(xsocket xs) {
  int cnt;
  int buf_len = 8192;
  char* buf = xmalloc_ty(buf_len, char);
  int server_version = get_server_version(xs);
  
  printf("Server version: %x\n", server_version);

  // only support rfb v3.8
  strcpy(buf, "RFB 003.008\n");
  cnt = xsocket_write(xs, buf, 12);

  exchange_server_security_types(xs);

  do_vnc_auth(xs);

  xfree(buf);
}

int main() {
  xstr host_str = xstr_new();
  xstr_set_cstr(host_str, "localhost");
  xsocket xs = xsocket_new(host_str, 5901);

  printf("This is vnc_proxy!\n");
  printf("Connecting to %s\n", xstr_get_cstr(host_str));

  xsocket_connect(xs);
  vnc_proxy(xs);
  xsocket_delete(xs);

  return xmem_usage(NULL);
}


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>

#include "xstr.h"
#include "xnet.h"
#include "xhash.h"
#include "xmemory.h"
#include "xcrypto.h"
#include "xutils.h"

#include "vnc_auth/d3des.h"

// globally shared data
xhash vnc_mapping;


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
  int i;
  int cnt;
  xbool has_error = XFALSE;

  printf("[info] got new client!\n");

  // handshake on VNC version, only support 3.8
  if (has_error == XFALSE && xsocket_write(client_xs, "RFB 003.008\n", 12) == -1) {
    fprintf(stderr, "[error] cannot write to client\n");
    has_error = XTRUE;
  }

  if (has_error == XFALSE && xsocket_read(client_xs, buf, 12) != 12) {
    fprintf(stderr, "[error] cannot read from client\n");
    has_error = XTRUE;
  }

  if (has_error == XFALSE && xcstr_startwith_cstr((char *) buf, "RFB 003.008") == XFALSE) {
    fprintf(stderr, "[failure] client vnc version not supported!\n");
    has_error = XTRUE;
  }

  if (has_error == XFALSE) {
    // tell client the security types
    buf[0] = 1; // 1 security type
    buf[1] = 2; // vnc auth
    xsocket_write(client_xs, buf, 2);
  }

  if (has_error == XFALSE && xsocket_read(client_xs, buf, 1) == -1) {
    printf("[error] cannot read from client\n");
    has_error = XTRUE;
  }

  printf("[info] client choose security type %d\n", (int) buf[0]);

  // make random challenge
  for (i = 0; i < challenge_size; i++) {
    challenge[i] = (unsigned char) (rand() & 0xFF);
  }
  xsocket_write(client_xs, challenge, challenge_size);
  xsocket_read(client_xs, response, challenge_size);

  // calculate the expected response, NOTE: key is padded by null to 8 bytes, if strlen(key) > 8, it is trimmed
  rfbDesKey((unsigned char *) "nopass\0\0", EN0); // TODO configurable key from vnc_proxy_ctl
  for (i = 0; i < challenge_size; i += 8) {
    rfbDes(challenge + i, expected_response + i);
  }
  printf("[info] expected the following bytes:\n");
  print_bytes((char *) expected_response, challenge_size);
  printf("[info] actually got the following bytes:\n");
  print_bytes((char *) response, challenge_size);

  if (memcmp(response, expected_response, challenge_size) != 0) {
    char* failure_message = "authentication failed";
    printf("[info] auth failed\n");

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
    printf("[info] going to write the following error message:\n");
    print_bytes((char *) buf, buf[7] + 8);
    xsocket_write(client_xs, buf, buf[7] + 8);
    has_error = XTRUE;

  } else {
    // try to connect to real server
    xstr vnc_host = xstr_new();
    int vnc_port = 5901;
    xsocket vnc_xs;
    int auth_type_count = 0;
    xbool has_vnc_auth = XFALSE;
    xbool has_none_auth = XFALSE;

    xstr_set_cstr(vnc_host, "166.111.131.34");

    vnc_xs = xsocket_new(vnc_host, vnc_port);
    xsocket_connect(vnc_xs);

    // handshake vnc version
    xsocket_read(vnc_xs, buf, 12);
    xsocket_write(vnc_xs, "RFB 003.008\n", 12);
    
    xsocket_read(vnc_xs, buf, 1);
    auth_type_count = buf[0];
    printf("[info] real VNC server has %d types of auth\n", auth_type_count);
    xsocket_read(vnc_xs, buf, auth_type_count);

    for (i = 0; i < auth_type_count; i++) {
      if (buf[i] == 1) {
        // no auth
        has_none_auth = XTRUE;
      } else if (buf[i] == 2) {
        // vnc auth
        has_vnc_auth = XTRUE;
      }
    }

    if (has_none_auth == XTRUE) {
      // tell server to use none_auth
      xsocket_write(vnc_xs, "\1", 1);
  
    } else if (has_vnc_auth == XTRUE) {
      buf[0] = (unsigned char) 2;
      xsocket_write(vnc_xs, buf, 1);  // tell vnc server: use vnc auth
      xsocket_read(vnc_xs, challenge, challenge_size);

      // TODO use real key for the real vnc server
      rfbDesKey((unsigned char *) "nopass\0\0", EN0);

      for (i = 0; i < challenge_size; i += 8) {
        rfbDes(challenge + i, response + i);
      }

      xsocket_write(vnc_xs, response, challenge_size);  // auth for the real vnc server
      cnt = xsocket_read(vnc_xs, buf, buf_len);

      // check success
      if (buf[3] != 0) {
        // TODO failure
        printf("[info] auth failed on real vnc server\n");
        has_error = XTRUE;
      } else {
        printf("[info] passed auth on real vnc server\n");
      }

      // redirect auth information to client
      xsocket_write(client_xs, buf, cnt);
    }

    if (has_error == XFALSE) {
      // start forwarding
      printf("[info] start forwarding...\n");
      xsocket_shortcut(client_xs, vnc_xs);
      printf("[info] client disconnected\n"); // TODO never reach this line?
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
    fprintf(stderr, "in start_vnc_proxy_server(): failed to init xserver!\n");
    return XFAILURE;
  }
  printf("[info] VNC proxy server running on %s:%d\n", xstr_get_cstr(bind_addr), port);
  ret = xserver_serve(xs);
  xstr_delete(bind_addr);
  return ret;
}


int main(int argc, char* argv[]) {
  xstr bind_addr = xstr_new();
  int port = 5900;
  int i;

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

  // TODO prepare the vnc mapping hash table
//  vnc_mapping = xhash_new();

  return start_vnc_proxy_server(bind_addr, port);
}


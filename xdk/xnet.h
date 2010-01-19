#ifndef XNET_H_
#define XNET_H_

#include "xdef.h"
#include "xstr.h"

/**
  @author
    Santa Zhang

  @file
    xnet.h

  @brief
    Networking utility.
*/

struct xsocket_impl;

/**
  @brief
    Wrapper around network socket.
*/
typedef struct xsocket_impl* xsocket;

/**
  @brief
    Create a new xsocket object.

  @param host
    The address of host machine.
  @param port
    The service port of host machine.

  @return
    A new xsocket object. It is not 'connected' to the target address at creation time, though.
    You can connect the xsocket by xsocket_connect().

  @see
    xsocket_connect
*/
xsocket xsocket_new(xstr host, int port);

/**
  @brief
    Get host address, in c-string.

  @param xs
    The xsocket object containing host address info.

  @return
    The host address, in c-string.
*/
const char* xsocket_get_host_cstr(xsocket xs);

/**
  @brief
    Get host machine's service port.
  
  @param xs
    The xsocket object containing the port number.

  @return
    The port number.
*/
int xsocket_get_port(xsocket xs);

/**
  @brief
    Write data to an xsocket.

  @param xs
    The xsocket to be written to.
  @param data
    Pointer to data buffer.
  @param len
    Length of data to be written, in bytes.

  @return
    If error occurred, -1 will be return. Otherwise the number of bytes written will be returned.
*/
int xsocket_write(xsocket xs, const void* data, int len);

/**
  @brief
    Read data from xsocket.

  @param xs
    The xsocket where new data will be read from.
  @param buf
    The data buffer that will hold input data.
  @param max_len
    Maximum number of bytes to be read.

  @return
    If error occurred, -1 will be return. Otherwise the number of bytes read will be returned.

  @warning
    Make sure the buffer has got enough size!
*/
int xsocket_read(xsocket xs, void* buf, int max_len);


/**
  @brief
    Connect an xsocket to server side.

  @param xs
    The xsocket to be connectted.

  @return
    Whether the connection is successful.
*/
xsuccess xsocket_connect(xsocket xs);

/**
  @brief
    Destroy an xsocket object.

  @param xs
    The xsocket object to be destroyed.
*/
void xsocket_delete(xsocket xs);

/**
  @brief
    Join 2 xsockets, do forwarding between them. This is a blocking call.

  @param xs1
    One of the xsockets.
  @param xs2
    One of the xsockets.
*/
void xsocket_shortcut(xsocket xs1, xsocket xs2);

struct xserver_impl;

/**
  @brief
    Works as server object.
*/
typedef struct xserver_impl* xserver;

/**
  @brief
    Call-back function that handles client requests.

  @param client_xs
    xsocket object that was bound to incoming clients.
  @param args
    Additional arguments for the acceptor fucntion.
*/
typedef void (*xserver_acceptor)(xsocket client_xs, void* args);

/**
  @brief
    Create a new xserver object.

  @param host
    The host address where this xserver will be bound to.
  @param port
    The port number where this xserver will be bound to.
  @param backlog
    A parameter to the 'listen()' function, maximum number of waiting clients. Preferred range is 5 ~ 10.
  @param acceptor
    The call-back function working as acceptor.
  @param serv_count
    How many rounds of service should this server provide. Could be XUNLIMITED, which means it runs forever.
  @param serv_mode
    Could be 'p', 't', or 'b'. 'p' means in a (forked) new process, 't' means in a separate thread (provided by pthread library), 'b' means it is blocking (served in current process, current thread).
  @param args
    Additional arguments for acceptor function.

  @return
    Newly created xserver, all ready 'bind()' and 'listen()'. However, if error occurred, NULL will be returned.

  @warning
    Be careful! You must check if NULL is returned!
*/
xserver xserver_new(xstr host, int port, int backlog, xserver_acceptor acceptor, int serv_count, char serv_mode, void* args);

/**
  @brief
    Start service. The xserver will be self-destryoed after this call, when serv_count is reached.

  @param xs
    The xserver providing service.

  @return
    XSUCCESS if everything goes fine. XFAILURE if something goes wrong (not enough threads/process, etc.).

  @warning
    Remember, xserver will be self-destroyed after this call!
*/
xsuccess xserver_serve(xserver xs);

/**
  @brief
    Get the port on which xserver is serving.

  @param xs
    The xserver object.

  @return
    The xserver object's service port.
*/
int xserver_get_port(xserver xs);

/**
  @brief
    Get c-string representation of xserver's service IP address.

  @param xs
    The xserver object.

  @return
    IP address in c-string, returned by inet_ntoa.
*/
const char* xserver_get_ip_cstr(xserver xs);

#endif  // XNET_H_


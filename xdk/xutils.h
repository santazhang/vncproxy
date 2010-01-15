#ifndef XUTILS_H_
#define XUTILS_H_

/**
  @author
    Santa Zhang

  @file
    xutils.h

  @brief
    Miscellaneous utilities.
*/

#include <netinet/in.h>

#include "xdef.h"
#include "xstr.h"

/**
  @brief
    Convert int value into a char* string.

  @param value
    The int value to be converted.
  @param buf
    The buffer that will contain text representation of the int value.
  @param base
    The numerical base for text representation, in range [2, 36].

  @return
    The char* string containing text representation of the int value.
    Return empty string if error occured.

  @warning
    Make sure buf have enough size!
*/
char* xitoa(int value, char* buf, int base);


/**
  @brief
    Convert long long value into a char* string.

  @param value
    The long long value to be converted.
  @param buf
    The buffer that will contain text representation of the long value.
  @param base
    The numerical base for text representation, in range [2, 36].

  @return
    The char* string containing text representation of the long value.
    Return empty string if error occured.

  @warning
    Make sure buf have enough size!
*/
char* xltoa(long long value, char* buf, int base);

/**
  @brief
    Test if a string starts with another string.

  @param str
    The string to be tested.
  @param head
    The begining of the string.

  @return
    1 if true, 0 if false.
*/
xbool xcstr_startwith_cstr(const char* str, const char* head);

/**
  @brief
    Strips white space around a string.

  @param str
    The string to be stripped.

  @return
    The stripped string.
*/
char* xcstr_strip(char* str);

/**
  @brief
    Convert an IP value into string.

  @param ip
    The IP value.
  @param str
    The string that will contain text representation of the IP value.
    It must have enough size (>= 16).
  
  @return
    -1 if convert failure, otherwise 0.

  @warning
    str must have enough size!
*/
xsuccess xinet_ip2str(int ip, char* str);

/**
  @brief
    Convert text representation of inet address into socket address.

  @param host
    Text representation of inet address.
  @param port
    The port of socket address.
  @param addr
    Socket address to be filled in.

  @return
    -1 if failure, otherwise 0.
*/
xsuccess xinet_get_sockaddr(const char* host, int port, struct sockaddr_in* addr);

/**
  @brief
    Join two segments of file path into a full path.

  @param fullpath
    The output of this function, which will hold the joined path. It will also be normalized, if it is an absolute path.
  @param current_dir
    Current directory, act as front part of the fullpath.
  @param append_dir
    The path that will be appended to current_dir. If it starts with '/',
    things gets a little different: the append_dir will be considered as new fullpath.
*/
void xjoin_path_cstr(xstr fullpath, const char* current_dir, const char* append_dir);

/**
  @brief
    Sleep in milliseconds.

  @param msec
    Time to sleep, in milliseconds.
*/
void xsleep_msec(int msec);

/**
  @brief
    Remove files, (possibily) recursive, like "rm -rf".

  @param path
    Fullpath to the file or directory.

  @return
    Whether the action is successful.

*/
xsuccess xfilesystem_rmrf(const char* path);

/**
  @brief
    Check if an filesystem item identified by 'path' is a file.

  @param path
    Path to the filesystem item. (TODO check if it could be relative)
  @param optional_succ
    A pointer to a xsuccess value, if internal error occured, it will be set to XFAILURE. It is optional, to disable it, assign NULL.

  @return
    Whether the filesystem item is a file.
*/
xbool xfilesystem_is_file(const char* path, xsuccess* optional_succ);

/**
  @brief
    Check if an filesystem item identified by 'path' is a dir.

  @param path
    Path to the filesystem item. (TODO check if it could be relative)
  @param optional_succ
    A pointer to a xsuccess value, if internal error occured, it will be set to XFAILURE. It is optional, to disable it, assign NULL.

  @return
    Whether the filesystem item is a dir.
*/
xbool xfilesystem_is_dir(const char* path, xsuccess* optional_succ);

/**
  @brief
    Cdup for an normalized path.

  @param norm_path
    The normalized path, like '/a/b/c/d'. It will be changed, like 'cdup' action.
    It could be '/', in which case nothing will be changed.

  @return
    Whether the action is succesful.
*/
xsuccess xfilesystem_path_cdup(xstr norm_path);

/**
  @brief
    Normalize an absolute path.

  @param abs_path
    The absolute path to be normalized.
  @param norm_path
    The xstr that will hold normalized result.

  @return
    Whether the process is successful.
*/
xsuccess xfilesystem_normalize_abs_path(const char* abs_path, xstr norm_path);

#endif  // XUTILS_H_


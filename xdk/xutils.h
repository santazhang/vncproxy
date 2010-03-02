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

#include <stdio.h>
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
    Split host:port representation.

  @param host_port
    The host:port representation, in c-string.
  @param host
    The host name.
  @param port
    The port number. If port is not given in the host_port, its value is untouched.

  @return
    Whether the host_port is successfully splitted, i.e, if the host_port has correct grammar.
*/
xsuccess xinet_split_host_port(const char* host_port, xstr host, int* port);

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
    Check if filesystem entry exists.

  @param path
    Path to the filesystem entry.

  @return
    Whether the filesystem entry exists.
*/
xbool xfilesystem_exists(const char* path);

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


/**
  @brief
    Convert file size in c-string form to int value. Such as "10M" -> 10 * 1024 * 1024.

  @param size_cstr
    File size in cstr. Units are: G, g, M, m, K, k, B, b. If no units given, b (bytes) will be assumed.
    There could be space between unit and the size value, such as "10 Mb".

  @return
    File size on succcess, or -1 will be returned if failed.
*/
long xfilesystem_parse_filesize(const char* size_cstr);

/**
  @brief
    Get hash value of a c-string.

  @param key
    Pointer to the c-string.

  @return
    The hash value of the c-string.
*/
int xhash_hash_cstr(void* key);


/**
  @brief
    Get hash value of an x-string.

  @param key
    Pointer to the x-string.

  @return
    The hash value of the x-string.
*/
int xhash_hash_xstr(void* key);


/**
  @brief
    A wrapper around xstr_eql(), prevent gcc from warning.

  @param key1
    A key to be compared.
  @param key2
    A key to be compared.

  @return
    Whether the 2 xstr is equal. 
*/
xbool xhash_eql_xstr(void* key1, void* key2);


/**
  @brief
    Get a line from file pointer.

  @param fp
    The file pointer.
  @param line
    Where the new line will be stored. This param is cleared every time. The EOL character (\\n, \\r\\n) is trimmed.

  @return
    If new lines are read, XSUCCESS will be returned. Other wise (including EOF) XFAILURE is returned.
*/
xsuccess xgetline_fp(FILE* fp, xstr line);

#endif  // XUTILS_H_


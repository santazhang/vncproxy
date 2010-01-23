#ifndef XSTR_H_
#define XSTR_H_

#include "xdef.h"

/**
  @author
    Santa Zhang

  @file
    xstr.h

  @brief
    String implementation.
*/

// Implementation is hidden in .c file.
struct xstr_impl;

/**
  @brief
    Simple string implementation.
*/
typedef struct xstr_impl* xstr;

/**
  @brief
    Create a new xstr object.

  @return
    New xstr object, already initialized.
*/
xstr xstr_new();

/**
  @brief
    Destroy an xstr object.

  @param xs
    The xstr to be destroyed.
*/
void xstr_delete(xstr xs);

/**
  @brief
    Get c-string from xstr.

  @param xs
    The xstr from which c-string will be extracted.

  @return
    The extracted c-string.

  @warning
    Do not modify the content of returned c-string!
*/
const char* xstr_get_cstr(xstr xs);

/**
  @brief
    Set content of xstr to a c-string.

  @param xs
    The xstr to be changed.
  @param cs
    The new content, has c-string type.
*/
void xstr_set_cstr(xstr xs, const char* cs);

/**
  @brief
    Get length of an xstr.

  @param xs
    The xstr whose length we care about.

  @return
    Length of the xstr.
*/
int xstr_len(xstr xs);

/**
  @brief
    This is printf like function, except that it prints into an xstr. It only supports some basic formats.

  @param xs
    The xstr that will be printed into. New data will be appended to the end of xs.
  @param fmt
    The format string like in printf. Only support \%d, \%c, \%s.

  @return
    On error (format error, etc.) return -1.
    Otherwise the number of chars appended to xs.
*/
int xstr_printf(xstr xs, const char* fmt, ...);

/**
  @brief
    Append a char to xstr.

  @param xs
    The xstr where new char will be appended to.
  @param ch
    The char to be appended. If ch == '\\0', this function will do nothing.
*/
void xstr_append_char(xstr xs, char ch);

/**
  @brief
    Append a c-string to xstr.

  @param xs
    The xstr where new c-string will be appended to.
  @param cs
    The c-string to be appended.
*/
void xstr_append_cstr(xstr xs, const char* cs);

/**
  @brief
    Test if an xstr starts with a c-string.

  @param xs
    The xstring to be tested.
  @param head
    The cstring which might be the head.

  @return
    XTRUE or XFALSE.
*/
xbool xstr_startwith_cstr(xstr xs, const char* head);

/**
  @brief
    Deeply copy an xstr object.

  @param orig
    The original xstr object.

  @return
    A deeply copied xstr object, it should be destroyed by xstr_delete later.
*/
xstr xstr_copy(xstr orig);

/**
  @brief
    Return the last char in xstr.

  @param xs
    The xstr object.

  @return
    Will return '\\0' if xstr has length 0.
*/
char xstr_last_char(xstr xs);

/**
  @brief
    Test if 2 xstr is equal.

  @param xstr1
    The xstr to be tested.
  @param xstr2
    The xstr to be tested.

  @return
    Whether the 2 xstr is equal.
*/
xbool xstr_eql(xstr xstr1, xstr xstr2);

/**
  @brief
    Strip an xstr.
  
  @param xs
    The xstr object to be stripped.
  @param strip_set
    A c-string containing all the chars to be stripped.
*/
void xstr_strip(xstr xs, char* strip_set);

#endif  // XSTR_H_

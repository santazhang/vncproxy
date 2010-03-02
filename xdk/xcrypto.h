#ifndef XCRYPTO_H_
#define XCRYPTO_H_

#include "xdef.h"

/**
  @author
    Santa Zhang

  @file
    xcrypto.h

  @brief
    A wrapper for 3rd party crypto functions. Currently included SHA-1, MD5
*/


/**
  @brief
    Wrapper around 3rd party md5 lib.
*/
struct xmd5_impl;

/**
  @brief
    MD5 object for user.
*/
typedef struct xmd5_impl* xmd5;

/**
  @brief
    Create a new md5 object.

  @return
    An initialized md5 object.
*/
xmd5 xmd5_new();

/**
  @brief
    Feed data into md5 object, update md5 sum value.

  @param xm
    The md5 which accepts data.
  @param data
    Input data.
  @param size
    Size of input data array, in bytes.
*/
void xmd5_feed(xmd5 xm, const void* data, int size);

/**
  @brief
    Retrieve data from md5 object.

  @param xm
    The md5 object.
  @param result
    Target where retrieved info will be saved into.
    Output data is 16 bytes long.

  @warning
    Make sure result has got enough memory size!
*/
void xmd5_result(xmd5 xm, unsigned char* result);

/**
  @brief
    Destroy an md5 object, release allocated memory resource.

  @param xm
    The md5 object to be destroyed.
*/
void xmd5_delete(xmd5 xm);

/**
  @brief
    Convert md5 value to cstr.

  @param md5
    Input value, the md5 hash sum.
  @param md5_cstr
    Output value, which will contain the text representation of the md5 value.

  @warning
    Make sure md5_cstr has got enough size (at least 33 bytes)!
*/
void xcrypto_md5_cstr(const unsigned char* md5, char* md5_cstr);


/**
  @brief
    Wrapper around 3rd party sha1 lib.
*/
struct xsha1_impl;

/**
  @brief
    The SHA1 calculator object.
*/
typedef struct xsha1_impl* xsha1;

/**
  @brief
    Create a new SHA1 calculator.

  @return
    Initialized SHA1 calculator object.
*/
xsha1 xsha1_new();

/**
  @brief
    Feed data into SHA1 calculator.

  @param xsh
    The SHA1 calculator.
  @param data
    The data which will be feed into SHA1 calculator.
  @param size
    The size of input data, in bytes.
*/
void xsha1_feed(xsha1 xsh, void* data, int size);

/**
  @brief
    Retrieve result from SHA1 calculator.

  @param xsh
    The SHA1 calculator.
  @param result
    Pointer to output array. It is an unsigned int array with 5 elements.

  @return
    XSUCCESS if calculation was succesfull. XFAILURE if failed to calculate SHA1 value (possible data corruption).

  @warning
    Make sure result has got enough memory size!
*/
xsuccess xsha1_result(xsha1 xsh, unsigned int* result);

/**
  @brief
    Destroy SHA1 calculator.

  @param xsh
    The SHA1 calculator to be destroyed.
*/
void xsha1_delete(xsha1 xsh);

#endif // XCRYPTO_H_


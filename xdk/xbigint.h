#ifndef XBIGINT_H_
#define XBIGINT_H_

#include "xdef.h"
#include "xstr.h"

/**
  @author
    Santa Zhang

  @file
    xbigint.c

  @brief
    Unlimited integer implementation.
*/

/**
  @brief
    Hidden implementation of xbigint.
*/
struct xbigint_impl;

/**
  @brief
    Interface of xbigint.
*/
typedef struct xbigint_impl* xbigint;

/**
  @brief
    Create a new xbigint object, with value = 0.

  @return
    A new xbigint with value = 0;
*/
xbigint xbigint_new();

/**
  @brief
    Destroy an xbigint object.
*/
void xbigint_delete(xbigint xbi);

/**
  @brief
    Set the xbigint's value from an int value.

  @param xbi
    The xbigint to be assigned.
  @param value
    The int value.
*/
void xbigint_set_int(xbigint xbi, int value);

/**
  @brief
    Set the xbigint's value to an double value.

  Please note that double value has limited precision (about 18 digits).
  The xbigint value will be rounded toward the nearest int value. <br>
  e.g.: <br>
  0.1 -> 0 <br>
  0.51 -> 1 <br>
  -0.3 -> 0 <br>
  -0.9 -> -1 <br>

  If the double is NaN or Inf, then the bigint will be set to 0, and XFAILURE will be returned.
  Otherwise the assignment will go correctly and XSUCCESS will be returned.

  @param xbi
    The xbigint to be assigned.
  @param value
    The double value to be assigned.

  @return
    Whether the assignement is succesful.
*/
xsuccess xbigint_set_double(xbigint xbi, double value);

/**
  @brief
    Set the bigint to a double value in text.

  The value will be rounded towards the nearest int, if necessary. <br>
  e.g.: <br>
  0.1 -> 0 <br>
  0.51 -> 1 <br>
  -0.3 -> 0 <br>
  -0.9 -> -1 <br>

  If the c-string does not have valid format, XFAILURE will be returned, and the xbigint will be set to 0. <br>

  Example of supported strings: <br>
  '0', '+0', '-0', '0.0', '-234234', '0.3', '3e3'

  The format of the string is:
  (1) a number is represented in 3 parts: fixed, friction, mantissa. <br>
  (2) fixed is required, and could be preceded by an optional sign ('+', '-'),
      space is NOT allowed between the sign and the fixed part. <br>
  (3) friction is optional. if it exists, the fixed part and friction
      part MUST be separated by a period, space is NOT allowed. <br>
  (4) mantissa is optional. if it exists, it must be preceded by a 'e' or 'E'.
      the mantiss could have an optional sign, and space is NOT allowed. <br>

  Precise grammar: <br>

  value ::= fixed friction_opt mantissa_opt ; <br>
  <br>

  fixed ::= ('+'|'-'|'')[0-9]+ ; <br>
  <br>

  friction_opt ::= '.'[0-9]+ <br>
                | '' <br>
                ; <br>
  <br>

  mantissa_opt ::= ('e'|'E')('+'|'-'|'')[0-9]+ <br>
                | '' <br>
                ; <br>

  @param xbi
    The xbigint to be assigned.
  @param cstr
    The text representation of the value.

  @return
    Whether the assign process is successful.
*/
xsuccess xbigint_set_cstr(xbigint xbi, char* cstr);

/**
  @brief
    Get int value from xbigint object.

  @param xbi
    The xbigint which has got the value to be converted.
  @param value
    The returned int value.

  @return
    Whether the assignment is successful. XFAILURE will be returned, if overflow occurred.

  @warning
    Make sure you have checked the return value, which tells you whether overflow occurred.
*/
xsuccess xbigint_get_int(xbigint xbi, int* value);

/**
  @brief
    Get double value from xbigint object.

  @param xbi
    The xbigint which has got the value to be converted.
  @param value
    The returned double value.

  @return
    Whether the assignment is successful. XFAILURE will be returned, if overflow occurred.

  @warning
    Make sure you have checked the return value, which tells you whether overflow occurred.
*/
xsuccess xbigint_get_double(xbigint xbi, double* value);

/**
  @brief
    Get xstr representation of xbigint object.

  @param xbi
    The xbigint which has got the value to be converted.
  @param xs
    The xstr object that will hold returned result.

  @return
    Always return XSUCCESS;
*/
xsuccess xbigint_get_xstr(xbigint xbi, xstr xs);

/**
  @brief
    Copy an xbigint.

  @param xbi
    The xbigint to be copied.

  @return
    A newly copied xbigint object.
*/
xbigint xbigint_copy(xbigint xbi);

/**
  @brief
    Change the sign of xbigint. If 0 is given, nothing will be changed.

  @param xbi
    The xbigint whose sign will be changed.
*/
void xbigint_change_sign(xbigint xbi);

/**
  @brief
    Get the sign of an xbigint object.

  @param xbi
    The xbigint whose sign will be extracted.

  @return
    0 if xbi == 0, -1 if xbi < 0, 1 if xbi > 0.
*/
int xbigint_get_sign(xbigint xbi);

/**
  @brief
    Set an xbigint to 0.

  @param xbi
    The xbigint object.
*/
void xbigint_set_zero(xbigint xbi);

/**
  @brief
    Add an xbigint by another xbigint's value.

  @param xbi_dest
    The xbigint which will be added (changed).
  @param xbi_src
    The xbigint whose value will be added to xbi_dest.
*/
void xbigint_add(xbigint xbi_dest, xbigint xbi_src);

/**
  @brief
    Add an xbigint by an int value.

  @param xbi
    The xbigint which will be added (changed).
  @param value
    The int value which will be added to xbi.
*/
void xbigint_add_int(xbigint xbi, int value);

/**
  @brief
    Subtract an xbigint by another xbigint's value.

  @param xbi_dest
    The xbigint which will be subtracted (changed).
  @param xbi_src
    The xbigint whose value will be subtracted from xbi_dest.
*/
void xbigint_sub(xbigint xbi_dest, xbigint xbi_src);

/**
  @brief
    Subtract an xbigint by an int value.

  @param xbi
    The xbigint which will be subtracted (changed).
  @param value
    The int value which will be subtracted from xbi.
*/
void xbigint_sub_int(xbigint xbi, int value);

/**
  @brief
    Multiply an xbigint by another xbigint's value.

  @param xbi_dest
    The xbigint which will be multiplied (changed).
  @param xbi_src
    The xbigint whose value will be multiplied to xbi_dest.
*/
void xbigint_mul(xbigint xbi_dest, xbigint xbi_src);

/**
  @brief
    Multiply an xbigint by an int value.

  @param xbi
    The xbigint which will be multiplied (changed).
  @param value
    The int value which will be multiplied to xbi.
*/
void xbigint_mul_int(xbigint xbi, int value);

/**
  @brief
    Divide an xbigint by an int value.

  @param xbi
    The xbigint which will be divided (changed).
  @param value
    The int value which will be divided from xbi.

  @return
    XFAILURE if value == 0, otherwise XSUCCESS will be returned.
*/
xsuccess xbigint_div_int(xbigint xbi, int value);

/**
  @brief
    Modular divide an xbigint by an int value.

  @param xbi
    The xbigint which will be modular divided. It is not changed
  @param value
    The int value which will be modular divided from xbi.
  @param result
    Pointer to an int value which will contain the modular division result.

  @return
    XFAILURE if value == 0. Otherwise XSUCCESS is returned.
*/
xsuccess xbigint_mod_int(xbigint xbi, int value, int* result);

/**
  @brief
    Compare 2 xbigint objects.

  @param xbi1
    The xbigint to be compared.
  @param xbi2
    The xbigint to be compared.

  @return
    0 if xbi1 == xbi2, -1 if xbi1 < xbi2, 1 if xbi1 > xbi2.
*/
int xbigint_cmp(xbigint xbi1, xbigint xbi2);

#endif  // XBIGINT_H_


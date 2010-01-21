#ifndef XDK_XOPTION_H_
#define XDK_XOPTION_H_

#include "xdef.h"

/**
  @author
    Santa Zhang
  @brief
    Helper for parsing command line options.
  @file
    xoption.h
*/

struct xoption_impl;

/**
  @brief
    Interface of xoption, which is exposed to users.
*/
typedef struct xoption_impl* xoption;

/**
  @brief
    Create a new xoption.

  @return
    A new xoption object.
*/
xoption xoption_new();

/**
  @brief
    Try to parse the command line args sent to main().

  @param xopt
    The xoption which will hold the results.
  @param argc
    The number of args, as in main().
  @param argv
    The array of args, as in main().

  @return
    Whether the result is successfully parsed.
*/
xsuccess xoption_parse(xoption xopt, int argc, char* argv[]);

/**
  @brief
    Check whether a certain option was given.

  @param xopt
    The xoption object where the parameter will be retrieved.
  @param name
    The name of option, could be either short or long name.

  @return
    Whether the option exists.
*/
xbool xoption_has(xoption xopt, const char* name);

/**
  @brief
    Get values of a certain option.

  @param xopt
    The xoption object where option value will be retrieved.
  @param name
    The name of option, could be either short or long name.

  @return
    A point to the option values.
    If NULL was given, the list of ordinary (non-option) command line args will be returned.<br>
    e.g.: for "ls -al /home", calling xoption_get(xopt, NULL) will return [ls, /home]
*/
const char* xoption_get(xoption xopt, const char* name);

/**
  @brief
    Get the number of arguments of a certain option name.

  @param xopt
    The xoption object where option value will be retrieved.
  @param name
    The name of option, could be either short or long name.

  @return
    -1 if option not found, 0 if option does not have parameter, otherwise the length of array from xoption_get() is returned.
*/
int xoption_get_size(xoption xopt, const char* name);

/**
  @brief
    Destory the xoption object.

  @param xopt
    The xoption object to be destroyed.
*/
void xoption_delete(xoption xopt);

#endif  // #ifdef XDK_XOPTION_H_


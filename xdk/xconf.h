#ifndef XDK_XCONF_H_
#define XDK_XCONF_H_

#include "xdef.h"
#include "xstr.h"
#include "xhash.h"

/**
  @brief
    Utility for parsing configuration files.

  @file
    xconf.h

  @author
    Santa Zhang
*/

struct xconf_impl;

/**
  @brief
    Interface of xconf that is exposed to users.
*/
typedef struct xconf_impl* xconf;

/**
  @brief
    Create a new xconf object.
*/
xconf xconf_new();

/**
  @brief
    Load configs from file.

  @param xcf
    The xconf object to store the configs.
  @param fname
    The name of config file.

  @return
    Whether the configs are loaded successfully.
*/
xsuccess xconf_load(xconf xcf, const char* fname);

/**
  @brief
    Save configs to a file.

  @param xcf
    The xconf object containing the configs.
  @param fname
    The name of the config file.

  @return
    Whether the configs are succesfully saved.
*/
xsuccess xconf_save(xconf xcf, const char* fname);

/**
  @brief
    Get a section in the configs.

  @param xcf
    The xconf object.
  @param section_name
    The name of the section. If NULL is given, the default section will be used.

  @return
    NULL if no such section is found. Otherwise an xhash of that section is returned.
*/
xhash xconf_get_section(xconf xcf, const char* section_name);

/**
  @brief
    Get a value from a certain section.

  @param xcf
    The xconf object.
  @param section_name
    The name of the section. If NULL is given, the default section will be used.
  @param item_name
    The name of that item.

  @return
    NULL if no such section is found. Otherwise the value of that item is returned.
*/
xstr xconf_get_value(xconf xcf, const char* section_name, const char* item_name);

/**
  @brief
    Set value of a certain value.

  @param xcf
    The xconf object.
  @param section_name
    The name of the section. If NULL is given, the default section will be used.
  @param item_name
    The name of the item.
  @param value
    The new value of the item. If it is NULL, the item will be removed.

  @return
    Whether the operation is successfull. If section is not found, XFAILURE is returned.
*/
xsuccess xconf_set_value(xconf xcf, const char* section_name, const char* item_name, const char* value);

/**
  @brief
    Destroy an xconf object.

  @param xcf
    The xconf object to be destroyed.
*/
void xconf_delete(xconf xcf);

#endif  // #ifndef XDK_XCONF_H_


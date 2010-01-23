#include "xsys.h"

#ifdef WIN32_
const char xsys_fs_sep_char = '\\';
const char* xsys_fs_sep_cstr = "\\";
#else
const char xsys_fs_sep_char = '/';
const char* xsys_fs_sep_cstr = "/";
#endif  // #ifdef WIN32


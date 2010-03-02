#include <string.h>

#include "xoption.h"

#include "xmemory.h"
#include "xstr.h"
#include "xvec.h"
#include "xhash.h"
#include "xutils.h"
#include "xlog.h"
#include "xconf.h"

/**
  @brief
    Implementation of xoption, hidden from users.
*/
struct xoption_impl {
  xvec leftover;  ///< @brief The ordinary command line args that are not part of an option.
  xhash opt;  ///< @brief The hash table from option name to option values.
  xhash optlen; ///< @brief The length of options.
};

static void leftover_free(void* ptr) {
  // do nothing, since values are given from argv, which is const
}

static xbool hash_str_eql(void* key1, void* key2) {
  if (strcmp((char *) key1, (char *) key2) == 0) {
    return XTRUE;
  } else {
    return XFALSE;
  }
}

static void opt_free(void* key, void* value) {
  // key is copy of char* need to be free'd, but they are left to be free'd in opt_free (same copy of pointer)

  // NOTE The xfree job is left to optlen_free, instead of this function.
  // This is caused by short options like "-h", which have key 'h', but don't have value.
  // In this case, xopt->opt hash table does not have an entry for 'h',
  // but xopt->optlen hash table entry ("h", 0). This means we can always xfree
  // the pointer to "h", from xopt->optlen hash table. So the key is left to be xfree'd in xoptlen_free.
  // See add_short_option for details.
  
  // value is pointer to const c-string, so they don't need to be free'd
}

static void optlen_free(void* key, void* value) {
  xfree(key); // key is copy of char*, need to be free'd
  xfree(value); // value is int*, need to be free'd
}

xoption xoption_new() {
  xoption xopt = xmalloc_ty(1, struct xoption_impl);
  xopt->leftover = xvec_new(leftover_free);
  xopt->opt = xhash_new(xhash_hash_cstr, hash_str_eql, opt_free);
  xopt->optlen = xhash_new(xhash_hash_cstr, hash_str_eql, optlen_free);
  return xopt;
}

static void add_long_option(xoption xopt, char* arg) {
  int i;
  int split_idx = -1;
  int* len = xmalloc_ty(1, int);
  char* key = xmalloc_ty(strlen(arg), char);
  for (i = 0; arg[i] != '\0'; i++) {
    if (arg[i] == '=') {
      split_idx = i;
      break;
    } else {
      key[i] = arg[i];
    }
  }
  key[i] = '\0';
  if (split_idx < 0) {
    // no value
    *len = 0;
    xhash_put(xopt->optlen, key, len);
  } else {
    // has value
    char* val = xmalloc_ty(strlen(arg), char);  // no need to alloc strlen(arg) + 1, since it will not be fully used
    *len = 1;
    xhash_put(xopt->optlen, key, len);
    strcpy(val, arg + split_idx + 1);
    xhash_put(xopt->opt, key, val);
  }
}

static void add_short_option(xoption xopt, char key_ch, char* value) {
  char* key = xmalloc_ty(2, char);
  int* len = xmalloc_ty(1, int);
  key[0] = key_ch;
  key[1] = '\0';
  if (value != NULL) {
    *len = 1;
    xhash_put(xopt->opt, key, value);
  } else {
    *len = 0;
    // NOTE (key,value) pair is not put in xopt->opt in this case,
    // but they will always be putted into xopt->optlen
  }
  xhash_put(xopt->optlen, key, len);
}

static void add_grouped_short_option(xoption xopt, char* keys) {
  int i;
  for (i = 0; keys[i] != '\0'; i++) {
    add_short_option(xopt, keys[i], NULL);
  }
}

xsuccess xoption_parse(xoption xopt, int argc, char* argv[]) {
  xsuccess ret = XSUCCESS;
  int i;
  for (i = 0; i < argc; i++) {
    if (xcstr_startwith_cstr(argv[i], "--") == XTRUE) {
      add_long_option(xopt, argv[i] + 2);
    } else if (xcstr_startwith_cstr(argv[i], "-") == XTRUE) {
      if (strlen(argv[i]) > 2) {
        // grouped, like 'al' in 'ls -al'
        add_grouped_short_option(xopt, argv[i] + 1);
      } else {
        if (i + 1 < argc && xcstr_startwith_cstr(argv[i + 1], "-") == XFALSE) {
          add_short_option(xopt, argv[i][1], argv[i + 1]);
        } else {
          add_short_option(xopt, argv[i][1], NULL);
        }
      }
    } else {
      // ordinary command line args
      xvec_push_back(xopt->leftover, argv[i]);
    }
  }
  return ret;
}

// helper for converting xconf to xoption
static xbool xconf_section_visitor(void* key, void* value, void* args) {
  xoption xopt = (xoption) args;
  xstr xkey = (xstr) key;
  xstr xvalue = (xstr) value;
  if (xoption_get(xopt, xstr_get_cstr(key))) {
    // duplicate key, use command line args
    xlog_warning("configuration '%s' already provided on command line, the value provided in configuration file will be ignored!\n", xstr_get_cstr(key));
  } else {
    // adapted from "add_long_option"
    int* len = xmalloc_ty(1, int);
    char* ckey = xmalloc_ty(xstr_len(xkey) + 1, char);
    char* cvalue = xmalloc_ty(xstr_len(xvalue) + 1, char);
    *len = 1;
    strcpy(ckey, xstr_get_cstr(xkey));
    strcpy(cvalue, xstr_get_cstr(xvalue));
    xhash_put(xopt->optlen, ckey, len);
    xhash_put(xopt->opt, ckey, cvalue);
  }
  return XTRUE;
}

static xsuccess load_xconf_into_xoption(xoption xopt, const char* fname) {
  xsuccess ret;
  xconf xcf = xconf_new();
  xlog_info("loading configuration from file '%s'\n", fname);
  ret = xconf_load(xcf, fname);
  if (ret == XSUCCESS) {
    xhash xh = xconf_get_section(xcf, NULL);
    xhash_visit(xh, xconf_section_visitor, xopt);
  } else {
    xlog_error("configuration file '%s' not found\n", fname);
  }
  xconf_delete(xcf);
  return ret;
}

xsuccess xoption_parse_with_xconf(xoption xopt, int argc, char* argv[]) {
  xsuccess ret = xoption_parse(xopt, argc, argv);
  int i;
  if (ret == XSUCCESS) {
    for (i = 0; i < xvec_size(xopt->leftover) && ret == XSUCCESS; i++) {
      const char* opt_name = (const char *) xvec_get(xopt->leftover, i);
      if (opt_name[0] == '@') {
        xvec_remove(xopt->leftover, i); // the @blah is replaced by configs inside config file
        ret = load_xconf_into_xoption(xopt, opt_name + 1);
      }
    }
    /*
    // show debug info
    for (i = 0; i < xvec_size(xopt->leftover); i++) {
      const char* opt_name = (const char *) xvec_get(xopt->leftover, i);
      printf("%s\n", opt_name);
    }
    */
  }
  return ret;
}

const char* xoption_get(xoption xopt, const char* name) {
  return xhash_get(xopt->opt, (void *) name);
}

xvec xoption_get_array(xoption xopt, const char* name) {
  if (name == NULL) {
    return xopt->leftover;
  } else {
    return NULL;
  }
}

int xoption_get_size(xoption xopt, const char* name) {
  if (name != NULL) {
    int* p_len = xhash_get(xopt->optlen, (void *) name);
    if (p_len == NULL) {
      return -1;
    } else {
      return *p_len;
    }
  } else {
    return xvec_size(xopt->leftover);
  }
}

xbool xoption_has(xoption xopt, const char* name) {
  if (xoption_get_size(xopt, name) < 0) {
    return XFALSE;
  } else {
    return XTRUE;
  }
}

void xoption_delete(xoption xopt) {
  xvec_delete(xopt->leftover);
  xhash_delete(xopt->opt);
  xhash_delete(xopt->optlen);
  xfree(xopt);
}


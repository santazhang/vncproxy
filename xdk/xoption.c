#include <string.h>

#include "xoption.h"

#include "xmemory.h"
#include "xstr.h"
#include "xvec.h"
#include "xhash.h"
#include "xutils.h"

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
  // do nothing, since ptr is char*
}

static xbool hash_str_eql(void* key1, void* key2) {
  if (strcmp((char *) key1, (char *) key2) == 0) {
    return XTRUE;
  } else {
    return XFALSE;
  }
}

static void opt_free(void* key, void* value) {
  xfree(key); // key is copy of char*, need to be free'd
  // value is char*, no need to free
}

static void optlen_free(void* key, void* value) {
  // key is copy of char* need to be free'd, but already free'd once in opt_free
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
    int* len = xmalloc_ty(1, int);
    *len = 0;
    xhash_put(xopt->optlen, key, len);
    xhash_put(xopt->opt, key, NULL);
  } else {
    // has value
    char* val = xmalloc_ty(strlen(arg), char);
    int* len = xmalloc_ty(1, int);
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

const char* xoption_get(xoption xopt, const char* name) {
  return xhash_get(xopt->opt, (void *) name);
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


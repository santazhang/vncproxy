#include <string.h>
#include <stdarg.h>

#include "xstr.h"
#include "xmemory.h"
#include "xutils.h"

/**
  @brief
    Protected implementation of xstr.
*/
struct xstr_impl {
  char* str;  ///< @brief Pointer to c-string.
  int len; ///< @brief Length of the xstr.
  int mem_size; ///< @brief Memory usage.
};

xstr xstr_new() {
  xstr xs = xmalloc_ty(1, struct xstr_impl);
  xs->mem_size = 16;
  xs->str = xmalloc_ty(xs->mem_size, char);
  xs->str[0] = '\0';
  xs->len = 0;
  return xs;
}

void xstr_delete(xstr xs) {
  xfree(xs->str);
  xfree(xs);
}

const char* xstr_get_cstr(xstr xs) {
  return xs->str;
}

/**
  @brief
    Ensure that the xstr has got enough memroy.

  @param xstr
    The xstr that requires enough memory.
  @param mem_size
    The minimum required memroy size.
*/
static void ensure_mem_size(xstr xs, int mem_size) {
  if (mem_size > xs->mem_size) {

    // Note that we allocated mem_size * 2, which will reduce
    // calls to this function.
    // Think about the case where many chars are appended to an xstr.
    xs->mem_size = mem_size * 2;
    xs->str = xrealloc(xs->str, xs->mem_size);
  }
}

void xstr_set_cstr(xstr xs, const char* cs) {
  int cs_len = strlen(cs);
  xs->len = cs_len;

  // note the "+1", because of trailing '\0'
  ensure_mem_size(xs, xs->len + 1);
  strcpy(xs->str, cs);
}

int xstr_len(xstr xs) {
  return xs->len;
}

void xstr_append_char(xstr xs, char ch) {
  if (ch != '\0') {
    ensure_mem_size(xs, xs->len + 1);
    xs->str[xs->len] = ch;
    xs->len++;
    xs->str[xs->len] = '\0';
  }
}

// for the case where cs_len is pre-calculated
// prevent calling strlen(cs) more than once
static void xstr_append_cstr_len_precalculated(xstr xs, const char* cs, int cs_len) {
  if (cs_len > 0) {

    // *** do not modify "+16"! make sure there is *enough* memory
    ensure_mem_size(xs, xs->len + cs_len + 16);

    // Note: we did not use strcat(xs->str, ...), but used strcpy(xs->str + xs->len),
    // because we know exactly where new string should be appended, rather than scanning xs->str
    // to find the end point. This might enhance performance.
    strcpy(xs->str + xs->len, cs);
    xs->len += cs_len;
  }
}

void xstr_append_cstr(xstr xs, const char* cs) {
  int cs_len = strlen(cs);
  xstr_append_cstr_len_precalculated(xs, cs, cs_len);
}


int xstr_printf(xstr xs, const char* fmt, ...) {
  va_list argp;
  const char* p;
  int cnt = 0;
  long long ldval;
  int ival;
  char* sval;
  char cval;
  char buf[32]; // for xitoa & xltoa
  int str_len;

  va_start(argp, fmt);

  for (p = fmt; *p != '\0' && cnt >= 0; p++) {
    if (*p != '%') {
      cnt++;
      xstr_append_char(xs, *p);
      continue;
    }

    // otherwise, found an '%', go to next char
    p++;
    switch(*p) {
    case 'c':
      // Note, we use 'int' here instead of 'char', because 'char' type uses same number of bytes as 'int' on stack
      cval = va_arg(argp, int);
      xstr_append_char(xs, cval);
      cnt++;
      break;

    case 'l':
      p++;
      if (*p == 'd') {
        // %ld
        ldval = va_arg(argp, long long);
        xltoa(ldval, buf, 10);
        str_len = strlen(buf);
        cnt += str_len;
        xstr_append_cstr_len_precalculated(xs, buf, str_len);
      } else {
        xstr_append_cstr(xs, "** FORMAT ERROR ***");
        cnt = -1;
      }
      break;

    case 'd':
      ival = va_arg(argp, int);
      xitoa(ival, buf, 10);
      str_len = strlen(buf);
      cnt += str_len;
      xstr_append_cstr_len_precalculated(xs, buf, str_len);
      break;

    case 's':
      sval = va_arg(argp, char *);
      str_len = strlen(sval);
      cnt += str_len;
      xstr_append_cstr_len_precalculated(xs, sval, str_len);
      break;

    case '%':
      cnt++;
      break;

    default:
      xstr_append_cstr(xs, "** FORMAT ERROR ***");
      cnt = -1;
      break;
    }
  }
  va_end(argp);
  return cnt;
}

xbool xstr_startwith_cstr(xstr xs, const char* head) {
  int i;
  for (i = 0; xs->str[i] != '\0' && head[i] != '\0'; i++) {
    if (xs->str[i] != head[i])
      return XFALSE;
  }
  return head[i] == '\0';
}

xstr xstr_copy(xstr orig) {
  xstr new_str = xstr_new();
  xstr_set_cstr(new_str, xstr_get_cstr(orig));
  return new_str;
}

char xstr_last_char(xstr xs) {
  if (xs->len == 0) {
    return '\0';
  } else {
    return xs->str[xs->len - 1];
  }
}

xbool xstr_eql(xstr xstr1, xstr xstr2) {
  if (strcmp(xstr_get_cstr(xstr1), xstr_get_cstr(xstr2)) == 0) {
    return XTRUE;
  } else {
    return XFALSE;
  }
}


void xstr_strip(xstr xs, char* strip_set) {
  int new_begin = 0;
  int new_end = xs->len;  // exclusive end point
  int i;
  xbool should_strip;
  char* stripped_cstr;

  while (new_begin < new_end) {
    should_strip = XFALSE;
    for (i = 0; strip_set[i] != '\0'; i++) {
      if (xs->str[new_begin] == strip_set[i]) {
        should_strip = XTRUE;
        break;
      }
    }
    if (should_strip == XTRUE) {
      new_begin++;
    } else {
      break;
    }
  }

  while (new_begin < new_end) {
    should_strip = XFALSE;
    for (i = 0; strip_set[i] != '\0'; i++) {
      if (xs->str[new_end - 1] == strip_set[i]) {
        should_strip = XTRUE;
        break;
      }
    }
    if (should_strip == XTRUE) {
      new_end--;
    } else {
      break;
    }
  }
  
  stripped_cstr = xmalloc_ty(new_end - new_begin + 1, char);
  memcpy(stripped_cstr, xs->str + new_begin, new_end - new_begin);
  stripped_cstr[new_end - new_begin] = '\0';
  xstr_set_cstr(xs, stripped_cstr);
  xfree(stripped_cstr);

}


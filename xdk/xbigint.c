#include "bigint/bigint.h"

#include "xmemory.h"
#include "xbigint.h"

/**
  @brief
    Hidden implementation of xbigint.
*/
struct xbigint_impl {
  /**
    @brief
      Pointer to the raw data.

    When the bigint is finalized, this will be set to NULL.
  */
  int* p_data;
  int sign; ///< @brief Sign of the number, could be 0, -1, or 1.
  int data_len; ///< @brief Length of raw data array (data_len <= mem_size).
  int mem_size; ///< @brief Allocated size in memory.
};

xbigint xbigint_new() {
  xbigint xbi = xmalloc_ty(1, struct xbigint_impl);
  bigint_init((bigint *) xbi);
  return xbi;
}

void xbigint_delete(xbigint xbi) {
  bigint_release((bigint *) xbi);
  xfree(xbi);
}

void xbigint_set_int(xbigint xbi, int value) {
  bigint_from_int((bigint *) xbi, value);
}

xsuccess xbigint_set_double(xbigint xbi, double value) {
  if (bigint_from_double((bigint *) xbi, value) != -BIGINT_NOERR) {
    return XFAILURE;
  } else {
    return XSUCCESS;
  }
}

xsuccess xbigint_set_cstr(xbigint xbi, char* cstr) {
  if (bigint_from_string((bigint *) xbi, cstr) != -BIGINT_NOERR) {
    return XFAILURE;
  } else {
    return XSUCCESS;
  }
}

xsuccess xbigint_get_int(xbigint xbi, int* value) {
  if (bigint_to_int((bigint *) xbi, value) != -BIGINT_NOERR) {
    return XFAILURE;
  } else {
    return XSUCCESS;
  }
}

xsuccess xbigint_get_double(xbigint xbi, double* value) {
  if (bigint_to_double((bigint *) xbi, value) != -BIGINT_NOERR) {
    return XFAILURE;
  } else {
    return XSUCCESS;
  }
}

xsuccess xbigint_get_xstr(xbigint xbi, xstr xs) {
  int len = bigint_string_length((bigint *) xbi) + 4;
  char* buf = xmalloc_ty(len, char);
  bigint_to_string((bigint *) xbi, buf);
  xstr_set_cstr(xs, buf);
  xfree(buf);
  return XSUCCESS;
}

xbigint xbigint_copy(xbigint xbi) {
  xbigint xbi_copy = xbigint_new();
  bigint_copy((bigint *) xbi, (bigint *) xbi_copy);
  return xbi_copy;
}

void xbigint_change_sign(xbigint xbi) {
  bigint_change_sign((bigint *) xbi);
}

int xbigint_get_sign(xbigint xbi) {
  if (bigint_is_zero((bigint *) xbi)) {
    return 0;
  } else if (bigint_is_positive((bigint *) xbi)) {
    return 1;
  } else {
    return -1;
  }
}

void xbigint_set_zero(xbigint xbi) {
  bigint_set_zero((bigint *) xbi);
}

void xbigint_add(xbigint xbi_dest, xbigint xbi_src) {
  bigint_add_by((bigint *) xbi_dest, (bigint *) xbi_src);
}

void xbigint_add_int(xbigint xbi, int value) {
  bigint_add_by_int((bigint *) xbi, value);
}

void xbigint_sub(xbigint xbi_dest, xbigint xbi_src) {
  bigint_sub_by((bigint *) xbi_dest, (bigint *) xbi_src);
}

void xbigint_sub_int(xbigint xbi, int value) {
  bigint_sub_by_int((bigint *) xbi, value);
}

void xbigint_mul(xbigint xbi_dest, xbigint xbi_src) {
  bigint_mul_by((bigint *) xbi_dest, (bigint *) xbi_src);
}

void xbigint_mul_int(xbigint xbi, int value) {
  bigint_mul_by_int((bigint *) xbi, value);
}

xsuccess xbigint_div_int(xbigint xbi, int value) {
  if (bigint_div_by_int((bigint *) xbi, value) != -BIGINT_NOERR) {
    return XFAILURE;
  } else {
    return XSUCCESS;
  }
}

xsuccess xbigint_mod_int(xbigint xbi, int value, int* result) {
  if (bigint_mod_int((bigint *) xbi, value, result) != -BIGINT_NOERR) {
    return XFAILURE;
  } else {
    return XSUCCESS;
  }
}

int xbigint_cmp(xbigint xbi1, xbigint xbi2) {
  return bigint_compare((bigint *) xbi1, (bigint *) xbi2);
}


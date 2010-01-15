#include <stddef.h>

#include "xvec.h"
#include "xmemory.h"

/**
  @brief
    The implementation of xvec.
*/
struct xvec_impl {
  xvec_free xvfree; ///< @brief Function to release element memory.
  int size; ///< @brief Number of elements in the xvec.
  int max_size; ///< @brief Maximum number of elements in the xvec.
  void** data;  ///< @brief Vector of void* pointers to data array.
};

xvec xvec_new(xvec_free xvfree) {
  xvec xv = xmalloc_ty(1, struct xvec_impl);
  xv->xvfree = xvfree;
  xv->size = 0;
  xv->max_size = 16;
  xv->data = xmalloc_ty(xv->max_size, void *);
  return xv;
}

void xvec_delete(xvec xv) {
  int i;
  for (i = 0; i < xv->size; i++) {
    xv->xvfree(xv->data[i]);
  }
  xfree(xv->data);
  xfree(xv);
}

int xvec_size(xvec xv) {
  return xv->size;
}

void* xvec_get(xvec xv, int index) {
  if (index < 0 || index >= xv->size)
    return NULL;

  return xv->data[index];
}

static void ensure_max_size(xvec xv, int max_size) {
  if (xv->max_size < max_size) {
    xv->max_size = max_size * 2;
    xv->data = xrealloc(xv->data, xv->max_size * sizeof(void *));
  }
}

int xvec_put(xvec xv, int index, void* data) {
  if (index < 0 || index > xv->size)
    return -1;
  
  ensure_max_size(xv, index + 1);
  if (index == xv->size) {
    xv->size++; // append new data
  }
  xv->data[index] = data;
  return index;
}

int xvec_insert(xvec xv, int index, void* data) {
  if (index < 0 || index > xv->size)
    return -1;

  ensure_max_size(xv, xv->size + 1);
  xv->size++;
  if (index == xv->size) {
    // append to end
    xv->data[index] = data;
  } else {
    int i;
    for (i = xv->size - 1; i >= index; i--) {
      xv->data[i + 1] = xv->data[i];
    }
    xv->data[index] = data;
  }

  return index;
}

int xvec_push_back(xvec xv, void* data) {
  return xvec_put(xv, xv->size, data);
}

int xvec_remove(xvec xv, int index) {
  int i;
  if (index < 0 || index >= xv->size)
    return -1;
  
  xv->xvfree(xv->data[index]);
  for (i = index; i < xv->size - 1; i++) {
    xv->data[i] = xv->data[i + 1];
  }
  xv->size--;
  return index;
}


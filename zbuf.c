#include <pthread.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#ifndef __APPLE__
#include <malloc.h>
#endif // __APPLE__

#include "zbuf.h"

#define zmalloc_ty(cnt, ty) ((ty *) malloc((cnt) * sizeof(ty)))

static const int zb_min_mem_size = 16;

struct zbuf_impl {
  int begin;
  int end;

  // data range:
  // if end_idx == begin_idx
  //    empty
  // if end_idx > begin_idx
  //    [begin_idx, end_idx - 1]
  // if end_idx < begin_idx
  //    [begin_idx, mem_size - 1] + [0, end_idx - 1]
  //
  // end_idx: 0~mem_size
  // start_idx: 0~mem_size - 1

  zbyte* data;
  int mem_size;
  pthread_mutex_t mtx;

  int shrink_vote; // used for smart shrinking
};


zbuf zbuf_new() {
  zbuf zb = zmalloc_ty(1, struct zbuf_impl);
  zb->begin = 0;
  zb->end = 0;
  zb->mem_size = zb_min_mem_size;
  zb->data = zmalloc_ty(zb->mem_size, zbyte);

  zb->shrink_vote = 0;

  pthread_mutex_init(&(zb->mtx), NULL);

  return zb;
}

static int zbuf_size_with_lock(zbuf zb) {
  int ret;
  if (zb->begin == zb->end) {
    ret = 0;
  } else if (zb->begin < zb->end) {
    // no wrap
    ret = zb->end - zb->begin;
  } else {
    // have wrap
    int len1 = zb->mem_size - zb->begin;
    int len2 = zb->end - 0;
    ret = len1 + len2;
  }
  return ret;
}

int zbuf_size(zbuf zb) {
  pthread_mutex_lock(&(zb->mtx));
  int ret = zbuf_size_with_lock(zb);
  pthread_mutex_unlock(&(zb->mtx));
  return ret;
}

// forward declaration
static int zbuf_peek_with_lock(zbuf zb, zbyte* data_out, int pos, int len);


// caller must have lock
static void repack(zbuf zb, int new_size) {
  if (new_size < zb_min_mem_size) {
    new_size = zb_min_mem_size;
  }
  zbyte* buf = zmalloc_ty(new_size, zbyte);
  int cnt = zbuf_peek_with_lock(zb, buf, 0, new_size);
  free(zb->data);
  zb->data = buf;
  zb->begin = 0;
  zb->end = cnt;
  zb->mem_size = new_size;
  zb->shrink_vote = 0;
}

// smart shrinking, caller has lock
static void try_shrink(zbuf zb) {
  int cur_sz = zbuf_size_with_lock(zb);
  if (cur_sz < zb->mem_size / 10 && zb->mem_size > zb_min_mem_size) {
    zb->shrink_vote++;
  } else {
    zb->shrink_vote = 0;
  }
  if (zb->shrink_vote > 20) {
    repack(zb, cur_sz + 1);
  }
}

int zbuf_append(zbuf zb, zbyte* data, int len) {
  int cnt = 0;
  pthread_mutex_lock(&(zb->mtx));

  // ensure has enough size
  int cur_sz = zbuf_size_with_lock(zb);
  if (cur_sz + len > zb->mem_size - 1) {
    repack(zb, (cur_sz + len) * 2 + 16);
  }

  if (len <= zb->mem_size - zb->end) {
    // directly append
    memcpy(zb->data + zb->end, data, len);
    zb->end += len;
    cnt = len;
  } else {
    // 2 parts
    int len1 = zb->mem_size - zb->end;
    memcpy(zb->data + zb->end, data, len1);
    zb->end = 0;
    cnt += len1;

    int len2 = len - len1;
    memcpy(zb->data, data + len1, len2);
    zb->end = len2;
    cnt += len2;
  }

  pthread_mutex_unlock(&(zb->mtx));
  return cnt;
}

static int zbuf_peek_with_lock(zbuf zb, zbyte* data_out, int pos, int len) {
  int cnt = 0;
  int cur_sz = zbuf_size_with_lock(zb);
  if (pos + len > cur_sz) {
    len = cur_sz - pos;
  }
  if (len > 0) {
    int begin_pos = (zb->begin + pos) % zb->mem_size;
    if (begin_pos < zb->end) {
      // 1 part
      memcpy(data_out, zb->data + begin_pos, len);
      cnt += len;
    } else {
      // 2 parts
      int len1 = zb->mem_size - begin_pos;
      if (len1 > len) {
        len1 = len;
      }
      memcpy(data_out, zb->data + begin_pos, len1);
      cnt = len1;
      int len2 = zb->end - 0;
      if (len2 > len - len1) {
        len2 = len - len1;
      }
      if (len2 > 0) {
        memcpy(data_out + len1, zb->data + 0, len2);
      }
      cnt += len2;
    }
  }
  return cnt;
}

int zbuf_peek(zbuf zb, zbyte* data_out, int len) {
  pthread_mutex_lock(&(zb->mtx));
  int cnt = zbuf_peek_with_lock(zb, data_out, 0, len);
  try_shrink(zb);
  pthread_mutex_unlock(&(zb->mtx));
  return cnt;
}

int zbuf_peek_pos(zbuf zb, zbyte* data_out, int pos, int len) {
  pthread_mutex_lock(&(zb->mtx));
  int cnt = zbuf_peek_with_lock(zb, data_out, pos, len);
  try_shrink(zb);
  pthread_mutex_unlock(&(zb->mtx));
  return cnt;
}

static int zbuf_discard_with_lock(zbuf zb, int len) {
  int cur_sz = zbuf_size_with_lock(zb);
  if (len > cur_sz) {
    len = cur_sz;
  }

  zb->begin += len;
  if (zb->begin == zb->mem_size && zb->end == zb->mem_size) {
    // special case, all data fetched
    zb->begin = 0;
    zb->end = 0;
  } else if (zb->begin >= zb->mem_size) {
    zb->begin = zb->begin % zb->mem_size;
  }

  return len;
}

// discard some data, upto len bytes
int zbuf_discard(zbuf zb, int len) {
  pthread_mutex_lock(&(zb->mtx));
  int cnt = zbuf_discard_with_lock(zb, len);
  try_shrink(zb);
  pthread_mutex_unlock(&(zb->mtx));
  return cnt;
}


int zbuf_clear(zbuf zb) {
  pthread_mutex_lock(&(zb->mtx));
  int cnt = zbuf_size_with_lock(zb);

  zb->mem_size = zb_min_mem_size;
  free(zb->data);
  zb->data = zmalloc_ty(zb_min_mem_size, zbyte);
  zb->begin = 0;
  zb->end = 0;
  zb->shrink_vote = 0;

  pthread_mutex_unlock(&(zb->mtx));
  return cnt;
}

int zbuf_fetch(zbuf zb, zbyte* data_out, int len) {
  pthread_mutex_lock(&(zb->mtx));
  int cnt = zbuf_peek_with_lock(zb, data_out, 0, len);
  assert(cnt == zbuf_discard_with_lock(zb, cnt));
  try_shrink(zb);
  pthread_mutex_unlock(&(zb->mtx));
  return cnt;
}


void zbuf_delete(zbuf zb) {
  pthread_mutex_destroy(&(zb->mtx));
  free(zb->data);
  free(zb);
}

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
typedef uint8_t zu8;
typedef zu8 zbyte;

struct zbuf_impl;

typedef struct zbuf_impl* zbuf;

zbuf zbuf_new();

// get content size
int zbuf_size(zbuf zb);

int zbuf_append(zbuf zb, zbyte* data, int len);

int zbuf_fetch(zbuf zb, zbyte* data_out, int len);

// same as fetch, but not eject data
int zbuf_peek(zbuf zb, zbyte* data_out, int len);

// peeking from a position
int zbuf_peek_pos(zbuf zb, zbyte* data_out, int pos, int len);

// discard some data, upto len bytes
int zbuf_discard(zbuf zb, int len);

// return bytes dropped
int zbuf_clear(zbuf zb);

void zbuf_delete(zbuf zb);

#ifdef __cplusplus
}
#endif

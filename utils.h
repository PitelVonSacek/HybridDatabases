#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include "storage.h"
#include "lock.h"

static inline int ulog2(uint64_t n) {
  // FIXME there are much better ways to compute this
  int ret = -1;
  for (; n; n /= 2) ret++;
  return ret;
}

// From GNU libc documentation:
// The address of a block returned by malloc or realloc in the GNU system is
// always a multiple of eight (or sixteen on 64-bit systems).
//
// So it's aligned enough for us.

static inline void *tr_alloc(size_t size) { return malloc(size); }
static inline void tr_free(void *ptr) { free(ptr); }

#define TR_OFFSET(ptr, offset) ((void*)(((char*)ptr) + offset))

struct reader_fread_context {
  FILE *f;
  size_t size;
  size_t offset;
  unsigned char *data;
};

static void reader_fread_release_context(struct reader_fread_context *ctx) {
  fclose(ctx->f);
  free(ctx->data);
}
static size_t reader_fread(struct reader_fread_context* ctx, size_t l, unsigned char** out) {
  if (l > ctx->size) ctx->data = realloc(ctx->data, l);
  *out = ctx->data;
  return fread(ctx->data, 1, l, ctx->f);
}

static Reader* reader_fread_create(struct reader_fread_context* context, const char* file) {
  if (!(context->f = fopen(file, "rb"))) return 0;

  context->size = context->offset = 0;
  context->data = 0;

  return reader_create((size_t (*)(void *, size_t,  unsigned char **))reader_fread,
                       context, (void(*)(void*))reader_fread_release_context);
}


#endif


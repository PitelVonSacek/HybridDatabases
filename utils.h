#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "storage.h"

static inline int ulog2(uint64_t n) {
  // FIXME there are much better ways to compute this
  int ret = -1;
  for (; n; n /= 2) ret++;
  return ret;
}

typedef struct {
  uint64_t value;
} Lock;

static inline void l_init  (Lock* l) { l->value = 1; }

// little hack for dump_thread, it needs to know old value of lock
static inline uint64_t l_lock_(Lock* l, void* ptr, uint64_t version) {
  uint64_t v;

  while (1) {
    v = __sync_fetch_and_add(&l->value, 0);
    if ((v & 1) ? (v > version) : (v != (uint64_t)ptr)) return 0;
    if (__sync_bool_compare_and_swap(&l->value, v, ptr)) return v;
  }
}

static inline int  l_lock  (Lock* l, void* ptr, uint64_t version) {
  uint64_t v;

  while (1) {
    v = __sync_fetch_and_add(&l->value, 0);
    if ((v & 1) ? (v > version) : (v != (uint64_t)ptr)) return 0;
    if (__sync_bool_compare_and_swap(&l->value, v, ptr)) return 1;
  }
}

static inline int  l_unlock(Lock* l, void* ptr, uint64_t version) {
  return __sync_bool_compare_and_swap(&l->value, (uint64_t)ptr, version);
}

static inline int  l_check (Lock* l, void* ptr, uint64_t version) {
  uint64_t v = __sync_fetch_and_add(&l->value, 0);
  return (v & 1) ? (v <= version) : (v == (uint64_t)ptr);
}

// From GNU libc documentation:
// The address of a block returned by malloc or realloc in the GNU system is
// always a multiple of eight (or sixteen on 64-bit systems).
//
// So it's aligned enought for us.

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


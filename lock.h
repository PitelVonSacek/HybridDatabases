#ifndef __LOCK_H__
#define __LOCK_H__

#include <stdint.h>

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

static inline int l_lock  (Lock* l, void* ptr, uint64_t version) {
  uint64_t v;

  while (1) {
    v = __sync_fetch_and_add(&l->value, 0);
    if ((v & 1) ? (v > version) : (v != (uint64_t)ptr)) return 0;
    if (__sync_bool_compare_and_swap(&l->value, v, ptr)) return 1;
  }
}

static inline int l_unlock(Lock* l, void* ptr, uint64_t version) {
  return __sync_bool_compare_and_swap(&l->value, (uint64_t)ptr, version);
}

static inline int l_check (Lock* l, void* ptr, uint64_t version) {
  uint64_t v = __sync_fetch_and_add(&l->value, 0);
  return (v & 1) ? (v <= version) : (v == (uint64_t)ptr);
}

#endif


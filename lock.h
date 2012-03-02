#ifndef __LOCK_H__
#define __LOCK_H__

#include <stdint.h>

#include "atomic.h"


typedef struct { uint64_t value; } Lock;


static inline void l_init(Lock* l) { l->value = 1; }

// little hack for dump_thread, it needs to know old value of lock
static inline uint64_t l_lock_(Lock* l, void* ptr, uint64_t version) {
  uint64_t v;

  while (1) {
    v = atomic_read(&l->value);
    if ((v & 1) ? (v > version) : (v != (uint64_t)ptr)) return 0;
    if (atomic_cmpswp(&l->value, v, (uint64_t)ptr)) return v;
  }
}

/*
 * return value:
 *  0 - fail
 *  1 - success
 *  2 - success, lock was already locked by us
 */
static inline int l_lock  (Lock* l, void* ptr, uint64_t version) {
  uint64_t v;

  while (1) {
    v = atomic_read(&l->value);
    if ((v & 1) ? (v > version) : (v != (uint64_t)ptr)) return 0;
    if (atomic_cmpswp(&l->value, v, (uint64_t)ptr)) return 1 + (v == (uint64_t)ptr);
  }
}

static inline int l_unlock(Lock* l, void* ptr, uint64_t version) {
  return atomic_cmpswp(&l->value, (uint64_t)ptr, version);
}

static inline int l_check (Lock* l, void* ptr, uint64_t version) {
  uint64_t v = atomic_read(&l->value);
  return (v & 1) ? (v <= version) : (v == (uint64_t)ptr);
}

#endif


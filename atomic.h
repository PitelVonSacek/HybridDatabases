#ifndef __ATOMIC_H__
#define __ATOMIC_H__

#if 1

#include "atomic_amd64.h"

#define atomic_read_relaxed(ptr) atomic_read(ptr)
#define atomic_write_relaxed(...) atromic_write(__VA_ARGS__)

#define atomic_read(ptr) (*((volatile typeof(ptr))(ptr)))
#define atomic_write(ptr, value) (atomic_read(ptr) = (value))
#define atomic_barrier() asm volatile ()

#define atomic_cmpswp(ptr, old, new_) \
  _atomic_cmpxchg(ptr, old, new_, sizeof(*(ptr)), "lock; ")

#define atomic_swp(ptr, new_) \
  _atomic_xchg(new_, ptr, sizeof(*(ptr)))

#define atomic_add_and_fetch(ptr, x) \
  ({ \
    typeof(*(ptr)) __x = (x); \
    __x + _atomic_xadd(ptr, __x, "lock; "); \
  })

#define atomic_fetch_and_add(ptr, x) \
  _atomic_xadd(ptr, x, "lock; ")

#define atomic_add(ptr, x) ({ _atomic_xadd(ptr, x, "lock; "); (void)0; })

#define atomic_inc(ptr) atomic_add(ptr, 1)
#define atomic_dec(ptr) atomic_add(ptr, -1)

#else // USE_ATOMIC_BUILTINS

#define atomic_read(ptr) __sync_fetch_and_add(ptr, 0)
#define atomic_write(ptr, val) ((void)atomic_swp(ptr, val));
#define atomic_barrier() __sync_synchronize()

#define atomic_cmpswp(ptr, old, new_) \
  __sync_bool_compare_and_swap(ptr, old, new_)

// ugly & ineffective one
#define atomic_swp(ptr, val) __sync_val_compare_and_swap(ptr, *ptr, val)

#define atomic_add_and_fetch(ptr, x) __sync_add_and_fetch(ptr, x)
#define atomic_fetch_and_add(ptr, x) __sync_fetch_and_add(ptr, x)

#define atomic_add(ptr, x) atomic_add_and_fetch(ptr, x)

#define atomic_inc(ptr) __sync_add_and_fetch(ptr, 1)
#define atomic_dec(ptr) __sync_add_and_fetch(ptr, -1)
#endif

#endif


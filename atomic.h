#ifndef __ATOMIC_H__
#define __ATOMIC_H__

#define atomic_read(ptr) __sync_fetch_and_add(ptr, 0)

#define atomic_cmpswp(ptr, old, new_) \
  __sync_bool_compare_and_swap(ptr, old, new_)

#define atomic_inc(ptr) __sync_add_and_fetch(ptr, 1)
#define atomic_dec(ptr) __sync_add_and_fetch(ptr, -1)

#endif


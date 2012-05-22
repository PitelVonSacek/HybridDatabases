#ifndef __BLOCK_ALLOCATOR_H__
#define __BLOCK_ALLOCATOR_H__

/**
 * @file
 * @brief Jednoduchý cachující alokátor.
 *
 * Udržuje uvolněné objekty ve vnitřním bufferu ve zkonstruovaném stavu
 * vyjma prvních <tt>sizeof(void*)</tt> bajtů, které přepíše.
 *
 * @see allocators.h
 */

#include "../utils/basic_utils.h"
#include "../utils/atomic.h"
#include "../utils/slist.h"

/// Jednoduchý cachující alokátor.
struct SimpleAllocator {
  size_t obj_size;
  void (*init)(void*);
  void (*destroy)(void*);
  size_t gc_threshold;
  size_t free_objs_count;
  struct SList free_objs;
};

/// Inicializátor pro #SimpleAllocator.
#define SimpleAllocatorInit(size, GC_THRESHOLD, init_, destroy_) \
  { \
    .obj_size = size, \
    .init = init_, \
    .destroy = destroy_, \
    .gc_threshold = GC_THRESHOLD, \
    .free_objs_count = 0, \
    .free_objs = SListInit \
  }

/**
 * @brief Inicializuje #SimpleAllocator.
 *
 * @param block_size Velikost jednoho objektu.
 * @param gc_threshold Maximální počet cachovaných objektů.
 * @param init Konstruktor objektu.
 * @param destroy Destruktor objektu.
 */
static inline void simple_allocator_init(struct SimpleAllocator *A,
                                        size_t block_size, size_t gc_threshold,
                                        void(*init)(void*), void(*destroy)(void*));

/// Zničí #SimpleAllocator.
static inline void simple_allocator_destroy(struct SimpleAllocator *A);


/// Alokuje jeden objekt.
static inline void *simple_allocator_alloc(struct SimpleAllocator *A);
/// Uvolní objekt.
static inline void simple_allocator_free(struct SimpleAllocator *A, void *obj);


/********************
 *  Implementation  *
 ********************/

/// Vyčistí vnitřní buffer.
static void _simple_allocator_gc(struct SimpleAllocator *A) {
  while (atomic_read(&A->free_objs_count) > A->gc_threshold) {
    void *obj = slist_atomic_pop(&A->free_objs);

    if (!obj) return;
    else atomic_dec(&A->free_objs_count);

    A->destroy(obj);
    free(obj);
  }
}

static inline void *simple_allocator_alloc(struct SimpleAllocator *A) {
  void *obj = slist_atomic_pop(&A->free_objs);

  if (obj) atomic_dec(&A->free_objs_count);
  else {
    obj = xmalloc(A->obj_size);
    A->init(obj);
  }

  return obj;
}

static inline void simple_allocator_free(struct SimpleAllocator *A, void *obj) {
  atomic_inc(&A->free_objs_count);
  slist_atomic_push(&A->free_objs, obj);
  if (atomic_read(&A->free_objs_count) > A->gc_threshold)
    _simple_allocator_gc(A);
}

static inline void simple_allocator_init(struct SimpleAllocator *A,
                                         size_t block_size, size_t gc_threshold,
                                        void(*init)(void*), void(*destroy)(void*)) {
  *A = (struct SimpleAllocator)SimpleAllocatorInit(block_size, gc_threshold,
                                                   init, destroy);
}

static inline void simple_allocator_destroy(struct SimpleAllocator *A) {
  A->gc_threshold = 0;
  _simple_allocator_gc(A);
}

#endif


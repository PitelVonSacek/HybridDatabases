#ifndef __GENERIC_ALLOCATOR_H__ 
#define __GENERIC_ALLOCATOR_H__ 

/**
 * @file
 * @brief Primitivní alokátor transakční paměti.
 *
 * Pouze obaluje @c malloc o opožděné uvolňování paměti.
 *
 * @see allocators.h
 */

#include "../utils/basic_utils.h"
#include "../utils/atomic.h"
#include "../utils/slist.h"

/// @internal
struct GenericAllocatorItem {
  struct SList slist;
  uint64_t time;
};

/// Alokátor transakční paměti.
struct GenericAllocator {
  uint64_t (*get_time)(void*);
  void *get_time_context;

  size_t gc_threshold;

  size_t counter;
  struct SList free_list;
};


/**
 * @brief Inicializuje #GenericAllocator.
 *
 * @param gc_threshold Maximální délka fronty k uvolnění.
 * @param get_time Callback funkce pro zjištění aktuálního času.
 *                 Stránky starší než aktuální čas mohou být bezpečně uvolněny.
 * @param context Kontext pro callback @a get_time.
 */
void generic_allocator_init(struct GenericAllocator *A, size_t gc_threshold,
                            uint64_t (*get_time)(void*), void *context);
/// Znicí #GenericAllocator.
void generic_allocator_destroy(struct GenericAllocator *A);


/// Alokuje kus transakční paměti o velikost @a size.
static inline void *generic_allocator_alloc(struct GenericAllocator *A, size_t size);
/// Uvolní transakční paměť v čase @a end_time.
static inline void generic_allocator_free(struct GenericAllocator *A,
                                          void *ptr_, uint64_t end_time);


/********************
 *  Implementation  *
 ********************/

static inline void *generic_allocator_alloc(struct GenericAllocator *A, size_t size) {
  if (size < sizeof(struct GenericAllocatorItem))
    size = sizeof(struct GenericAllocatorItem);

  return xmalloc(size);
}

/// Uvolní z fronty paměť, ktrou uolnit lze.
void _generic_allocator_collect_garbage(struct GenericAllocator *A);

static inline void generic_allocator_free(struct GenericAllocator *A,
                                          void *ptr_, uint64_t end_time) {
  if (!ptr_) return;

  struct GenericAllocatorItem *ptr = ptr_;

  atomic_inc(&A->counter);

  ptr->time = end_time;
  slist_atomic_push(&A->free_list, &ptr->slist);

  if (atomic_read(&A->counter) > A->gc_threshold)
    _generic_allocator_collect_garbage(A);
}

#endif


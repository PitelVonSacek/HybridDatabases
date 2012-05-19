#ifndef __GENERIC_ALLOCATOR_H__ 
#define __GENERIC_ALLOCATOR_H__ 

#include "../utils/basic_utils.h"
#include "../utils/atomic.h"
#include "../utils/slist.h"

struct GenericAllocatorItem {
  struct SList slist;
  uint64_t time;
};

struct GenericAllocator {
  uint64_t (*get_time)(void*);
  void *get_time_context;

  size_t gc_threshold;

  size_t counter;
  struct SList free_list;
};

void generic_allocator_init(struct GenericAllocator *A, size_t gc_threshold,
                            uint64_t (*get_time)(void*), void *context);
void generic_allocator_destroy(struct GenericAllocator *A);


static inline void *generic_allocator_alloc(struct GenericAllocator *A, size_t size);
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


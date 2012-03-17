#ifndef __GENERIC_ALLOCATOR_H__ 
#define __GENERIC_ALLOCATOR_H__ 

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include "../utils/atomic.h"
#include "../utils/slist.h"

struct GenericAllocator {
  uint64_t (*get_time)(void*);
  void *get_time_context;

  size_t gc_threshold;

  size_t counter;
  SList(uint64_t time) free_list;
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
  if (size < sizeof(SListItem(&A->free_list)))
    size = sizeof(SListItem(&A->free_list));

  return malloc(size);
}

void _generic_allocator_collect_garbage(struct GenericAllocator *A);
static inline void generic_allocator_free(struct GenericAllocator *A,
                                          void *ptr_, uint64_t end_time) {
  if (!ptr_) return;

  SListItem(&A->free_list) *ptr = ptr_;

  atomic_inc(&A->counter);

  ptr->time = end_time;
  slist_atomic_push(&A->free_list, ptr);

  if (atomic_read(&A->counter) > A->gc_threshold)
    _generic_allocator_collect_garbage(A);
}

#endif


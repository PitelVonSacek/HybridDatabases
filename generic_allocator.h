#ifndef __GENERIC_ALLOCATOR_H__ 
#define __GENERIC_ALLOCATOR_H__ 

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include "atomic.h"

// extremely stupid but correct implementation

struct GenericAllocatorInfo {
  struct GenericAllocatorFreeItem {
    struct GenericAllocatorFreeItem *next;
    uint64_t time;
  } *free_list;
  size_t counter;
};


static inline void *generic_alloc(struct GenericAllocatorInfo *info, size_t size) {
  if (size < sizeof(struct GenericAllocatorFreeItem))
    size = sizeof(struct GenericAllocatorFreeItem);

  return malloc(size);
}

static inline void generic_free(struct GenericAllocatorInfo *info, void *ptr_, 
                                uint64_t end_time) {
  struct GenericAllocatorFreeItem *ptr = ptr_;

  atomic_inc(&info->counter);

  ptr->time = end_time;
  do {
    ptr->next = atomic_read(&info->free_list);
  } while (!atomic_cmpswp(&info->free_list, ptr->next, ptr));
}

void generic_allocator_init(struct GenericAllocatorInfo* info);
void generic_allocator_destroy(struct GenericAllocatorInfo* info);

bool generic_free_cache(struct GenericAllocatorInfo *info, size_t rest, uint64_t time);

#endif


#ifndef __GENERIC_ALLOCATOR_H__ 
#define __GENERIC_ALLOCATOR_H__ 

#include "node_allocator.h"

static const size_t GenericAllocatorSizes[] = {
  16, 32, 64, 104, 232, 488, 1000, 2024
};

#define GenericAllocatorAllocators \
  (sizeof(GenericAllocatorSizes)/sizeof(GenericAllocatorSizes[0]))

struct GenericAllocatorInfo {
  struct NodeAllocatorInfo allocators[GenericAllocatorAllocators];
};

#define GenericAllocatorInit \
  { \
    { .size =   16 + 8, .count = 0, .free_nodes = 0 }, \
    { .size =   32 + 8, .count = 0, .free_nodes = 0 }, \
    { .size =   64 + 8, .count = 0, .free_nodes = 0 }, \
    { .size =  104 + 8, .count = 0, .free_nodes = 0 }, \
    { .size =  232 + 8, .count = 0, .free_nodes = 0 }, \
    { .size =  488 + 8, .count = 0, .free_nodes = 0 }, \
    { .size = 1000 + 8, .count = 0, .free_nodes = 0 }, \
    { .size = 2024 + 8, .count = 0, .free_nodes = 0 }  \
  }

static inline void *generic_alloc(struct GenericAllocatorInfo *info, size_t size) {
  size_t *ret;
  switch (size) {
    case 0 ... 16:
      ret = node_alloc(info->allocators + 0);
      *ret = 0;
      break;
    case 17 ... 32:
      ret = node_alloc(info->allocators + 1);
      *ret = 1;
      break;
    case 33 ... 64:
      ret = node_alloc(info->allocators + 2);
      *ret = 2;
      break;
    case 65 ... 104:
      ret = node_alloc(info->allocators + 3);
      *ret = 3;
      break;
    case 105 ... 232:
      ret = node_alloc(info->allocators + 4);
      *ret = 4;
      break;
    case 233 ... 488:
      ret = node_alloc(info->allocators + 5);
      *ret = 5;
      break;
    case 489 ... 1000:
      ret = node_alloc(info->allocators + 6);
      *ret = 6;
      break;
    case 1001 ... 2024:
      ret = node_alloc(info->allocators + 7);
      *ret = 7;
      break;
    default:
      ret = malloc(size + 8);
      *ret = 8;
  }

  return (void*)(ret + 1);
}

void generic_free(struct GenericAllocatorInfo *info, void *ptr_, uint64_t end_time) {
  size_t *ptr = ptr_;
  ptr -= 1;

  // FIXME !!!
  if (*ptr >= GenericAllocatorAllocators) free(ptr);
  else node_free(info->allocators + *ptr, ptr, end_time);
}

#endif


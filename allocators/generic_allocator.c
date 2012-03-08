#include "generic_allocator.h"

bool generic_free_cache(struct GenericAllocatorInfo *info, size_t rest, uint64_t time) {
  return false;
}

void generic_allocator_init(struct GenericAllocatorInfo* info) {
  info->free_list = 0;
  info->counter = 0;
}

void generic_allocator_destroy(struct GenericAllocatorInfo* info) {
  while (info->free_list) {
    struct GenericAllocatorFreeItem *item = info->free_list;
    info->free_list = item->next;
    free(item);
  }

  generic_allocator_init(info);
}



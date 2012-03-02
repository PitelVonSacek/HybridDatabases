#ifndef __NODE_ALLOCATOR_H__
#define __NODE_ALLOCATOR_H__

#include <stdint.h>
#include <stdbool.h>

#include "atomic.h"

struct FreeNode {
  struct FreeNode *next;
  /*
   * For correct function, timestamps of released blocks must be nondecreasing
   */
  uint64_t timestamp;
};

struct NodeAllocatorInfo {
  size_t size;
  size_t counter;
  struct FreeNode *free_nodes;
};

void *node_alloc_nodes(struct NodeAllocatorInfo*);
// false when unable to resease enough nodes because they're too new
bool node_free_nodes(struct NodeAllocatorInfo*, size_t remaining, uint64_t older_than);

static inline void *node_alloc(struct NodeAllocatorInfo *info) {
  struct FreeNode *node;
 
  do {
    node = atomic_swp(&info->free_nodes, (struct FreeNode*)(char*)1);
  } while ((size_t)node == 1);

  if (node) {
    atomic_swp(&info->free_nodes, node->next);
    return node;
  } else {
    atomic_swp(&info->free_nodes, 0);
    return node_alloc_nodes(info);
  }

#if 0
  // locking could fail when releasing nodes
  loop:
  if (node = atomic_read(&(info->free_nodes))) {
    if (!atomic_cmpswp(&(info->free_nodes), node, node->next)) goto loop;
    atomic_dec(&(info->counter));
  } else 
    return node_alloc_nodes(info);

  return node;
#endif
}

static inline void _node_free(struct NodeAllocatorInfo* info, struct FreeNode *node) {
  do {
    node->next = atomic_read(&(info->free_nodes));
  } while ((size_t)node->next == 1 || 
           !atomic_cmpswp(&(info->free_nodes), node->next, node));
}

static inline void node_free(struct NodeAllocatorInfo* info, void *node_, uint64_t time) {
  struct FreeNode *node = node_;
  node->timestamp = time;

  atomic_inc(&(info->counter));
  _node_free(info, node);
}

#endif


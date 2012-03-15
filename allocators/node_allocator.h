#ifndef __NODE_ALLOCATOR_H__
#define __NODE_ALLOCATOR_H__

/*
 * Slab-like allocator, preferes space over speed
 */

#include <pthread.h>
#include "vpage_allocator.h"
#include "../utils/list.h"
#include "../utils/slist.h"

struct NodeType_;
struct Node_;

struct NodeAllocatorBlock {
  struct List head;
  struct SList free_nodes;
  size_t used;
  struct NodeType_ *type;
  unsigned char data[0];
};

struct NodeAllocator {
  struct VPageAllocator *allocator;
  struct NodeType_ *type;
  struct List blocks;
  pthread_mutex_t mutex;
};

void node_allocator_init(struct NodeAllocator *A,
                         struct VPageAllocator *page_allocator, struct NodeType_ *type);

void node_allocator_destroy(struct NodeAllocator *A);


static inline struct Node_ *node_allocator_alloc(struct NodeAllocator *A);
static inline void node_allocator_free(struct NodeAllocator *A, 
                                       struct Node_ *node, uint64_t time);


/********************
 *  Implementation  *
 ********************/

void *_node_allocator_alloc_page(struct NodeAllocator *A);

static inline struct Node_ *node_allocator_alloc(struct NodeAllocator *A) {
  struct NodeAllocatorBlock *block;
  void *ret = 0;

  pthread_mutex_lock(&A->mutex);
  if (block = listGetContainer(struct NodeAllocatorBlock, head, A->blocks.prev))
      ret = slist_pop(&block->free_nodes);
  if (ret) block->used++;
  pthread_mutex_unlock(&A->mutex);

  return ret ?: _node_allocator_alloc_page(A);
}

static inline void node_allocator_free(struct NodeAllocator *A, 
                                       struct Node_ *node, uint64_t time) {
  struct NodeAllocatorBlock *block = page_allocator_get_page(node);
  bool do_page_free = false;

  pthread_mutex_lock(&A->mutex);
  if (!--block->used) {
    do_page_free = true;
    list_remove(&block->head);
  } else 
    slist_push(&block->free_nodes, (struct SList*)node);
  pthread_mutex_unlock(&A->mutex);

  if (do_page_free) vpage_allocator_free(A->allocator, block, time);
}

#endif


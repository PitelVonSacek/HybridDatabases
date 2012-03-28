#ifndef __NODE_ALLOCATOR_H__
#define __NODE_ALLOCATOR_H__

/*
 * Slab-like allocator, preferes space over speed
 */

#include <pthread.h>
#include "vpage_allocator.h"
#include "../utils/list.h"
#include "../utils/slist.h"
#include "../database.types/node.h"

struct NodeType_;

struct NodeAllocatorBlock {
  struct List head;
  struct List dump_head;
  struct SList free_nodes;
  size_t used;
  struct NodeType_ *type;
  unsigned char data[0];
};

struct NodeAllocator {
  struct VPageAllocator *allocator;
  struct NodeType_ *type;
  struct List blocks;
  struct List dump_blocks;
  Node *dump_ptr;
  pthread_mutex_t mutex;
};

void node_allocator_init(struct NodeAllocator *A,
                         struct VPageAllocator *page_allocator, struct NodeType_ *type);

void node_allocator_destroy(struct NodeAllocator *A);


static inline Node *node_allocator_alloc(struct NodeAllocator *A);
static inline void node_allocator_free(struct NodeAllocator *A, 
                                       Node *node, uint64_t time);

void node_allocator_dump_init(struct NodeAllocator *A);
void node_allocator_dump_next(struct NodeAllocator *A);

/********************
 *  Implementation  *
 ********************/

void *_node_allocator_alloc_page(struct NodeAllocator *A);
void _node_allocator_dump_next_nolock(struct NodeAllocator *A);

static inline Node *node_allocator_alloc(struct NodeAllocator *A) {
  struct NodeAllocatorBlock *block;
  void *ret = 0;

  pthread_mutex_lock(&A->mutex);
  block = listGetContainer(struct NodeAllocatorBlock, head, A->blocks.prev);
  if (&block->head != &A->blocks)
      ret = slist_pop(&block->free_nodes);
  if (ret) block->used++;
  if (slist_empty(&block->free_nodes))
    list_add_begin(&A->blocks, list_remove(&block->head));
  pthread_mutex_unlock(&A->mutex);

  return ret ?: _node_allocator_alloc_page(A);
}

static inline void node_allocator_free(struct NodeAllocator *A, 
                                       Node *node, uint64_t time) {
  struct NodeAllocatorBlock *block = page_allocator_get_page(node);
  bool do_page_free = false;

  node->id = 0;

  pthread_mutex_lock(&A->mutex);
  if (A->dump_ptr == node) _node_allocator_dump_next_nolock(A);

  if (!--block->used) {
    do_page_free = true;
    list_remove(&block->head);
    list_remove(&block->dump_head);
  } else {
    if (slist_empty(&block->free_nodes))
      list_add_end(&A->blocks, list_remove(&block->head));
    slist_push(&block->free_nodes, (struct SList*)node);
  }
  pthread_mutex_unlock(&A->mutex);

  if (do_page_free) vpage_allocator_free(A->allocator, block, time);
}

#endif


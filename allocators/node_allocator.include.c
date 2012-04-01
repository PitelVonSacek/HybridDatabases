#include "node_allocator.h"
#include "../database.types/node.h"
#include "../database.types/node_type.h"
#include "../database.types/enums.h"

void node_allocator_init(struct NodeAllocator *A,
                         struct VPageAllocator *page_allocator, struct NodeType_ *type) {
  *A = (struct NodeAllocator){
    .allocator = page_allocator,
    .type = type,
    .dump_ptr = 0
  };

  list_init_head(&A->blocks);
  list_init_head(&A->dump_blocks);
  pthread_mutex_init(&A->mutex, 0);
}

void node_allocator_destroy(struct NodeAllocator *A) {
  while (!list_empty(&A->blocks)) 
    vpage_allocator_free(A->allocator, list_remove(A->blocks.prev), 0);
  
  pthread_mutex_destroy(&A->mutex);
}

void *_node_allocator_alloc_page(struct NodeAllocator *A) {
  struct NodeAllocatorBlock *block = vpage_allocator_alloc(A->allocator);

  *block = (struct NodeAllocatorBlock){
    .free_nodes = SListInit,
    .used = 1,
    .type = A->type
  };

  const size_t size = A->type->size;

  const unsigned nodes_per_block = 
    (PAGE_ALLOCATOR_PAGE_SIZE - sizeof(struct NodeAllocatorBlock)) / size;

  for (unsigned i = 0; i < nodes_per_block; i++) {
    Node *node = (Node*)(block->data + i * size);
    node->id = 0;
#if INPLACE_NODE_LOCKS
    l_init(&node->lock);
#endif
    slist_push(&block->free_nodes, &node->slist);
  }

  void *ret = slist_pop(&block->free_nodes);

  pthread_mutex_lock(&A->mutex);
  list_add_end(&A->blocks, &block->head);
  list_add_end(&A->dump_blocks, &block->dump_head);
  pthread_mutex_unlock(&A->mutex);

  return ret;
}


static bool _node_get_next_step(NodeType *type, struct NodeAllocatorBlock **block,
                               Node **node) {
  const size_t max_offset =
    ((PAGE_ALLOCATOR_PAGE_SIZE - sizeof(struct NodeAllocatorBlock))
    / type->size) * type->size;

  *node = util_apply_offset((*node), type->size);

  if (util_get_offset(block[0]->data, *node) >= max_offset) {
    *block = utilContainerOf(block[0]->dump_head.next,
                             struct NodeAllocatorBlock, dump_head);

    if (&block[0]->dump_head == &type->allocator->dump_blocks) return false;
    *node = (void*)block[0]->data;
  }

  return true;
}

void node_allocator_dump_init(struct NodeAllocator *A) {
  struct NodeAllocatorBlock *block;

  pthread_mutex_lock(&A->mutex);
  A->dump_ptr = 0;

  block = utilContainerOf(A->dump_blocks.next,
                          struct NodeAllocatorBlock, dump_head);

  if (&block->dump_head == &A->dump_blocks) goto end;

  Node *node = (void*)block->data;

  while (!atomic_read(&node->id))
    if (!_node_get_next_step(A->type, &block, &node)) goto end;

  A->dump_ptr = node;

  end:
  pthread_mutex_unlock(&A->mutex);
}

void node_allocator_dump_next(struct NodeAllocator *A) {
  pthread_mutex_lock(&A->mutex);
  _node_allocator_dump_next_nolock(A);
  pthread_mutex_unlock(&A->mutex);
}

void _node_allocator_dump_next_nolock(struct NodeAllocator *A) {
  Node *node = A->dump_ptr;
  struct NodeAllocatorBlock *block = page_allocator_get_page(node);

  do {
    if (!_node_get_next_step(A->type, &block, &node)) {
      A->dump_ptr = 0;
      return;
    }
  } while (!atomic_read(&node->id));

  A->dump_ptr = node;
}


#include "node_allocator.h"
#include "../database.types/node.h"

void node_allocator_init(struct NodeAllocator *A,
                         struct VPageAllocator *page_allocator, struct NodeType_ *type) {
  *A = (struct NodeAllocator){
    .allocator = page_allocator,
    .type = type
  };

  list_init_head(&A->blocks);
  list_init_head(&A->dump_blocks);
  pthread_mutex_init(&A->mutex, 0);
}

void node_allocator_destroy(struct NodeAllocator *A) {
  void *page;

  while (!list_empty(&A->blocks)) 
    vpage_allocator_free(A->allocator, list_remove(A->blocks.prev), 0);
  
  pthread_mutex_destroy(&A->mutex);
}

void *_node_allocator_alloc_page(struct NodeAllocator *A) {
  struct NodeAllocatorBlock *block = vpage_allocator_alloc(A->allocator);

  memset(block, 0x00, PAGE_ALLOCATOR_PAGE_SIZE);

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
    slist_push(&block->free_nodes, &node->slist);
  }

  void *ret = slist_pop(&block->free_nodes);

  pthread_mutex_lock(&A->mutex);
  list_add_end(&A->blocks, &block->head);
  list_add_end(&A->dump_blocks, &block->dump_head);
  pthread_mutex_unlock(&A->mutex);

  return ret;
}


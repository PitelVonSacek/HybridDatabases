#include "node_allocator.h"

void node_allocator_init(struct NodeAllocator *A,
                         struct VPageAllocator *page_allocator, size_t node_size) {
  *A = (struct NodeAllocator){
    .allocator = page_allocator,
    .node_size = node_size
  };

  list_init(&A->blocks);
  pthread_mutex_init(&A->mutex);
}

void node_allocator_destroy(struct NodeAllocator *A) {
  void *page;

  while (!list_empty(&A->blocks)) 
    vpage_allocator_free(A->allocator, list_remove(A->blocks.prev), 0);
  
  pthread_mutex_destroy(&A->mutex);
}

void *_node_allocator_alloc_page(struct NodeAllocator *A) {
  struct NodeAllocatorBlock *block = vpage_allocator_alloc(A->allocator);

  *block = (struct NodeAllocatorBlock){
    .empty_nodes = SListInit,
    .used = 1,
    .type = A->type
  };

  const size = A->type->size;

  const unsigned nodes_per_block = 
    (PAGE_ALLOCATOR_PAGE_SIZE - sizeof(struct NodeAllocatorBlock)) / size;

  for (unsigned i = 0; i < nodes_per_block; i++) {
    slist_push(&block->empty_nodes, (struct SList*)(block->data + i * size));
  }

  void *ret = slist_pop(&block->empty_nodes);

  pthread_mutex_lock(&A->mutex);
  list_add_end(&A->blocks, &block->head);
  pthread_mutex_unlock(&A->mutex);

  return ret;
}


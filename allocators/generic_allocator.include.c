#include "generic_allocator.h"

void generic_allocator_init(struct GenericAllocator *A, size_t gc_threshold,
                            uint64_t (*get_time)(void*), void *context) {
  *A = (struct GenericAllocator){
    .get_time = get_time,
    .get_time_context = context,

    .gc_threshold = gc_threshold,

    .counter = 0,
    .free_list = SListInit
  };
}

void generic_allocator_destroy(struct GenericAllocator *A) {
  void *ptr;

  while (ptr = slist_pop(&A->free_list)) free(ptr);
}

void _generic_allocator_collect_garbage(struct GenericAllocator *A) {
  SListItem(&A->free_list) *item;
  uint64_t time = A->get_time(A->get_time_context);

  typeof(A->free_list) free_list = SListInit;
  slist_atomic_swap(&A->free_list, &free_list);

  while (item = slist_pop(&free_list)) {
    if (item->time > time) slist_atomic_push(&A->free_list, item);
    else {
      atomic_dec(&A->counter);
      free(item);
    }
  }
}


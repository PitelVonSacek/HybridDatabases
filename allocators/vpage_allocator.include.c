#include "vpage_allocator.h"

void vpage_allocator_init(struct VPageAllocator *A, size_t gc_threshold,
                          uint64_t (*get_time)(void*), void *context) {
  *A = (struct VPageAllocator){
    .get_time = get_time,
    .get_time_context = context,

    .gc_threshold = gc_threshold,

    .free_pages_count = 0,
    .free_pages = SListInit
  };
}

static void _vpage_allocator_gc(struct VPageAllocator *A, uint64_t time) {
  struct VPageAllocatorItem *page;

  while (atomic_read(&A->free_pages_count) > A->gc_threshold) {
    page = _vpage_allocator_get_page(A);

    if (!page) return;
    if (page->timestamp > time) goto complicated_way;

    page_free(page);
  }

  return;
  complicated_way:;

  struct SList free_pages = SListInit;

  slist_push(&free_pages, &page->slist);
  slist_atomic_swap(&A->free_pages, &free_pages);

  while (page = (struct VPageAllocatorItem*)slist_pop(&free_pages)) {
    if (page->timestamp > time) slist_atomic_push(&A->free_pages, &page->slist);
    else {
      page_free(page);
      atomic_dec(&A->free_pages_count);
    }
  }
}

void vpage_allocator_destroy(struct VPageAllocator *A) {
  A->gc_threshold = 0;
  _vpage_allocator_gc(A, ~(uint64_t)0);
}

void _vpage_allocator_collect_garbage(struct VPageAllocator *A) {
  _vpage_allocator_gc(A, A->get_time(A->get_time_context));
}


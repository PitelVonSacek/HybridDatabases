#include "vpage_allocator.h"

void vpage_allocator_init(struct VPageAllocator *A, size_t gc_threshold,
                          uint64_t (*get_time)(void*), void *context) {
  *A = (struct VPageAllocator){
    .get_time = get_time,
    .context = context,

    .gc_threshold = gc_threshold,

    .free_pages_counter = 0,
    .free_pages = SListInit
  };
}

static void _vpage_allocator_gc(struct VPageAllocator *A, uint64_t time) {
  SListItem(&A->free_pages) *page;

  while (atomic_read(&A->free_pages_counter) > A->threshold) {
    page = _vpage_allocator_get_page(A);

    if (!page) return;
    if (page->timestamp > time) goto complicated_way;

    munmap(page, PAGE_ALLOCATOR_PAGE_SIZE);
  }

  return;
  complicated_way:

  typeof(&A->free_pages) free_pages = { .next = 0 };
  slist_push(&free_pages, page);
  slist_atomic_swap(&A->free_pages, &free_pages);

  while (page = slist_pop(&free_pages)) {
    if (page->timestamp > time) slist_atomic_push(&A->free_pages, page);
    else {
      munmap(page, PAGE_ALLOCATOR_PAGE_SIZE);
      atomic_dec(&A->free_pages_count);
    }
  }
}

void vpage_allocator_destroy(struct VPageAllocator *A) {
  A->gc_threshold = 0;
  _vpage_allocator_gc(A, ~(uint64_t)0);
}

void _vpage_allocator_collect_garbage(struct VPageAllocator *A) {
  _vpage_allocator_gc(A, A->get_time(A->context));
}

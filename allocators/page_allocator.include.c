#include "page_allocator.h"

#ifndef PAGE_ALLOCATOR_THRESHOLD
#define PAGE_ALLOCATOR_THRESHOLD 64
#endif

void page_allocator_init(struct PageAllocator *A, size_t gc_threshold) {
  *A = (struct PageAllocator){
    .gc_threshold = gc_threshold,

    .free_pages_counter = 0,
    .free_pages = SListInit
  };
}

void page_allocator_destroy(struct PageAllocator *A) {
  A->gc_threshold = 0;
  _page_allocator_collect_garbage(A);
}

void _page_allocator_collect_garbage(struct PageAllocator *A) {
  size_t threshold = atomic_read(&A->gc_threshold);

  while (atomic_read(&A->free_pages_counter) > threshold) {
    void *page = _page_allocator_get_page(A);

    if (!page) return;

    munmap(page, PAGE_ALLOCATOR_PAGE_SIZE);
  }
}

static __attribute__((constructor)) void _page_allocator_check_page_size() {
  if (PAGE_ALLOCATOR_PAGE_SIZE != sysconf(_SC_PAGE_SIZE)) abort();
}

struct PageAllocator page_allocator;

static __attribute__((constructor)) void _pa_init() {
  page_allocator_init(&page_allocator, PAGE_ALLOCATOR_THRESHOLD);
}

static __attribute__((destructor)) void _pa_destroy() { 
  page_allocator_destroy(&page_allocator);
}


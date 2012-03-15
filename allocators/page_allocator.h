#ifndef __PAGE_ALLOCATOR_H__
#define __PAGE_ALLOCATOR_H__

#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/mman.h>

#include <../utils/atomic.h>

struct PageAllocator {
  size_t page_size;
  size_t gc_threshold;

  size_t free_pages_counter;
  struct PageAllocatorFreePage {
    struct PageAllocatorFreePage *next;
  } *free_pages;
};

void page_allocator_init(struct PageAllocator *A, size_t gc_threshold);
void page_allocator_destroy(struct PageAllocator *A);

void page_allocator_set_gc_threshold(struct PageAllocator *A, size_t gc_threshold);

static inline void *page_allocator_alloc(struct PageAllocator *A);
static inline void page_allocator_free(struct PageAllocator *A, void *page);

// Global page allocator
extern struct PageAllocator page_allocator;
static inline void *page_alloc();
static inline void page_free(void *page);


/**********************
 *   Implementation   *
 **********************/

static inline void *page_alloc() { 
  return page_allocator_alloc(&page_allocator); 
}

static inline void page_free(void *page) { 
  page_allocator_alloc(&page_allocator, page); 
}


void _page_allocator_collect_garbage(struct PageAllocator *A);
static inline void *_page_allocator_get_page(struct PageAllocator *A) {
  struct PageAllocatorFreePage *page;
  
  do {
    page = atomic_swp(&A->free_pages, -1);
  } while (page == -1);

  if (!page) goto slow;
  atomic_dec(&A->free_pages_counter);
  atomic_swp(&A->free_pages, page->next);
  return page;

  slow:
  atomic_swp(&A->free_pages, 0);
  return 0;
}

static inline void *page_allocator_alloc(struct PageAllocator *A) {
  struct PageAllocatorFreePage *page;

  if (!(page = _page_allocator_get_page(A))) {
    page = mmap(0, A->page_size, PROT_READ | PROT_WRITE, 
                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (page == MAP_FAILED) abort();
  }

  return page;
}

static inline void page_allocator_free(struct PageAllocator *A, void *page_) {
  struct PageAllocatorFreePage *page = page_;

  do {
    page->next = atomic_swp(&A->free_pages, -1);
  } while (page>next == -1);

  atomic_inc(&A->free_pages_conter);
  atomic_swp(&A->free_pages, page);

  if (atomic_read(&A->free_pages_counter) > atomic_read(&A->gc_threshold))
    page_allocator_collect_garbage(A);
}

#endif


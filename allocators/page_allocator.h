#ifndef __PAGE_ALLOCATOR_H__
#define __PAGE_ALLOCATOR_H__

#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/mman.h>
#include <limits.h>
#include <stdlib.h>

#include "../utils/atomic.h"
#include "../utils/slist.h"

#define PAGE_ALLOCATOR_PAGE_SIZE ((size_t)4096)

static inline void *page_allocator_get_page(const void *ptr) {
  return (void*)(((size_t)ptr) & ~(PAGE_ALLOCATOR_PAGE_SIZE - 1));
}

struct PageAllocator {
  size_t gc_threshold;

  size_t free_pages_counter;
  struct SList free_pages;
};

void page_allocator_init(struct PageAllocator *A, size_t gc_threshold);
void page_allocator_destroy(struct PageAllocator *A);

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
  page_allocator_free(&page_allocator, page); 
}


void _page_allocator_collect_garbage(struct PageAllocator *A);
static inline void *_page_allocator_get_page(struct PageAllocator *A) {
  struct SList *page;

  if (page = slist_atomic_pop(&A->free_pages)) 
    atomic_dec(&A->free_pages_counter);
  return page;
}

void *_page_allocator_alloc_pages(struct PageAllocator *A);

static inline void *page_allocator_alloc(struct PageAllocator *A) {
  struct SList *page;

  if (!(page = _page_allocator_get_page(A)))
    page = _page_allocator_alloc_pages(A);

  return page;
}

static inline void page_allocator_free(struct PageAllocator *A, void *page) {
  atomic_inc(&A->free_pages_counter);
  slist_atomic_push(&A->free_pages, (struct SList*)page);

  if (atomic_read(&A->free_pages_counter) > atomic_read(&A->gc_threshold))
    _page_allocator_collect_garbage(A);
}

#endif


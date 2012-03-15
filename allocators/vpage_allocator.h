#ifndef __VPAGE_ALLOCATOR_H__
#define __VPAGE_ALLOCATOR_H__

#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/mman.h>
#include <limits.h>
#include <stdlib.h>

#include "page_allocator.h"

struct VPageAllocator {
  uint64_t (*get_time)(void*);
  void *get_time_context;

  size_t gc_threshold;

  size_t free_pages_counter;
  SList(uint64_t timestamp) free_pages;
};

void vpage_allocator_init(struct VPageAllocator *A, size_t gc_threshold,
                          uint64_t (*get_time)(void*), void *context);
void vpage_allocator_destroy(struct VPageAllocator *A);

static inline void *vpage_allocator_alloc(struct VPageAllocator *A);
static inline void vpage_allocator_free(struct VPageAllocator *A,
                                        void *page, uint64_t time);


/**********************
 *   Implementation   *
 **********************/

void _vpage_allocator_collect_garbage(struct VPageAllocator *A);
static inline void *_vpage_allocator_get_page(struct VPageAllocator *A) {
  SListItem(&A->free_pages) *page;

  if (page = slist_atomic_pop(&A->free_pages))
    atomic_dec(&A->free_pages_counter);
  return page;
}

static inline void *vpage_allocator_alloc(struct VPageAllocator *A) {
  struct SList *page;

  if (!(page = _vpage_allocator_get_page(A))) {
    page = mmap(0, PAGE_ALLOCATOR_PAGE_SIZE, PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (page == MAP_FAILED) abort();
  }

  return page;
}

static inline void vpage_allocator_free(struct VPageAllocator *A,
                                        void *page_, uint64_t time) {
  SListItem(&A->free_pages) *page = page_;
  page->timestamp = time;

  atomic_inc(&A->free_pages_counter);
  slist_atomic_push(&A->free_pages, page);

  if (atomic_read(&A->free_pages_counter) > atomic_read(&A->gc_threshold))
    _vpage_allocator_collect_garbage(A);
}

#endif


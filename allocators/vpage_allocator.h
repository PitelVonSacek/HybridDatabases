#ifndef __VPAGE_ALLOCATOR_H__
#define __VPAGE_ALLOCATOR_H__

#include <sys/mman.h>
#include <limits.h>

#include "../utils/basic_utils.h"
#include "page_allocator.h"

struct VPageAllocatorItem {
  struct SList slist;
  uint64_t timestamp;
};

struct VPageAllocator {
  uint64_t (*get_time)(void*);
  void *get_time_context;

  size_t gc_threshold;

  size_t free_pages_count;
  struct SList free_pages;
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
  struct SList *page;

  if (page = slist_atomic_pop(&A->free_pages))
    atomic_dec(&A->free_pages_count);

  return page;
}

static inline void *vpage_allocator_alloc(struct VPageAllocator *A) {
  struct SList *page;

  if (!(page = _vpage_allocator_get_page(A)))
    page = page_alloc();

  return page;
}

static inline void vpage_allocator_free(struct VPageAllocator *A,
                                        void *page_, uint64_t time) {
  struct VPageAllocatorItem *page = page_;
  page->timestamp = time;

  atomic_inc(&A->free_pages_count);
  slist_atomic_push(&A->free_pages, &page->slist);

  if (atomic_read(&A->free_pages_count) > atomic_read(&A->gc_threshold))
    _vpage_allocator_collect_garbage(A);
}

#endif


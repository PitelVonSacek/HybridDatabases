/// @file
/// @brief Implementace @ref page_allocator.h.

#include "page_allocator.h"

#ifndef PAGE_ALLOCATOR_THRESHOLD
#define PAGE_ALLOCATOR_THRESHOLD 2048 // 8 MB
#endif

void page_allocator_init(struct PageAllocator *A, size_t gc_threshold) {
  *A = (struct PageAllocator){
    .gc_threshold = gc_threshold > 2 ? gc_threshold : 2,

    .free_pages_counter = 0,
    .free_pages = SListInit
  };

#if PAGE_ALLOCATOR_SERIALIZE_MMAP
  pthread_mutex_init(&A->mutex, 0);
#endif
}

void page_allocator_destroy(struct PageAllocator *A) {
  A->gc_threshold = 0;
  _page_allocator_collect_garbage(A);

#if PAGE_ALLOCATOR_SERIALIZE_MMAP
  pthread_mutex_destroy(&A->mutex);
#endif
}

void _page_allocator_collect_garbage(struct PageAllocator *A) {
  size_t threshold = atomic_read(&A->gc_threshold);

  while (atomic_read(&A->free_pages_counter) > threshold) {
    void *page = _page_allocator_get_page(A);

    if (!page) return;

    munmap(page, PAGE_ALLOCATOR_PAGE_SIZE);
  }
}

void *_page_allocator_alloc_pages(struct PageAllocator *A) {
  void *block;
#if PAGE_ALLOCATOR_SERIALIZE_MMAP
  pthread_mutex_lock(&A->mutex);
  block = _page_allocator_get_page(A);
  if (!block) {
#endif

  size_t pages = PAGE_ALLOCATOR_PAGE_SIZE * A->gc_threshold /
                 PAGE_ALLOCATOR_ALLOC_RATIO ?: 1;

  block = mmap(0, pages, PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  if (block == MAP_FAILED) utilDie("mmap failed");

  for (int i = 1; i < A->gc_threshold / 2; i++)
    page_allocator_free(A, util_apply_offset(block, i * PAGE_ALLOCATOR_PAGE_SIZE));

#if PAGE_ALLOCATOR_SERIALIZE_MMAP
  }
  pthread_mutex_unlock(&A->mutex);
#endif

  return block;
}

/// Kontrola velikosti stránky při startu aplikace.
static __attribute__((constructor)) void _page_allocator_check_page_size() {
  assert(PAGE_ALLOCATOR_PAGE_SIZE == sysconf(_SC_PAGE_SIZE));
}

struct PageAllocator page_allocator;

/// Kontruktor #page_allocator.
static __attribute__((constructor)) void _pa_init() {
  page_allocator_init(&page_allocator, PAGE_ALLOCATOR_GC_THRESHOLD);
}

/// Destruktor #page_allocator.
static __attribute__((destructor)) void _pa_destroy() { 
  page_allocator_destroy(&page_allocator);
}


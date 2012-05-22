#ifndef __PAGE_ALLOCATOR_H__
#define __PAGE_ALLOCATOR_H__

/**
 * @file
 * @brief Jednoduchý alokátor stránek paměti.
 *
 * Umožňuje alokovat paměť po stránkách. Uvolněné stránky nepředáva
 * ihned zpět systému, ale odkládá si je do bufferu pro další použití.
 * Odložené stránky jsou uvolněny pokud jejich množství přeroste
 * atribut @c gc_threshold.
 */

#include <sys/mman.h>
#include <limits.h>

#include "../database.types/enums.h"

#include "../utils/basic_utils.h"
#include "../utils/atomic.h"
#include "../utils/slist.h"


/**
 * 1 pokud má být mmap() serializován pomocí mutexů.
 * (paralelní vykonámí mmapu může způsobit alokaci příliš mnoha paměti,
 * takže se spustí gc)
 */
#ifndef PAGE_ALLOCATOR_SERIALIZE_MMAP
#define PAGE_ALLOCATOR_SERIALIZE_MMAP 1
#endif

/**
 * Maximum cachovaných stránek pro globální instanci PageAllocatoru
 */
#ifndef PAGE_ALLOCATOR_GC_THRESHOLD
#define PAGE_ALLOCATOR_GC_THRESHOLD 2048 // 8 MB
#endif

/**
 * Určuje po kolika se mají alokovat stránky od systému.
 * Alokuje se vždy <tt>.gc_threshold / PAGE_ALLOCATOR_ALLOC_RATIO</tt> stránek.
 */
#ifndef PAGE_ALLOCATOR_ALLOC_RATIO
#define PAGE_ALLOCATOR_ALLOC_RATIO 2
#endif

#if PAGE_ALLOCATOR_SERIALIZE_MMAP
#include <pthread.h>
#endif

/// Pevně zadrátovaná velikost stránky, protože ji potřebujeme již při kompilaci.
#define PAGE_ALLOCATOR_PAGE_SIZE ((size_t)4096)

/// Vrátí adresu stránky, do níž patří @a ptr.
static inline void *page_allocator_get_page(const void *ptr) {
  return (void*)(((size_t)ptr) & ~(PAGE_ALLOCATOR_PAGE_SIZE - 1));
}


/// Alokátor stránek.
struct PageAllocator {
  size_t gc_threshold;

  size_t free_pages_counter;
  struct SList free_pages;

#if PAGE_ALLOCATOR_SERIALIZE_MMAP
  pthread_mutex_t mutex;
#endif
};


/// Inicializuje #PageAllocator.
void page_allocator_init(struct PageAllocator *A, size_t gc_threshold);
/// Zničí #PageAllocator.
void page_allocator_destroy(struct PageAllocator *A);


/// Alokuje stránku.
static inline void *page_allocator_alloc(struct PageAllocator *A);
/// Uvolí stránku.
static inline void page_allocator_free(struct PageAllocator *A, void *page);

/// Vyčistí vnitřní buffer.
static inline void page_allocator_collect_garbage(struct PageAllocator *A);


/// Globální instance PageAllocatoru.
extern struct PageAllocator page_allocator;

/// Zkratka za <tt>page_allocator_alloc(&page_allocator)</tt>.
static inline void *page_alloc();
/// Zkratka za <tt>page_allocator_free(&page_allocator, ptr)</tt>.
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


/// Vrátí paměť z buferu zpět systému.
void _page_allocator_collect_garbage(struct PageAllocator *A, size_t threshold);

/// Vrátí stránku z vnitřínho bufferu.
static inline void *_page_allocator_get_page(struct PageAllocator *A) {
  struct SList *page;

  if (page = slist_atomic_pop(&A->free_pages)) 
    atomic_dec(&A->free_pages_counter);
  return page;
}

/// Alokuje blok stránek ze systému.
void *_page_allocator_alloc_pages(struct PageAllocator *A);

static inline void *page_allocator_alloc(struct PageAllocator *A) {
  struct SList *page;

  if (!(page = _page_allocator_get_page(A)))
    page = _page_allocator_alloc_pages(A);

  return page;
}

static inline void page_allocator_free(struct PageAllocator *A, void *page) {
  atomic_inc(&A->free_pages_counter);
  slist_atomic_push(&A->free_pages, page);

  if (atomic_read(&A->free_pages_counter) > atomic_read(&A->gc_threshold))
    _page_allocator_collect_garbage(A, atomic_read(&A->gc_threshold) / 2);
}

static inline void page_allocator_collect_garbage(struct PageAllocator *A) {
  _page_allocator_collect_garbage(A, 0);
}

#endif


#ifndef __FAST_STACK_H__
#define __FAST_STACK_H__

/**
 * @file
 * @brief Rychlá implementace zásobníku.
 *
 * Implementace zásobníku, která alokuje paměť po blocích
 * (přesněji po stránkách), takže nedochází k realokaci.
 * K alokaci paměti užívá PageAllocator.
 *
 * Bloky drží ve spojáku @c blocks. Každý blok má jako
 * hlavičku List, pak následují data.
 *
 * Ukazatele @c begin, @c ptr a @c end ukazují na odpovídající
 * místa posledního bloku.
 *
 * Je-li zásobník vyprázdněn, ponechává si jeden blok paměti
 * alokovaný (protože přechod mezi jedním a nula prvky je velmi častý).
 */

#include <stddef.h>

#include "../allocators/page_allocator.h"
#include "list.h"

typedef struct {} IsFastStack;

/// Definuje typ zásobníku obsahující prvky typu @c Type.
#define FastStack(Type) \
  struct { \
    size_t block_count; \
    struct List blocks; \
    Type *begin, *ptr, *end; \
    \
    struct { \
      struct List head; \
      Type data[(PAGE_ALLOCATOR_PAGE_SIZE - sizeof(struct List)) / sizeof(Type)]; \
    } __block[0]; \
    IsFastStack is_fast_stack; \
    struct { \
      char _non_empty_block[((PAGE_ALLOCATOR_PAGE_SIZE - sizeof(struct List)) / \
          sizeof(Type) > 0) ? 1 : -1]; \
    } _non_empty_block_assert[0]; \
  }


/// Inicializuje zásobník.
#define fstack_init(stack) \
  ({ \
    typeof(&*(stack)) __stack = (stack); \
    STATIC_ASSERT(types_equal(typeof(__stack->is_fast_stack), IsFastStack)); \
    *__stack = (typeof(*__stack)){ \
      .block_count = 0, \
      .blocks = ListInit(__stack->blocks), \
      \
      .begin = (void*)(((char*)(&__stack->blocks)) + _fstack_offset_begin), \
      .ptr = (void*)(((char*)(&__stack->blocks)) + _fstack_offset_end), \
      .end = (void*)(((char*)(&__stack->blocks)) + _fstack_offset_end) \
    }; \
    __stack; \
  })

/// Uvolní paměť používanou zásobníkem.
#define fstack_destroy(stack) \
  ({ \
    typeof(&*(stack)) __stack_ = (stack); \
    STATIC_ASSERT(types_equal(typeof(__stack_->is_fast_stack), IsFastStack)); \
    while (!list_empty(&__stack_->blocks)) \
      page_free(list_remove(__stack_->blocks.next)); \
    fstack_init(__stack_); \
  })


/// Počet prvků v jednom bloku.
#define fstack_block_size(stack) \
  (sizeof((stack)->__block->data)/sizeof((stack)->__block->data[0]))


/// Vrátí @c true, je-li zásobník prázdný.
#define fstack_empty(stack) \
  ({ \
    STATIC_ASSERT(types_equal(typeof((stack)->is_fast_stack), IsFastStack)); \
    (!((stack)->block_count)); \
  })

/// Počet prvků v zásobníku.
#define fstack_size(stack) \
  ({ \
    typeof(&*(stack)) __stack = (stack); \
    STATIC_ASSERT(types_equal(typeof(__stack->is_fast_stack), IsFastStack)); \
    (__stack->block_count - 1) * fstack_block_size(__stack) + \
      (__stack->ptr - __stack->begin); \
  })


/// Vymaže všechny prvky.
#define fstack_erase(stack) \
  do { \
    typeof(&*(stack)) __stack = (stack); \
    STATIC_ASSERT(types_equal(typeof(__stack->is_fast_stack), IsFastStack)); \
    while (__stack->block_count) \
      _fstack_shrink(utilCast(GenericFastStack, __stack), _fstack_offsets); \
  } while (0)


/// Vloží prvek na vrchol zásobníku.
#define fstack_push(stack, item) \
  ({ \
    typeof(&*(stack)) __stack = (stack); \
    STATIC_ASSERT(types_equal(typeof(__stack->is_fast_stack), IsFastStack)); \
    if (__stack->ptr == __stack->end) \
      _fstack_expand(utilCast(GenericFastStack, __stack), _fstack_offsets); \
    *__stack->ptr++ = (item); \
    (void)0; \
  })

/// Přečte prvek z vrcholu zásobníku.
#define fstack_top(stack) \
  (*({ \
    STATIC_ASSERT(types_equal(typeof((stack)->is_fast_stack), IsFastStack)); \
    (stack)->ptr - 1; \
  }))

/// Odstraní prvek z vrcholu zásobníku.
#define fstack_pop(stack) \
  do { \
    typeof(&*(stack)) __stack = (stack); \
    STATIC_ASSERT(types_equal(typeof(__stack->is_fast_stack), IsFastStack)); \
    if (--__stack->ptr == __stack->begin) \
      _fstack_shrink(utilCast(GenericFastStack, __stack), _fstack_offsets); \
  } while (0)


/// Prohodí dva zásobníky.
#define fstack_swap(s1, s2) \
  ({ \
    typeof(&*(s1)) __stack1 = (s1); \
    typeof(&*(s2)) __stack2 = (s2); \
    typeof(*(s1)) __stack = *__stack1; \
    STATIC_ASSERT(types_equal(typeof(__stack1->is_fast_stack), IsFastStack)); \
    STATIC_ASSERT(types_equal(typeof(__stack2->is_fast_stack), IsFastStack)); \
    *__stack1 = *__stack2; \
    *__stack2 = __stack; \
    if (__stack1->blocks.next != &__stack2->blocks) { \
      __stack1->blocks.next->prev = &__stack1->blocks; \
      __stack1->blocks.prev->next = &__stack1->blocks; \
    } else { \
      __stack1->blocks.next = &__stack1->blocks; \
      __stack1->blocks.prev = &__stack1->blocks; \
    } \
    if (__stack2->blocks.next != &__stack1->blocks) { \
      __stack2->blocks.next->prev = &__stack2->blocks; \
      __stack2->blocks.prev->next = &__stack2->blocks; \
    } else { \
      __stack2->blocks.next = &__stack2->blocks; \
      __stack2->blocks.prev = &__stack2->blocks; \
    } \
    (void)0; \
  })


/// Cyklus přes všechny prvky zásobníku odspodu.
/// @warning Nefungují v něm @c continue a @c break.
#define fstack_for_each(var, stack) \
  if (!stack->block_count) {} else \
    list_for_each_item(__block, &stack->blocks, typeof(stack->__block[0]), head) \
      for (typeof(__block->data[0])* var = __block->data, \
           *__last = ((__block->head.next == &stack->blocks) ? \
             stack->ptr : var + fstack_block_size(stack) ); \
           var < __last; var++)


////////////////////////////////////////////////////////////////////////////////

// Implementační detaily

#define _fstack_offsets _fstack_offset_begin, _fstack_offset_end 
#define _fstack_offset_begin  offsetof(typeof(__stack->__block[0]), data[0])
#define _fstack_offset_end  offsetof(typeof(__stack->__block[0]), data[fstack_block_size(__stack)])

typedef FastStack(void*) GenericFastStack;

// Přidá do zásobníku blok paměti.
static __attribute__((unused))
void _fstack_expand(GenericFastStack *stack,
                    ptrdiff_t offset_begin, ptrdiff_t offset_end) {
  stack->block_count++;

  if (stack->block_count != 1 || list_empty(&stack->blocks))
    list_add_end(&stack->blocks, page_alloc());

  stack->begin = (void**)(((char*)(stack->blocks.prev)) + offset_begin);
  stack->ptr = stack->begin; 
  stack->end = (void**)(((char*)(stack->blocks.prev)) + offset_end);
}

// Odebere ze zásobníku blok paměti.
static __attribute__((unused))
void _fstack_shrink(GenericFastStack *stack,
                    ptrdiff_t offset_begin, ptrdiff_t offset_end) {
  stack->block_count--;

  if (stack->block_count) page_free(list_remove(stack->blocks.prev));

  stack->begin = (void**)(((char*)(stack->blocks.prev)) + offset_begin);
  stack->end = (void**)(((char*)(stack->blocks.prev)) + offset_end);
  stack->ptr = stack->end;
}

#endif


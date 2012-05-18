#ifndef __INLINE_STACK_H__
#define __INLINE_STACK_H__

/**
 * @file
 * @brief Implementace zásobníku s omezenou maximální velikostí.
 *
 * Celý zásobník je schován v datové struktuře, nedochází k žádné dynamické
 * alokaci paměti.
 */

#include <stddef.h>

/// Definuje typ zásobníku prvků daného typu a dané velikosti.
#define InlineStack(Type, Size) \
  struct { \
    ssize_t ptr; \
    Type data[Size]; \
  }

/// Inicializátor pro proměnné typu InlineStack().
#define InlineStackInit { 0, {} }

/// @c true pokud je zásobník prázdný.
#define istack_empty(stack) (!((stack)->ptr))

/// Vrací velikost zásobníku.
#define istack_size(stack) ((stack)->ptr)
/// Vrací kapacitu zásobníku.
#define istack_capacity(stack) \
  (sizeof((stack)->data) / sizeof((stack)->data[0]))

/// Vymaže všechny prvky ze zásobníku.
#define istack_erase(stack) \
  do { (stack)->ptr = 0; } while (0)


// Není-li kontrola přetečení nijak nastavena, zapneme ji.
#ifndef ISTACK_CHECK_BOUNDS
#define ISTACK_CHECK_BOUNDS 1
#endif

#if ISTACK_CHECK_BOUNDS
#include <assert.h>
#define _istack_check_bounds(stack, offset) \
  do { \
    assert(stack->ptr + offset >= 0); \
    assert(stack->ptr + offset < istack_capacity(stack)); \
  } while (0)
#else
#define _istack_check_bounds(stack, offset) do {} while (0)
#endif


/// Vloží prvek na zásobník.
#define istack_push(stack, item) \
  ({ \
    typeof(&*(stack)) __stack = (stack); \
    _istack_check_bounds(__stack, 0); \
    __stack->data[__stack->ptr++] = (item); \
    (void)0; \
  })

/// Vrátí prvek z vrcholu zásobníku.
#define istack_top(stack) \
  ({ \
    typeof(&*(stack)) __stack = (stack); \
    _istack_check_bounds(__stack, -1); \
    __stack->data[__stack->ptr - 1]; \
  })

/// Odebere prvek z vrcholu zásobníku a vrátí ho.
#define istack_pop(stack) \
  ({ \
    typeof(&*(stack)) __stack = (stack); \
    _istack_check_bounds(__stack, -1); \
    __stack->data[--__stack->ptr]; \
  })


/// Zmenší data na zásobníku na danou velikost.
/// (Tedy posune ukazatel na vrchol, nemění kapacitu).
#if ISTACK_CHECK_BOUNDS
#define istack_shrink(stack, size) \
  do { \
    typeof(&*(stack)) __stack = (stack); \
    ssize_t __size = (size); \
    assert(__size > 0); \
    assert(__size <= __stack->ptr); \
    __stack->ptr = __size; \
  } while (0)
#else
#define istack_shrink(stack, size) \
  do { (stack)->ptr = (size); } while (0)
#endif


/// Prohodí obsah dvou zásobníku. Pomalé.
#define istack_swap(s1, s2) \
  do { \
    typeof(&*(s1)) __stack1 = (s1); \
    typeof(&*(s2)) __stack2 = (s2); \
    typeof(*(s1)) __stack = *__stack1; \
    *__stack1 = *__stack2; \
    *__stack2 = __stack; \
  } while (0)

#endif


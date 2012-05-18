#ifndef __STACK_H__
#define __STACK_H__

/**
 * @file
 * @brief Jednoduchá implementace zásobníku.
 *
 * Zásobník je implementován jako dynamicky alokované pole.
 * To je dle potřeby realokováno.
 */

#include "basic_utils.h"

/// Prázdná struktura pro typovou kontrolu.
typedef struct {} IsStack;


/// Minimální velikost alokovaného pole.
#define STACK_MIN_SIZE 17

/// Definuje typ zásobníku s prvky daného typu.
#define Stack(Type) struct { \
    typeof(Type) *begin, *ptr, *end; \
    IsStack is_stack; \
  }


/// Incializátor pro proměnné typu Stack().
#define StackInit { 0, 0, 0 }

/// Inicializuje zásobník.
#define stack_init(stack) ({ \
    typeof(&*(stack)) _stack_helper = (stack); \
    STATIC_ASSERT(types_equal(typeof(_stack_helper->is_stack), IsStack)); \
    _stack_helper->begin = _stack_helper->ptr = _stack_helper->end = 0; \
    _stack_helper; \
  })

/// Alokuje a inicializuje zásobník.
#define stack_alloc() \
  ((void*)stack_init((struct GenericStack*)xmalloc(sizeof(GenericStack))))


/// Vloží prvek do zásobníku.
#define stack_push(stack, value) ({ \
    typeof(&*(stack)) _stack_helper = (stack); ({ \
    STATIC_ASSERT(types_equal(typeof(_stack_helper->is_stack), IsStack)); \
    typeof(_stack_helper) _stack = _stack_helper; \
    if (_stack->ptr == _stack->end) \
      _stack_expand(sizeof(*_stack->begin), utilCast(GenericStack, _stack)); \
    *_stack->ptr++ = (value); \
    _stack; \
  });})


/// Vrátí @c true, je-li zásobník prázdný.
#define stack_empty(stack) ({ \
    typeof(&*(stack)) _stack_helper = (stack); ({ \
    STATIC_ASSERT(types_equal(typeof(_stack_helper->is_stack), IsStack)); \
    typeof(_stack_helper) _stack = _stack_helper; \
    _stack->ptr == _stack->begin; \
  });})

/// Vrátí počet prvků v zásobníku.
#define stack_size(stack) ({ \
    typeof(&*(stack)) _stack_helper = (stack); ({ \
    STATIC_ASSERT(types_equal(typeof(_stack_helper->is_stack), IsStack)); \
    typeof(_stack_helper) _stack = _stack_helper; \
    _stack->ptr - _stack->begin; \
  });})

/// Vrátí současnou kapacitu zásobníku (tj. velikost alokovaného pole).
#define stack_capacity(stack) ({ \
    typeof(&*(stack)) _stack_helper = (stack); ({ \
    STATIC_ASSERT(types_equal(typeof(_stack_helper->is_stack), IsStack)); \
    typeof(_stack_helper) _stack = _stack_helper; \
    _stack->end - _stack->begin; \
  });})


/// Vrátí prvek na pozici @c i počítáno odspodu. Nekontroluje meze.
#define stack_at(stack, i) ((stack)->begin[i])


/// Vyprázdní zásobník.
#define stack_erase(stack) ({ \
    typeof(&*(stack)) _stack_helper = (stack); ({ \
    STATIC_ASSERT(types_equal(typeof(_stack_helper->is_stack), IsStack)); \
    typeof(_stack_helper) _stack = _stack_helper; \
    _stack->ptr = _stack->begin; \
    stack; \
  });})


/// Vrátí prvek na vrcholu zásobníku. Nekontroluje podtečení.
#define stack_top(stack) \
  ({ \
    STATIC_ASSERT(types_equal(typeof((stack)->is_stack), IsStack)); \
    (stack)->ptr[-1]; \
  })

/// Vyjme prvek z vrcholu zásobníku a vrátí ho. Nekontroluje podtečení.
#define stack_pop(stack) ({ \
    typeof(&*(stack)) _stack_helper = (stack); ({ \
    STATIC_ASSERT(types_equal(typeof(_stack_helper->is_stack), IsStack)); \
    typeof(_stack_helper) _stack = _stack_helper; \
    if (stack_capacity(_stack) > 2 * stack_size(_stack) && \
        stack_capacity(_stack) > 2 * STACK_MIN_SIZE) \
      _stack_shrink(sizeof(*_stack->begin), utilCast(GenericStack, _stack)); \
    *--_stack->ptr; \
  });})


/// Destruktor zásobníku.
#define stack_destroy(stack) do { free((stack)->begin); } while (0)

/// Provede dynamicky alokovaného pole i zásobníku samotného.
#define stack_free(stack) do { \
    typeof(&*(stack)) _stack_helper = (stack); ({ \
    STATIC_ASSERT(types_equal(typeof(_stack_helper->is_stack), IsStack)); \
    typeof(_stack_helper) _stack = _stack_helper; \
    stack_destroy(_stack); \
    free(_stack); \
  });} while (0)


/// Prohodí obsah dvou zásobníků.
#define stack_swap(stack1, stack2) \
  do { \
    typeof(&*(stack1)) _stack1 = (stack1); \
    typeof(&*(stack2)) _stack2 = (stack2); \
    STATIC_ASSERT(types_equal(typeof(_stack1->is_stack), IsStack)); \
    STATIC_ASSERT(types_equal(typeof(_stack2->is_stack), IsStack)); \
    typeof(_stack1->ptr) _helper = _stack1->begin; \
    _stack1->begin = _stack2->begin; _stack2->begin = _helper; \
    _helper = _stack1->ptr; _stack1->ptr = _stack2->ptr; _stack2->ptr = _helper; \
    _helper = _stack1->end; _stack1->end = _stack2->end; _stack2->end = _helper; \
  } while (0)


/// Cyklus přes všechny prvky zásobníku odspodu.
#define stack_for_each(var, stack) \
  for (typeof(stack->ptr) var = stack->begin; var < stack->ptr; var++)


////////////////////////////////////////////////////////////////////////////////

// Implementační detaily

// Zásobník obsahující znaky, užíván při změně velikosti.
typedef Stack(unsigned char) GenericStack;

// Zvětší zásobník.
static __attribute__((unused))
void _stack_expand(size_t elem_size, GenericStack* stack) {
  size_t new_cap = stack_capacity(stack) * 2 + STACK_MIN_SIZE * elem_size;
  unsigned char *new_stack = xrealloc(stack->begin, new_cap);
  stack->ptr = new_stack + (stack->ptr - stack->begin);
  stack->end = new_stack + new_cap;
  stack->begin = new_stack;
}

// Zmenší zásobník.
static __attribute__((unused))
void _stack_shrink(size_t elem_size, GenericStack* stack) {
  size_t new_cap = ((stack_capacity(stack) + elem_size) / (2 * elem_size)) * elem_size;
  unsigned char *new_stack = xrealloc(stack->begin, new_cap);
  stack->ptr = new_stack + (stack->ptr - stack->begin);
  stack->end = new_stack + new_cap;
  stack->begin = new_stack;
}

#endif


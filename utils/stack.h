#ifndef __STACK_H__
#define __STACK_H__

#include "basic_utils.h"

// simple generic stack implementation

typedef struct {} IsStack;

#define Stack_alloc(x)      xmalloc(x)
#define Stack_free(x)       free(x)
#define Stack_realloc(x, s) xrealloc(x, s)


#define Stack(Type) struct { \
    typeof(Type) *begin, *ptr, *end; \
    IsStack is_stack; \
  }

#define StackInit { 0, 0, 0 }

#define stack_init(stack) ({ \
    typeof(&*(stack)) _stack_helper = (stack); \
    STATIC_ASSERT(types_equal(typeof(_stack_helper->is_stack), IsStack)); \
    _stack_helper->begin = _stack_helper->ptr = _stack_helper->end = 0; \
    _stack_helper; \
  })
#define stack_alloc() \
  ((void*)stack_init((struct GenericStack*)Stack_alloc(sizeof(struct GenericStack))))


#define stack_push(stack, value) ({ \
    typeof(&*(stack)) _stack_helper = (stack); ({ \
    STATIC_ASSERT(types_equal(typeof(_stack_helper->is_stack), IsStack)); \
    typeof(_stack_helper) _stack = _stack_helper; \
    if (_stack->ptr == _stack->end) \
      _stack_expand(sizeof(*_stack->begin), (struct GenericStack*)_stack); \
    *_stack->ptr++ = (value); \
    _stack; \
  });})

#define stack_empty(stack) ({ \
    typeof(&*(stack)) _stack_helper = (stack); ({ \
    STATIC_ASSERT(types_equal(typeof(_stack_helper->is_stack), IsStack)); \
    typeof(_stack_helper) _stack = _stack_helper; \
    _stack->ptr == _stack->begin; \
  });})

#define stack_size(stack) ({ \
    typeof(&*(stack)) _stack_helper = (stack); ({ \
    STATIC_ASSERT(types_equal(typeof(_stack_helper->is_stack), IsStack)); \
    typeof(_stack_helper) _stack = _stack_helper; \
    _stack->ptr - _stack->begin; \
  });})

#define stack_capacity(stack) ({ \
    typeof(&*(stack)) _stack_helper = (stack); ({ \
    STATIC_ASSERT(types_equal(typeof(_stack_helper->is_stack), IsStack)); \
    typeof(_stack_helper) _stack = _stack_helper; \
    _stack->end - _stack->begin; \
  });})

#define stack_erase(stack) ({ \
    typeof(&*(stack)) _stack_helper = (stack); ({ \
    STATIC_ASSERT(types_equal(typeof(_stack_helper->is_stack), IsStack)); \
    typeof(_stack_helper) _stack = _stack_helper; \
    _stack->ptr = _stack->begin; \
    stack; \
  });})

#define stack_top(stack) \
  (*({ \
    STATIC_ASSERT(types_equal(typeof((stack)->is_stack), IsStack)); \
    (stack)->ptr - 1; \
  }))

#define stack_pop(stack) (*({ \
    typeof(&*(stack)) _stack_helper = (stack); ({ \
    STATIC_ASSERT(types_equal(typeof(_stack_helper->is_stack), IsStack)); \
    typeof(_stack_helper) _stack = _stack_helper; \
    if (stack_capacity(_stack) > 2 * stack_size(_stack) && \
        stack_capacity(_stack) > 100) \
      _stack_shrink(sizeof(*_stack->begin), (struct GenericStack*)_stack); \
    --_stack->ptr; \
  });}))

#define stack_destroy(stack) ({ Stack_free((stack)->begin); (void)0; })
#define stack_free(stack) ({ \
    typeof(&*(stack)) _stack_helper = (stack); ({ \
    STATIC_ASSERT(types_equal(typeof(_stack_helper->is_stack), IsStack)); \
    typeof(_stack_helper) _stack = _stack_helper; \
    stack_destroy(_stack); \
    Stack_free(_stack); \
    (void)0; \
  });})

#define stack_swap(stack1, stack2) ({ \
    typeof(&*(stack1)) _stack1 = (stack1); \
    typeof(&*(stack2)) _stack2 = (stack2); \
    STATIC_ASSERT(types_equal(typeof(_stack1->is_stack), IsStack)); \
    STATIC_ASSERT(types_equal(typeof(_stack2->is_stack), IsStack)); \
    typeof(_stack1->ptr) _helper = _stack1->begin; \
    _stack1->begin = _stack2->begin; _stack2->begin = _helper; \
    _helper = _stack1->ptr; _stack1->ptr = _stack2->ptr; _stack2->ptr = _helper; \
    _helper = _stack1->end; _stack1->end = _stack2->end; _stack2->end = _helper; \
    (void)0; \
  })

#define stack_for_each(var, stack) \
  for (typeof(stack->ptr) var = stack->begin; var < stack->ptr; var++)

struct GenericStack { unsigned char *begin, *ptr, *end; IsStack is_stack; };

static __attribute__((unused))
void _stack_expand(size_t elem_size, struct GenericStack* stack) {
  size_t new_cap = stack_capacity(stack) * 2 + 20 * elem_size;
  unsigned char *new_stack = Stack_realloc(stack->begin, new_cap);
  stack->ptr = new_stack + (stack->ptr - stack->begin);
  stack->end = new_stack + new_cap;
  stack->begin = new_stack;
}

static __attribute__((unused))
void _stack_shrink(size_t elem_size, struct GenericStack* stack) {
  size_t new_cap = ((stack_capacity(stack) + elem_size) / (2 * elem_size)) * elem_size;
  unsigned char *new_stack = Stack_realloc(stack->begin, new_cap);
  stack->ptr = new_stack + (stack->ptr - stack->begin);
  stack->end = new_stack + new_cap;
  stack->begin = new_stack;
}

#endif


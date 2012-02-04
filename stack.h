#ifndef __STACK_H__
#define __STACK_H__

#include <stdlib.h>

// simple generic stack implementation

#define AStack(Type) struct { \
    typeof(Type) *begin, *ptr, *end; \
  }

#define StackInit { 0, 0, 0 }

#define stack_init(stack) ({ \
    typeof(&*(stack)) _stack_helper = (stack); \
    _stack_helper->begin = _stack_helper->ptr = _stack_helper->end = 0; \
    _stack_helper; \
  })
#define stack_alloc() \
  ((void*)stack_init((struct GenericStack*)malloc(sizeof(struct GenericStack))))

#define Stack(Type) struct Stack__##Type
#define DefineStack(Type) Stack(Type) { \
    typeof(Type) *begin, *ptr, *end; \
  }

#define stack_push(stack, value) ({ \
    typeof(&*(stack)) _stack_helper = (stack); ({ \
    typeof(_stack_helper) _stack = _stack_helper; \
    if (_stack->ptr == _stack->end) \
      _stack_expand(sizeof(*_stack->begin), (struct GenericStack*)_stack); \
    *_stack->ptr++ = (value); \
    _stack; \
  });})

#define stack_empty(stack) ({ \
    typeof(&*(stack)) _stack_helper = (stack); ({ \
    typeof(_stack_helper) _stack = _stack_helper; \
    _stack->ptr == _stack->begin; \
  });})

#define stack_size(stack) ({ \
    typeof(&*(stack)) _stack_helper = (stack); ({ \
    typeof(_stack_helper) _stack = _stack_helper; \
    _stack->ptr - _stack->begin; \
  });})

#define stack_capacity(stack) ({ \
    typeof(&*(stack)) _stack_helper = (stack); ({ \
    typeof(_stack_helper) _stack = _stack_helper; \
    _stack->end - _stack->begin; \
  });})

#define stack_erase(stack) ({ \
    typeof(&*(stack)) _stack_helper = (stack); ({ \
    typeof(_stack_helper) _stack = _stack_helper; \
    _stack->ptr = _stack->begin; \
    stack; \
  });})

#define stack_top(stack) ((stack)->ptr[-1])

#define stack_pop(stack) (*({ \
    typeof(&*(stack)) _stack_helper = (stack); ({ \
    typeof(_stack_helper) _stack = _stack_helper; \
    if (stack_capacity(_stack) > 2 * stack_size(_stack) && \
        stack_capacity(_stack) > 1000) \
      _stack_shrink(sizeof(*_stack->begin), (struct GenericStack*)_stack); \
    --_stack->ptr; \
  });}))

#define stack_destroy(stack) ({ free((stack)->begin); (void)0; })
#define stack_free(stack) ({ \
    typeof(&*(stack)) _stack_helper = (stack); ({ \
    typeof(_stack_helper) _stack = _stack_helper; \
    stack_destroy(_stack); \
    free(_stack); \
    (void)0; \
  });})

#define stack_swap(stack1, stack2) ({ \
    typeof(&*(stack1)) _stack1 = (stack1); \
    typeof(&*(stack2)) _stack2 = (stack2); \
    typeof(_stack1->ptr) _helper = _stack1->begin; \
    _stack1->begin = _stack2->begin; _stack2->begin = _helper; \
    _helper = _stack1->ptr; _stack1->ptr = _stack2->ptr; _stack2->ptr = _helper; \
    _helper = _stack1->end; _stack1->end = _stack2->end; _stack2->end = _helper; \
    (void)0; \
  })

struct GenericStack { char *begin, *ptr, *end; };

static void _stack_expand(size_t elem_size, struct GenericStack* stack) {
  size_t new_cap = stack_capacity(stack) * 2 + 20 * elem_size;
  char *new_stack = realloc(stack->begin, new_cap);
  stack->ptr = new_stack + (stack->ptr - stack->begin);
  stack->end = new_stack + new_cap;
  stack->begin = new_stack;
}

static void _stack_shrink(size_t elem_size, struct GenericStack* stack) {
  size_t new_cap = ((stack_capacity(stack) + elem_size) / (2 * elem_size)) * elem_size;
  char *new_stack = realloc(stack->begin, new_cap);
  stack->ptr = new_stack + (stack->ptr - stack->begin);
  stack->end = new_stack + new_cap;
  stack->begin = new_stack;
}

#endif


#ifndef __INLINE_STACK_H__
#define __INLINE_STACK_H__



#define InlineStack(Type, Size) \
  struct { \
    size_t ptr;
    Type data[Size];
  }

#define InlineStackInit { 0, {} }

#define istack_empty(stack) (!((stack)->ptr))

#define istack_size(stack) ((stack)->ptr)
#define istack_capacity(stack) \
  (sizeof((stack)->data) / sizeof((stack)->data[0]))

#define istack_erase(stack) \
  ({ (stack)->ptr = 0; (void)0; })


/*
 * Fast implementation omiting bound checking.
 * For debuging use slower version with checks enabled
 */

#ifndef FSTACK_CHECK_BOUNDS

#define istack_push(stack, item) \
  ({ \
    typeof(&*(stack)) __stack = (stack); \
    __stack->data[__stack->ptr++] = (item); \
    (void)0; \
  })

#define istack_top(stack) \
  ({ \
    typeof(&*(stack)) __stack = (stack); \
    __stack->data[__stack->ptr - 1]; \
  })


#define istack_pop(stack) \
  ({ \
    typeof(&*(stack)) __stack = (stack); \
    __stack->data[--__stack->ptr]; \
  })

#define istack_shrink(stack, size) \
  ({ \
    (stack)->ptr = (size); \
    (void)0; \
  })

#else // bound checking on

#define istack_push(stack, item) \
  ({ \
    typeof(&*(stack)) __stack = (stack); \
    __stack->data[__stack->ptr++] = (item); \
    (void)0; \
  })

#define istack_top(stack) \
  ({ \
    typeof(&*(stack)) __stack = (stack); \
    __stack->data[__stack->ptr - 1]; \
  })


#define istack_pop(stack) \
  ({ \
    typeof(&*(stack)) __stack = (stack); \
    __stack->data[--__stack->ptr]; \
  })

#define istack_shrink(stack, size) \
  ({ \
    (stack)->ptr = (size); \
    (void)0; \
  })

#endif // bound checking


#define istack_swap(s1, s2) \
  ({ \
    typeof(&*(s1)) __stack1 = (s1); \
    typeof(&*(s2)) __stack2 = (s2); \
    typeof(*(s1)) __stack = *__stack1; \
    *__stack1 = *__stack2; \
    *__stack2 = __stack; \
    (void)0; \
  })


#endif


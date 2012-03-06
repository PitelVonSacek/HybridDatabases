#ifndef __FAST_STACK_H__
#define __FAST_STACK_H__

#include <stddef.h>

#include "../node_allocator.h"
#include "list.h"

#define FastStack(Type, BlockSize) \
  struct { \
    struct NodeAllocatorInfo *allocator; \
    size_t block_count; \
    struct List blocks; \
    Type *begin, *ptr, *end; \
    \
    struct { \
      struct List head; \
      Type data[BlockSize]; \
    } __block[0]; \
  }

#define fstack_init(stack, alloc) \
  ({ \
    typeof(&*(stack)) __stack = (stack); \
    *__stack = (typeof(*__stack)){ \
      .allocator = alloc, \
      .block_count = 0, \
      .blocks = ListInit(__stack->blocks), \
      \
      .begin = (void*)(((char*)(&__stack->blocks)) + _fstack_offset_begin), \
      .ptr = (void*)(((char*)(&__stack->blocks)) + _fstack_offset_end), \
      .end = (void*)(((char*)(&__stack->blocks)) + _fstack_offset_end) \
    }; \
    __stack; \
  })

#define fstack_destroy(stack) \
  ({ \
    typeof(&*(stack)) __stack_ = (stack); \
    while (!list_empty(&__stack_->blocks)) \
      node_free(__stack_->allocator, list_remove(__stack_->blocks.next), 0); \
    fstack_init(__stack_, __stack_->allocator); \
  })

#define fstack_block_size(stack) \
  (sizeof((stack)->__block->data)/sizeof((stack)->__block->data[0]))

#define fstack_empty(stack) (!((stack)->block_count))

#define fstack_size(stack) \
  ({ \
    typeof(&*(stack)) __stack = (stack); \
    (__stack->block_count - 1) * fstack_block_size(__stack) + \
      (__stack->ptr - __stack->begin); \
  })

#define fstack_erase(stack) fstack_destroy(stack)


#define fstack_push(stack, item) \
  ({ \
    typeof(&*(stack)) __stack = (stack); \
    if (__stack->ptr == __stack->end) \
      _fstack_expand((GenericFastStack*)__stack, _fstack_offsets); \
    *__stack->ptr++ = (item); \
    (void)0; \
  })

#define fstack_top(stack) ((stack)->ptr[-1])

#define fstack_pop(stack) \
  ({ \
    typeof(&*(stack)) __stack = (stack); \
    if (--__stack->ptr == __stack->begin) \
      _fstack_shrink((GenericFastStack*)__stack, _fstack_offsets); \
    (void)0; \
  })


// must fix block pointers :-(
#define fstack_swap(s1, s2) \
  ({ \
    typeof(&*(s1)) __stack1 = (s1); \
    typeof(&*(s2)) __stack2 = (s2); \
    typeof(*(s1)) __stack = *__stack1; \
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


// WARNING: fstack_for_each actualy contains two nested for cycles
// so break DOES NOT work as expected, instead use goto
#define fstack_for_each(var, stack) \
  list_for_each_item(__block, &stack->blocks, typeof(stack->__block[0]), head) \
    for (typeof(__block->data[0])* var = __block->data, \
         *__last = ((__block->head.next == &stack->blocks) ? \
           stack->ptr : var + fstack_block_size(stack) ); \
         var < __last; var++)


#define _fstack_offsets _fstack_offset_begin, _fstack_offset_end 
#define _fstack_offset_begin  offsetof(typeof(__stack->__block[0]), data[0])
#define _fstack_offset_end  offsetof(typeof(__stack->__block[0]), data[fstack_block_size(__stack)])

typedef FastStack(void*, 1) GenericFastStack;

static void _fstack_expand(GenericFastStack *stack, 
                           ptrdiff_t offset_begin, ptrdiff_t offset_end) {
  stack->block_count++; 
  list_add_end(&stack->blocks, node_alloc(stack->allocator)); 
  stack->begin = (void**)(((char*)(stack->blocks.prev)) + offset_begin);
  stack->ptr = stack->begin; 
  stack->end = (void**)(((char*)(stack->blocks.prev)) + offset_end);
}

static void _fstack_shrink(GenericFastStack *stack, 
                           ptrdiff_t offset_begin, ptrdiff_t offset_end) {
  stack->block_count--;
  node_free(stack->allocator, list_remove(stack->blocks.prev), 0);
  stack->begin = (void**)(((char*)(stack->blocks.prev)) + offset_begin);
  stack->end = (void**)(((char*)(stack->blocks.prev)) + offset_end);
  stack->ptr = stack->end;
}

#endif


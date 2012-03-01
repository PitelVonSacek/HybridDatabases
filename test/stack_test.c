#include <stdio.h>

#include "stack.h"


typedef Stack(int) Stack_int;

void main() {
  Stack_int s[1] = { StackInit };

  stack_push(stack_push(s, 1), 2);

  for (int i = 100; i > 0; i--) stack_push(s, i);

  for (; !stack_empty(s); stack_pop(s)) {
    ;
    printf("%3i %3i %3i\n", (int)stack_capacity(s), (int)stack_size(s), stack_top(s));
  }

  stack_destroy(s);
}


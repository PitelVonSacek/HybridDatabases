#include <stdio.h>
#include "../utils/fast_stack.h"

#define MAX 100

typedef FastStack(int, 10) TSt;

void main() {
  TSt S[1];
  
  struct NodeAllocatorInfo allocator = {
    sizeof(S->__block[0]), 0, 0
  };

  fstack_init(S, &allocator);

  for (int i = 0; i < MAX; i++) fstack_push(S, i);

  int sum = 0;

  fstack_for_each(i, S) sum += *i;

  printf("sum: %i\n", sum);

  while (!fstack_empty(S)) {
    printf("> %i\n", fstack_top(S));
    fstack_pop(S);
  }

  fstack_destroy(S);

  node_free_nodes(&allocator, 0, 0);
}


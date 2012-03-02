#include <stdio.h>
#include "../node_allocator.h"

#define MAX 10000
#define SIZE 24

void *pole[MAX];
size_t ptr = 0;

void main() {
  struct NodeAllocatorInfo allocator = {
    SIZE, 0, 0
  };
  
  for (int i = 0; i < MAX / 2; i++) pole[ptr++] = node_alloc(&allocator);

  while (ptr) node_free(&allocator, pole[--ptr], 0);

  node_free_nodes(&allocator, 0, 0);
}


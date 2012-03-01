#include "node_allocator.h"
#include "stdlib.h"
#include <time.h>

void *node_alloc_nodes(struct NodeAllocatorInfo *info) {
  // TODO: do something smarter, allocate bunch of nodes at once
  return malloc(info->size);
}

#define min_delay 1000      // 1 μs
#define max_delay 100000000 // 0.1 s

bool node_free_nodes(struct NodeAllocatorInfo *info, size_t limit, uint64_t older_than) {
  struct FreeNode *node;
  struct timespec delay = { .tv_sec = 0, .tv_nsec = min_delay };

  while (atomic_read(&(info->counter)) > limit) {
    loop:  
    do {
      node = atomic_swap(&info->free_nodes, (struct FreeNode*)(char*)1);
    } while ((size_t)node == 1);
    
    if (node) {
      atomic_swp(&(info->free_nodes), node->next);
      
      if (node->timestamp > older_than) goto too_new_nodes; // node too new
       
      atomic_dec(&(info->counter));
      free(node);
    } else {
      nanosleep(&delay, 0);
      delay.tv_nsec *= 2;
      if (delay.tv_nsec > max_delay) delay.tv_nsec = max_delay;
    }
  }

  return true;
  
  too_new_nodes:
  // ugly solution, we steal whole list and then return too new items
  // back into the list
  do {
    node->next = atomic_swp(&info->free_nodes, 0);
  } while ((size_t)node->next == 1);

  while (node)  {
    struct FreeNode *n = node->next;
    if (node->timestamp > older_than) _node_free(info, node);
    else {
      atomic_dec(&info->counter);
      free(node);
    }
    node = n;
  }

  return atomic_read(&info->counter) <= limit;
}

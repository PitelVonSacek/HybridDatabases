#ifndef __NODE_H__
#define __NODE_H__

#include "../storage/storage.h"

#include "../node_allocator.h"
#include "../generic_allocator.h"

#include "../utils/list.h"
#include "../utils/num_dictionary.h"
#include "../utils/type_magic.h"

#include "enums.h"

struct NodeAttribute {
  const char * name;
  short type;
  short index;
  int offset;
};

#define Node struct Node_
#define Handler struct Handler_
struct Node_;
struct Handler_;

typedef NumDictionary(uint64_t, Node*) IdToNode;

typedef struct {
  const char * name; // this pointer is used as unique id of NodeType

  bool (*load)(Reader*, struct GenericAllocatorInfo*, Node*);
  void (*store)(Writer*, Node*);

  void (*init_pointers)(IdToNode*, Node*);

  // changes all pointers in node to 0, required for deleting node
  int (*destroy_pointers)(Handler*, Node*);

  void (*init)(Node*);
  void (*destroy)(struct GenericAllocatorInfo*, Node*, uint64_t end_time);

  struct NodeAllocatorInfo allocator_info[1];

  bool (*update_indexes)(Handler*, enum CallbackEvent, Node*);

  int size;
  int extra_space;
  int id;

  int attributes_count;
  const struct NodeAttribute *attributes;
} NodeType;
#undef Node
#undef Handler

typedef struct Node_ {
  NodeType * const type; // have to be first

  // linked list of nodes
  struct List __list;

  // node id
  const uint64_t id;
  // # of references from other nodes
  // node can be deleted only if ref_count == 0
  uint64_t ref_count;

  DummyAncestor __ancestor;
} Node;

static inline bool _node_ref_count_increase(Node *node) {
  atomic_inc(&node->ref_count);
}

static inline bool _node_ref_count_decrease(Node *node) {
  uint64_t old = atomic_fetch_and_add(&node->ref_count, -1);
  assert(old > 0);
}

#endif


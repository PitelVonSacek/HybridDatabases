#ifndef __NODE_H__
#define __NODE_H__

#include "../utils/basic_utils.h"
#include "../storage/storage.h"

#include "../allocators/node_allocator.h"
#include "../allocators/generic_allocator.h"

#include "../utils/num_dictionary.h"

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

typedef struct NodeType_ {
  const char * name; // this pointer is used as unique id of NodeType

  bool (*load)(Reader*, struct GenericAllocator*, Node*);
  void (*store)(Writer*, Node*);

  void (*init_pointers)(IdToNode*, Node*);

  // changes all pointers in node to 0, required for deleting node
  bool (*destroy_pointers)(Handler*, Node*);

  void (*init)(Node*);
  void (*destroy)(struct GenericAllocator*, Node*, uint64_t end_time);

  struct NodeAllocator allocator[1];

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
  union {
    uint64_t ref_count;
    struct SList slist;
  };
  const uint64_t id;

  DummyAncestor __ancestor;
} Node;

static inline NodeType *node_get_type(Node *node) {
  return ((struct NodeAllocatorBlock*)page_allocator_get_page(node))->type;
}

/*
  Real node:

typedef struct {
  Node __ancestor;

  attributes

  attribute_indexes;
} MyNode;

extern const NodeType MyNode_desc;
*/

#endif


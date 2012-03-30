#ifndef __NODE_H__
#define __NODE_H__

/// @file

#include "../utils/basic_utils.h"
#include "../utils/slist.h"

typedef struct {
  union {
    uint64_t ref_count;
    struct SList slist;
  };
  uint64_t id; ///< Unikátní id uzlu. Pro každý validní uzel > 0.
#if INPLACE_LOCKS
  Lock lock;
#endif

  DummyAncestor __ancestor; ///< @ref type_magic
} Node;


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


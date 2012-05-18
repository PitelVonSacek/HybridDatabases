#ifndef __DATABASE_TYPES__INDEX_H__
#define __DATABASE_TYPES__INDEX_H__

/// @file

#include "../utils/basic_utils.h"
#include "node.h"
#include "node_type.h"
#include "handle.h"
#include "../allocators/generic_allocator.h"

typedef struct {
  const char* name;
  bool (*callback)(void *context, Handle *H, enum CallbackEvent event, Node *node);

  size_t context_size;
  size_t node_context_size;

  void (*context_init)(void*, struct GenericAllocator*);
  void (*context_destroy)(void*, struct GenericAllocator*);
} IndexType;

#if INPLACE_INDEX_LOCKS
typedef Lock IndexLock;
#else
typedef struct {} IndexLock;
#endif

/*
  Real index:

typedef struct {
  ReturnType1 function_one(void *context, Handle *, Args...);
  ReturnType2 function_two(void *context, Handle *, Args...);
  ...

} MyIndex_functions;

typedef struct {
  IndexType desc;
  MyIndex_functions functions;
} MyIndex_desc_t;

typedef struct {
  MyIndex_context_t context;

  int (*callback)(void *context, Handle *H, enum CallbackEvent event, Node *node);

  MyIndex_functions functions;
} MyIndex_handle_t;

extern const MyIndex_desc_t MyIndex_desc;

#define trIndex(Index, Function, ...) \
  H->my_database->indexes.Index.Function(&H->my_database->indexes.Index.context, \
                                          H, __VA_ARGS__)

*/

#endif


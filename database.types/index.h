#ifndef __DATABASE_TYPES__INDEX_H__
#define __DATABASE_TYPES__INDEX_H__

#include <stdint.h>

#include "node.h"
#include "handler.h"
#include "../allocators/generic_allocator.h"

typedef struct {
  const char* name;
  bool (*callback)(void *context, Handler *H, enum CallbackEvent event, Node *node);

  size_t context_size;
  size_t node_context_size;

  void (*context_init)(void*, struct GenericAllocator*);
  void (*context_destroy)(void*, struct GenericAllocator*);
} IndexType;

/*
  Real index:

typedef struct {
  ReturnType1 function_one(void *context, Handler *, Args...);
  ReturnType2 function_two(void *context, Handler *, Args...);
  ...

} MyIndex_functions;

typedef struct {
  IndexType desc;
  MyIndex_functions functions;
} MyIndex_desc_t;

typedef struct {
  MyIndex_context_t context;

  int (*callback)(void *context, Handler *H, enum CallbackEvent event, Node *node);

  MyIndex_functions functions;
} MyIndex_handler_t;

extern const MyIndex_desc_t MyIndex_desc;

#define trIndex(Index, Function, ...) \
  H->my_database->indexies.Index.Function(&H->my_database->indexies.Index.context, \
                                          H, __VA_ARGS__)

*/

#endif


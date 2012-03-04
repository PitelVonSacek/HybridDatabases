#ifndef __DATABASE_TYPES__INDEX_H__
#define __DATABASE_TYPES__INDEX_H__

#include <stdint.h>

#include "node.h"
#include "handler.h"

typedef struct {
  const char* name;
  int (*callback)(void *context, Handler *H, enum CallbackEvent event, Node *node);

  size_t context_size;
  size_t node_context_size;

  void (*context_init)(void*);
  void (*context_destroy)(void*);
} IndexType;

/*
  Real index:

typedef struct {
  IndexType desc;

  ReturnType1 function_one(void *context, Handler *, Args...);
  ReturnType2 function_two(void *context, Handler *, Args...);
  ...

} MyIndex_desc_t;

typedef struct {
  Context1 context;

  int (*callback)(void *context, Handler *H, enum CallbackEvent event, Node *node);

  ReturnType1 function_one(void *context, Handler *, Args...);
  ReturnType2 function_two(void *context, Handler *, Args...);
} MyIndex_context_t;

#define trIndex(Index, Function, ...) \
  H->my_database->indexies.Index.Function(&H->my_database->indexies.Index.context, \
                                          H, __VA_ARGS__)

*/

#endif


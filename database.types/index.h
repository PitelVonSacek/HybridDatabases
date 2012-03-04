#ifndef __DATABASE_TYPES__INDEX_H__
#define __DATABASE_TYPES__INDEX_H__

#include <stdint.h>

#include "node.h"
#include "handler.h"

typedef struct {
  const char* name;
  int (*callback)(void *context, size_t node_context_offset,
                  Handler *H, enum CallbackEvent event, Node *node);

  size_t context_size;
  size_t node_context_size;

  void (*context_init)(void*);
  void (*context_destroy)(void*);
} IndexType;

struct Index {
  IndexType* desc;
  size_t context_offset;
  size_t node_context_offset;
};

#endif


#ifndef DATABASE_INLINE_H
#define DATABASE_INLINE_H

#include "database_types.h"

static void _tr_undo_log(Transaction* tr, void* data, int size) {
  TrLogItem item;
  item.ptr = data;
  for (int i = 0; i < size; i++) item.data[i] = ((char*)data)[i];
  item.size = size;
  stack_push(tr->write_log, item);
}

#endif // DATABASE_INLINE_H

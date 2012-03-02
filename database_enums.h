#ifndef __DATABASE_ENUMS_H__
#define __DATABASE_ENUMS_H__

#define DB_LOCKS 251 // better than 256 because is prime
// type DB_LOCKS fits in
#define DB_LOCKS_NR_TYPE unsigned char

enum CommitType {
  CT_SYNC = 1,
  CT_ASYNC = 0,
  CT_FORCE_ASYNC = -1
};

enum CallbackEvent {
  CBE_NODE_CREATED,
  CBE_NODE_MODIFIED,
  CBE_NODE_DELETED,
  CBE_NODE_LOADED // emited when creating indexes after loading database
};

enum {
  LI_TYPE_RAW = 1,
  LI_TYPE_NODE_MODIFY,

  LI_TYPE_ATOMIC_RAW,
  LI_TYPE_ATOMIC_NODE_MODIFY,

  LI_TYPE_NODE_ALLOC,
  LI_TYPE_NODE_DELETE,
  LI_TYPE_MEMORY_ALLOC,
  LI_TYPE_MEMORY_DELETE
};

#endif


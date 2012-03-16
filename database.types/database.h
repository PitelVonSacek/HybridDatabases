#ifndef __DATABASE_TYPES_H__
#define __DATABASE_TYPES_H__

#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>
#include <stdlib.h>

#include "../utils/lock.h"
#include "../utils/stack.h"

#include "../utils/type_magic.h"

#include "../allocators/node_allocator.h"
#include "../allocators/generic_allocator.h"

#include "enums.h"
#include "attributes.h"
#include "node.h"
#include "handler.h"
#include "index.h"

struct Handler_;
typedef bool(*UpdateIndexes)(Handler*, enum CallbackEvent, Node*);

struct Database_;
#define Database struct Database_

typedef struct {
  const char *name;
  const char *version;
  size_t size;

  void (*init_indexes)(Database*);
  void (*destroy_indexes)(Database*);

  DummyAncestor __ancestor;

  size_t node_types_count;
  const NodeType *const *node_types;
  const UpdateIndexes *update_indexes;

  size_t indexes_count;
  const IndexType *const *indexes;
} DatabaseType;

/*
 Real DatabaseType:

struct {
  DatabseType desc;
    
  struct {
    bool (*NodeType1_update_indexes)(...);
    ...
  } node_types;

  struct {
    Index1_desc_t *Index1_desc;
    ...
  } indexies;

} MyDatabase_desc_t;

*/

#undef Database
typedef struct Database_ {
  const DatabaseType * const type;
  
  /*
   * Filenames will be:
   * name.desc - database description
   * name.n for n = 1.. - data files
   */
  const char *filename;

  /* nr of data file we're writing in */
  int current_file_index;

  enum {
    DB_READ_ONLY = 1,  // if set, output.file is /dev/null
    DB_CREATE = 2
  } flags;

  /* global time, read by each strarting transaction, increased during commit */
  uint64_t time;
  /* id which will be given to next created node */
  uint64_t node_id_counter;

  Lock locks[DB_LOCKS];

  Handler* __dummy_handler[0];  // for type magic only
  Stack(Handler*) handlers[1];

#ifndef LOCKLESS_COMMIT
  pthread_mutex_t mutex;
#endif

  pthread_t service_thread;
  sem_t service_thread_pause;

  struct {
    FILE *file;
    sem_t counter[1];

    pthread_mutex_t dump_running[1];

    struct OutputList {
      struct OutputList *next;

      enum {
        DB_SERVICE__COMMIT = 1,
        DB_SERVICE__SYNC_COMMIT,
        DB_SERVICE__SYNC_ME,
        DB_SERVICE__START_DUMP,
        DB_SERVICE__CREATE_NEW_FILE,
        DB_SERVICE__COLLECT_GARBAGE,
        DB_SERVICE__PAUSE,
        DB_SERVICE__HANDLER_REGISTER,
        DB_SERVICE__HANDLER_UNREGISTER
      } type;

      uint64_t end_time;
      sem_t *lock;
      union {
        TransactionLog log[1];
        uint64_t *answer;
        Handler *handler;
      } content; // anonymous member would be better but gcc < 4.6
                 // has bug #10676 that prevents using such fields in initializers
    } *head, **tail;
  } output;

  struct VPageAllocator vpage_allocator[1];
  struct GenericAllocatorInfo tm_allocator[1];

  DummyAncestor __ancestor; // required for type magic

  size_t node_types_count;
  NodeType node_types[0];
} Database;

/*
 Real Database:

typedef struct {
  Database __ancestor;

  struct {
    NodeType1 NodeType1_desc;
    NodeType2 NodeType2_desc;
    ...
  } node_types;

  struct {
    Index1_handler_t index1;
    Index1_handler_t index2;
    ...
  } indexes;

} MyDatabase;

extern const DatabaseType MyDatabase_desc;

*/

#endif


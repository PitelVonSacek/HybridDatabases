#ifndef __DATABASE_TYPES_H__
#define __DATABASE_TYPES_H__

#include <pthread.h>
#include <semaphore.h>

#include "../utils.h"
#include "../utils/stack.h"

#include "../utils/type_magic.h"

#include "../node_allocator.h"
#include "../generic_allocator.h"

#include "enums.h"
#include "attributes.h"
#include "node.h"
#include "handler.h"

struct Database_;
#define Database struct Database_

typedef struct {
  const char *name;
  const char *version;
  size_t size;

  void (*init)(Database*);
  void (*indexes_destroy)(Database*);

  size_t node_types_count;
  NodeType *node_types[0];
} DatabaseType;

/*
 Real DatabaseType:

struct {
  DatabseType desc;
    
  struct {
    NodeType *NodeType1_desc;
    bool (*NodeType1_update_indexies)(...);
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

  struct List node_list;

  Handler* __dummy_handler[0];  // for type magic only
  Stack(Handler*) handlers[1];
  pthread_mutex_t handlers_mutex[1];

#ifndef LOCKLESS_COMMIT
  pthread_mutex_t mutex;
#endif

#ifdef SINGLE_SERVICE_THREAD
  pthread_t service_thread;
#else
  pthread_t io_thread;
  pthread_t gc_thread;
#endif

#if defined(SINGLE_SERVICE_THREAD) && !defined(LOCKLESS_COMMIT)
  struct {
    struct NodeAllocatorInfo allocator[1];
    FILE *file;
    sem_t counter[1];

    pthread_mutex_t dump_running[1];

    struct OutputList {
      struct OutputList *next;

      enum {
        DB_SERVICE__COMMIT = 1,
        DB_SERVICE__SYNC_COMMIT,
        DB_SERVICE__START_DUMP,
        DB_SERVICE__CREATE_NEW_FILE
      } type;

      uint64_t end_time;
      sem_t *lock;
      TransactionLog log[1];
      uint64_t *answer;
    } *head, **tail;
  } output;
#else
  struct {
    struct NodeAllocatorInfo allocator[1];

    struct OutputList {
      struct OutputList *next;
      
      uint64_t end_time;

      sem_t *lock;

      union {
        TransactionLog log[1];
        struct {
          void *data;
          size_t size;
        };
      };

      enum {
        DB_OUTPUT__SYNC_COMMIT = 1,
        DB_OUTPUT__DUMP_SIGNAL = 2,
        DB_OUTPUT__CANCELED = 4,
        DB_OUTPUT__READY = 8,
        DB_OUTPUT__RAW = 16,
        DB_OUTPUT__SHUTDOWN = 32
      } flags;
    } *head, **tail;

    Signal io_signal[1];
    Signal garbage_signal[1];

    FILE *file;
  } output;

  struct {
    enum {
      DB_DUMP__IO_THREAD_WAITS = 1,
      DB_DUMP__DO_DUMP = 2,
      DB_DUMP__DUMP_RUNNING = 4,
      DB_DUMP__DO_GC = 8,
      DB_DUMP__GC_RUNNING = 16,
      DB_DUMP__RESUME_IO_THREAD = 32
    } flags;
 
    Signal signal[1];

//    pthread_mutex_t mutex[1]; // to sync gc & io threads
//    pthread_cond_t signal[1]; // signals end of dump
  } dump;
#endif    

  struct GenericAllocatorInfo tm_allocator[1];

  DummyAncestor __ancestor; // required for type magic

  size_t node_types_count;
  NodeType node_types[0];
} Database;

/*
 Real Database:

typedef struct {
  union {
    DatabaseType *type;
    MyDatabase_desc_t *my_type;
    Database __ancestor;
  };

  struct {
    NodeType node_type_1;
    NodeType node_type_2;
    ...
  } node_types;

  struct {
    Context1 index1;
    Context2 index2;
    ...
  } indexies;

} MyDatabase;

*/

#endif


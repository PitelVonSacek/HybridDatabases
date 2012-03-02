#ifndef __DATABASE_TYPES_H__
#define __DATABASE_TYPES_H__

#include "utils.h"
#include "exception.h"
#include "dictionary.h"
#include "utils/stack.h"
#include "storage.h"

#include "database_enums.h"

// extracted from libucw
#include "bitarray.h"

#include <semaphore.h>
#include <pthread.h>



EXCEPTION_DECLARE(TR_FAIL, NONE);
EXCEPTION_DECLARE(TR_MAIN_ABORT, TR_FAIL);
EXCEPTION_DECLARE(TR_ABORT, TR_MAIN_ABORT);

struct Database_;
struct Handler_;
struct Node_;
#define Database struct Database_
#define Handler struct Handler_


enum {
  TRERR_SUCCESS = 0,
  TRERR_COLLISION = -1,
  TRERR_DUMP_RUNNING = -2,
  TRERR_NONZERO_REF_COUNT = -3,
  TRERR_TRANSACTION_RUNNING = -4
};


#include "attributes.h"
#include "node.h"
#include "handler.h"

typedef struct {
  const char* name;
  int (*callback)(void*, Handler*, enum CallbackEvent, Node*);

  size_t context_size;
  void (*context_init)(void*);
  void (*context_destroy)(void*);
} IndexDesc;

struct DatabaseIndex {
  IndexDesc* desc;
  size_t offset;
};

typedef struct {
  const char *name;
  const char *version;
  size_t size;

  void (*indexes_init)(Database*);
  void (*indexes_destroy)(Database*);
  int  (*indexes_update)(Handler*, enum CallbackEvent, Node*);

  size_t node_types_count;
  NodeType *node_types[0];

  // size_t indexes_count;
  // struct DatabaseIndex indexes[0];
} DatabaseType;

enum {
  DB_SHUT_DOWN = 1,
  DB_DO_DUMP = 2,
  DB_DUMP_RUNNING = 4
};




#undef Database
typedef struct Database_ {
  const DatabaseType * const type;
  
  /*
   * Filenames will be:
   * name.desc - database description
   * name.n for n = 1.. - data files
   */
  char * filename;

  /* nr of data file we're writing in */
  int current_file_index;

  enum DatabaseFlags {
  
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

  pthread_mutex_t mutex;

  pthread_t io_thread;
  pthread_t gc_thread;

  struct {
    NodeAllocatorInfo allocator[1];

    struct OutputList {
      struct OutputList *next;
      
      uint64_t end_time;

      sem_t *lock;

      union {
        // FastStack(struct LogItem, 63) log[1];
        typeof(((Handle*)0)->log[0]) log[1];
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
      DB_DUMP__GC_RUNNING = 16
    } flags;
 
    pthread_mutex_t mutex[1]; // to sync gc & io threads
    pthread_cond_t signal[1]; // signals end of dump
  } dump;

  struct GenericAllocator tm_allocator[1];

  struct {} __ancestor; // required for type magic

  size_t node_types_count;
  NodeType node_types[0];
} Database;


#endif


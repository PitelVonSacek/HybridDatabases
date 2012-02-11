#ifndef __DATABASE_TYPES_H__
#define __DATABASE_TYPES_H__

#include "utils.h"
#include "exception.h"
#include "dictionary.h"
#include "stack.h"
#include "storage.h"

// extracted from libucw
#include "bitarray.h"

#include <semaphore.h>
#include <pthread.h>

#define DB_LOCKS 256

EXCEPTION_DECLARE(TR_FAIL, NONE);
EXCEPTION_DECLARE(TR_MAIN_ABORT, TR_FAIL);
EXCEPTION_DECLARE(TR_ABORT, TR_MAIN_ABORT);

struct Database_;
struct Transaction_;
struct Node_;
#define Database struct Database_
#define Transaction struct Transaction_
#define Node struct Node_

typedef Dictionary(uint64_t, Node*, NUMBER) IdToNode;

typedef Stack(Node*) NodeStack;
typedef Stack(void*) VoidStack;

enum {
  TRERR_SUCCESS = 0,
  TRERR_COLLISION = -1,
  TRERR_DUMP_RUNNING = -2,
  TRERR_NONZERO_REF_COUNT = -3,
  TRERR_TRANSACTION_RUNNING = -4
};

enum CallbackEvent {
  CBE_NODE_CREATED,
  CBE_NODE_MODIFIED,
  CBE_NODE_DELETED,
  CBE_NODE_LOADED // emited when creating indexes after loading database
};

typedef struct {
  const char* name;
  unsigned size;
  unsigned id;

  void (*init)(void*);
  void (*destroy)(void*);

  int (*copy)(Transaction*, void* dest, const void* src);

  void (*load)(Reader*, void*);
  void (*store)(Writer*, void*);
} AttributeType;

struct NodeAttribute {
  const char * name;
  const AttributeType *type;
  int index;
  int offset;
};

typedef struct NodeType_ {
  const char * name; // this pointer is used as unique id of NodeType

  size_t my_size;
  size_t size;

  void (*load)(Reader*, Node*);
  void (*store)(Writer*, Node*);

  void (*init_pointers)(IdToNode*, Node*);

  // changes all pointers in node to 0, required for deleting node
  int (*destroy_pointers)(Transaction*, Node*);

  void (*init)(Node*);
  void (*destroy)(Node*);

//  void (*update_indexes)(Transaction*, enum CallbackEvent, Node*);
  int id;

  int attributes_count;
  struct NodeAttribute attributes[0];
} NodeType;

#undef Node
typedef struct Node_ {
  const NodeType * const type; // have to be first

  // linked list of nodes
  struct Node_ *_prev, *_next;

  // node id
  const uint64_t id;
  // # of references from other nodes
  // node can be deleted only if ref_count == 0
  uint64_t ref_count;

  struct {} __node;
} Node;

struct VoidStackList {
  struct VoidStackList *next;
  VoidStack stack[1];
};

struct NodeStackList {
  struct NodeStackList *next;
  NodeStack stack[1];
};

struct TransactionList {
  struct TransactionList* newer;

  struct NodeStackList *node_list;
  struct VoidStackList *void_list;

  uint64_t time;

  /* number of running transactions with start time time
   * atomicaly decrease at commit/rollback & test result,
   * when it's 0, grab database->mutex and do clean up
   */
  size_t running;
};

struct OutputQueue {
  struct OutputQueue* next;
  void *data;
  size_t size;
  int flags;
  sem_t *lock;
};

typedef struct {
  const char* name;
  int (*callback)(void*, Transaction*, enum CallbackEvent, Node*);

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
  int  (*indexes_update)(Transaction*, enum CallbackEvent, Node*);

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

#include "attributes.h"

typedef struct {
  void *ptr;
  short size;
  char data[MAX_ATTR_SIZE];
} TrLogItem;

struct ReadSet {
  // wrapped in structure so it can be easily copied and assigned
  BIT_ARRAY(read_set, DB_LOCKS);
};

typedef struct {
  struct ReadSet read_set;
  size_t write_log, nodes_deleted, memory_new, memory_deleted;
  Node *nodes_new;
  uint64_t writer_pos;
} TrNestedTransaction;

#undef Transaction
typedef struct Transaction_ {
  // const void* indexes_vtable;
  Database *database;

  struct TransactionList *tr_list;

  uint64_t start_time;
  struct ReadSet read_set;

  Stack(TrNestedTransaction) tr_stack[1];

  Stack(TrLogItem) write_log[1];
  Node* nodes_new, *nodes_new_end;
  NodeStack nodes_deleted[1];
  VoidStack memory_new[1];
  VoidStack memory_deleted[1];

  Writer W[1];

  struct {} __transaction; // required for type magic
} Transaction;

#undef Database
typedef struct Database_ {
  const DatabaseType * const type;

  /* global time, read by each strarting transaction, increased during commit */
  uint64_t time;
  /* id which will be given to next created node */
  uint64_t node_id_counter;

  Lock locks[DB_LOCKS];

  /* array of lists of nodes, each member of array is for one node type */
  Node *node_list, *node_list_end;

  /* list with count of running transactions with specific
   * start time, used for dealyed freeing of memory */
  // QUESTION: is list enought, or do I need heap ??
  struct TransactionList *tr_old, *tr_new;

  /*
   * Filenames will be:
   * name.desc - database description
   * name.n for n = 1.. - data files
   */
  char * filename;

  FILE* output;

  /* nr of data file we're writing in */
  int current_file_index;

  /* lock to serialize writing to file */
  int flags, dump_flags;

  pthread_mutex_t mutex;
  pthread_cond_t signal;

  pthread_t gc_thread;

  pthread_t dump_thread;
  pthread_mutex_t dump_mutex;
  pthread_cond_t dump_signal;

  pthread_t io_thread;
//  sem_t oq_size;
  struct OutputQueue *oq_head, *oq_tail;

  struct {} __database; // required for type magic
  Transaction __dummy_transaction[0];
} Database;


#endif


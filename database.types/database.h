#ifndef __DATABASE_TYPES_H__
#define __DATABASE_TYPES_H__

/// @file

#include <pthread.h>
#include <semaphore.h>

#include "../utils/basic_utils.h"
#include "../utils/lock.h"
#include "../utils/stack.h"

#include "../allocators/node_allocator.h"
#include "../allocators/generic_allocator.h"

#include "enums.h"
#include "attributes.h"
#include "node.h"
#include "node_type.h"
#include "handler.h"
#include "index.h"

struct Handler_;
typedef bool(*UpdateIndexes)(Handler*, enum CallbackEvent, Node*);

struct Database_;
#define Database struct Database_


/**
 * @brief Deskriptor typu databáze.
 */
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
  } indexes;

} MyDatabase_desc_t;

*/

struct OutputList {
  struct OutputList *next;

  enum DbService type;

  sem_t *lock;

#if SIMPLE_SERVICE_THREAD || FAST_COMMIT
  sem_t ready;
#endif
#if SIMPLE_SERVICE_THREAD
  Writer W[1];
#else
  uint64_t end_time;
  TransactionLog log[1];
#endif

  uint64_t *answer;
};

#undef Database
/**
 * @brief Databáze, abstraktní třída, předek konkrétních typů databází.
 */
typedef struct Database_ {
  const DatabaseType * const type; ///< Deskriptor typu databáze.
  
  /**
   * @brief Název souborů obsahujících databázi.
   *
   * Schéma databáze se jmenuje <tt>filename.schema</tt>,
   * datové soubory <tt>filename.N</tt> pro @c N = 1, 2, ...
   */
  const char *filename;

  int current_file_index; ///< Číslo datového souboru, do kterého probíhá zápis.

  enum DbFlags flags; ///< Příznaky databáze, k jejich čtení užijte database_get_flags()


  uint64_t time; ///< Globální hodiny
  uint64_t node_id_counter; ///< Nejvyšší id přidělěné nějakému uzlu,
                            ///  novému uzlu bude přidělěno id @c node_id_counter + 1.

#if !INPLACE_NODE_LOCKS || !INPLACE_INDEX_LOCKS
  Lock locks[DB_LOCKS]; ///< Globální pole zámků
#endif

  Handler* __dummy_handler[0]; ///< Nutné pro získání typové informace při výrobě
                               ///  handleru pomocí dbHandlderCreate().
  Stack(Handler*) handlers[1]; ///< Seznam všech handlerů.
  pthread_mutex_t handlers_mutex[1];

  pthread_mutex_t mutex; ///< Zámek chránící vkládání do výstupní fronty.

  pthread_t sync_helper_thread;
  pthread_mutex_t sync_helper_mutex;
  double sync_helper_period;

  pthread_t service_thread;   ///< Thread id servisního vlákna.
  sem_t service_thread_pause; ///< Semafor předávaný servisnímu vláknu ve volání
                              ///  DB_SERVICE__PAUSE.

  int file_desc; ///< Soubor do něhož probíhá zápis.
  sem_t counter[1]; ///< Počítadlo prvků ve výstupní frontě.

  pthread_mutex_t dump_running[1]; ///< Tento zámek je zamčen vždy, když probíhá dump.

  struct OutputList *head, **tail; ///< Výstupní fronta. Skrze ni se předávají
                                   ///  požadavky servisnímu vláknu.

  struct VPageAllocator vpage_allocator[1];
  struct GenericAllocator tm_allocator[1];

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


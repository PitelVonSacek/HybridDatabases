#ifndef __HANDLER_H__
#define __HANDLER_H__

#include <semaphore.h>

#include "../utils/basic_utils.h"

#include "enums.h"
#include "attributes.h"

#include "../utils/fast_stack.h"
#include "../utils/inline_stack.h"
#include "../utils/bit_array.h"

struct LogItem {
  void *ptr;
 
  unsigned type : 3;
  unsigned size : 5;
  unsigned index: 8;
  unsigned offset: 12;
  unsigned attr_type : 4;

  char data_old[MAX_ATTR_SIZE];
  char data_new[MAX_ATTR_SIZE];
};

typedef BitArray(DB_LOCKS) ReadSet;

struct Transaction {
  ReadSet read_set;
  struct LogItem *pos;
  unsigned acquired_locks;
  enum CommitType commit_type;
};

typedef FastStack(struct LogItem) TransactionLog;

struct Database_;

typedef struct Handler_ {
  struct Database_ *database;

  uint64_t start_time;
  ReadSet read_set;

  FastStack(struct Transaction) transactions[1];

  TransactionLog log[1];

  bool allocated;
  enum CommitType commit_type;

  // keep list of acquired lock, so we dont have to iterate
  // through all locks when releasing them
  InlineStack(DB_LOCKS_NR_TYPE, DB_LOCKS) acquired_locks[1];

  sem_t write_finished[1];

  DummyAncestor __ancestor; // required for type magic
} Handler;

/*
  Database type specific handler:

typedef union {
  Database *database;
  MyDatabase *my_database;
  Handler __ancestor;
} MyDatabase_handler_t;

*/

#endif


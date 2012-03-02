#ifndef __HANDLER_H__
#define __HANDLER_H__

#include <string.h>

#include "database_enums.h"
#include "attributes/attributes_defs.h"

#include "utils/fast_stack.h"
#include "utils/inline_stack.h"
#include "utils/bitarray.h"

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

struct ReadSet {
  // wrapped in structure so it can be easily copied and assigned
  BIT_ARRAY(read_set, DB_LOCKS);
};

struct Transaction {
  struct ReadSet read_set;
  struct LogItem *pos;
  unsigned acquired_locks;
  enum CommitType commit_type;
};

struct Database_;

typedef struct Handler_ {
  struct Database_ *database;

  uint64_t start_time;
  struct ReadSet read_set;

  FastStack(struct Transaction, 10) transactions[1];

  FastStack(struct LogItem, 63) log[1];

  enum CommitType commit_type;

  // keep list of acquired lock, so we dont have to iterate
  // through all locks when releasing them
  InlineStack(DB_LOCKS_NR_TYPE, DB_LOCKS) acquired_locks[1];

  struct {} __ancestor; // required for type magic
} Handler;

#endif


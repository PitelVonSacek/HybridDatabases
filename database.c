#include "database.h"
#include <stdlib.h>

#undef database_close
#undef database_dump
#undef database_wait_for_dump
#undef database_create_new_file

#undef db_handler_create
#undef db_handler_free
#undef db_handler_init
#undef db_handler_destroy

#undef tr_begin
#undef tr_abort
#undef tr_commit
#undef tr_hard_abort
#undef tr_is_main
#undef tr_validate

#undef tr_node_create
#undef tr_node_delete

#undef tr_node_read
#undef tr_node_write
#undef tr_node_update_indexies

#undef tr_node_check

#undef tr_node_get_type

static struct NodeAllocatorInfo log_allocator = { 
  .size = sizeof(((Handler*)0)->log->__block[0]),
  .counter = 0,
  .free_nodes = 0
};

static struct NodeAllocatorInfo transaction_allocator = { 
  .size = sizeof(((Handler*)0)->transactions->__block[0]),
  .counter = 0,
  .free_nodes = 0
};

__attribute__((destructor)) static void static_allocators_destroy() {
  node_allocator_destroy(&log_allocator);
  node_allocator_destroy(&transaction_allocator);
}

#define sendServiceMsg(D, ...) \
  do { \
    struct OutputList *__out = node_alloc(D->output.allocator); \
    *__out = (struct OutputList)__VA_ARGS__; \
    output_queue_push(D, __out, false); \
  } while (0)

// write.c
static void write_schema(Writer *W, Database *D);
static void write_file_header(Writer *W, Database *D, uint64_t magic);
static void write_file_footer(Writer *W, uint64_t magic);
static void write_dump_begin(Writer *W);
static void write_dump_end(Writer *W);

static void write_node_store(Writer *W, Node* node);
static void write_node_alloc(Writer *W, NodeType *type, uint64_t id);
static void write_node_delete(Writer *W, uint64_t id);
static void write_node_modify(Writer *W, uint64_t node_id, unsigned attr, 
                              unsigned attr_type, const void *value);

#ifndef SINGLE_SERVICE_THREAD
static void write_log(Writer *W, TransactionLog *log);
#endif

// read.c
static bool read_schema(Reader *R, Database *D);
static bool read_file_header(Reader *R, Database *D, uint64_t *magic);
static bool read_file_footer(Reader *R, uint64_t *magic);
static bool read_dump_begin(Reader *R);
static bool read_dump_end(Reader *R);

static bool read_node_prepare(Reader *R, Database *D, IdToNode *nodes, 
                              NodeType **type, Node **node_);
static bool read_node_load(Reader *R, Database *D, IdToNode *nodes);
static bool read_node_create(Reader *R, Database *D, IdToNode *nodes);

static bool read_node_delete(Reader *R, Database *D, IdToNode *nodes);
static bool read_node_modify(Reader *R, Database *D, IdToNode *nodes);


// transaction.c
static void _tr_unlock(Handler* H, uint64_t ver);
static void handler_cleanup(Handler *H);

static void log_undo_item(Handler *H, struct LogItem *item, uint64_t end_time);
void _tr_handler_rollback(Handler *H, struct Transaction *tr);

static void output_queue_push(Database *D, struct OutputList *item, bool hash_lock);

// void _tr_abort_main(Handler *H);
// bool _tr_commit_main(Handler *H, enum CommitType commit_type);

// database.c
static Database *database_alloc(const DatabaseType *type);
// database_close()
// ...

static uint64_t _generate_magic_nr();
static bool _database_new_file(Database *D, bool dump_begin, uint64_t magic_nr);

static void *service_thread(Database *D);

#include "attributes/attributes.h"

#include "database.include/database.c"
#include "database.include/database_create.c"
#include "database.include/handler.c"
#include "database.include/node.c"
#include "database.include/read.c"
#include "database.include/threads.c"
#include "database.include/transaction.c"
#include "database.include/write.c"


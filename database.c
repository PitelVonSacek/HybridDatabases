#include "database.h"
#include "database_create.include.c"

static struct NodeAllocatorInfo log_allocator = { 
  .size = sizeof(((Handler*)0)->log->__block[0]),
  .counter = 0,
  .free_nodes = 0
};


// write.c
static void write_schema(Writer *W, Database *D);
static void write_file_header(Writer *W, Database *D, uint64_t magic);
static void write_file_footer(Writer *W, Database *D, uint64_t magic);
static void write_dump_begin(Writer *W);
static void write_dump_end(Writer *W);

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
static bool read_file_footer(Writer *W, Database *D, uint64_t *magic);
static bool read_dump_begin(Reader *R);
static bool read_dump_end(Reader *R);

static bool read_node_prepare(Reader *R, Database *D, IdToNode *nodes, 
                              NodeType **type, Node **node_);
static bool read_node_load(Reader *R, Database *D, IdToNode *nodes);
static bool read_node_create(Reader *R, Database *D, IdToNode *nodes);

static bool read_node_delete(Reader *R, Database *D, IdToNodes *nodes);
static bool read_node_modify(Reader *R, Database *D, IdToNode *nodes);


// transaction.c
static void _tr_unlock(Handler* H, uint64_t ver);
static void handler_cleanup(Handler *H);

static void log_undo_item(Hanler *H, struct LogItem *item, uint64_t end_time);
static void handler_rollback(Handler *H, struct Transaction *tr);

static void output_queue_push(Database *D, struct OutputList *item);

// void _tr_abort_main(Handler *H);
// bool _tr_commit_main(Handler *H, enum CommitType commit_type);

// database.c
static void database_alloc(DatabaseType *type);
static void database_free(Database *database);

static bool _database_new_file(Database *D, bool dump_begin, uint64_t magic_nr);

#include "database.include/database.c"
#include "database.include/handler.c"
#include "database.include/node.c"
#include "database.include/read.c"
#include "database.include/threads.c"
#include "database.include/transaction.c"
#include "database.include/write.h"


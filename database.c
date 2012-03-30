#include "database.h"
#include <stdlib.h>
#include <time.h>
#include <dirent.h>
#include "allocators/simple_allocator.h"

// undef macros defined in type_magic.h so underlaying functions can be implemented
#undef database_close
#undef database_dump
#undef database_wait_for_dump
#undef database_create_new_file

#undef database_collect_garbage
#undef database_pause_service_thread
#undef database_resume_service_thread

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
#undef tr_node_update_indexes

#undef tr_node_check

#undef tr_node_get_type



#ifndef BD_OUTPUT_LIST_CACHE
#define BD_OUTPUT_LIST_CACHE 32
#endif

static void output_list_init(void *ptr) {
  struct OutputList *O = ptr;
  sem_init(&O->ready, 0, 0);
  writer_init(O->W);
}

static void output_list_destroy(void *ptr) {
  struct OutputList *O = ptr;
  sem_destroy(&O->ready);
  writer_destroy(O->W);
}

static struct SimpleAllocator output_list_allocator = 
  SimpleAllocatorInit(sizeof(struct OutputList), BD_OUTPUT_LIST_CACHE,
                      &output_list_init, &output_list_destroy);


__attribute__((destructor)) static void static_allocators_destroy() {
  simple_allocator_destroy(&output_list_allocator);
}

__attribute__((constructor)) static void init_rand() {
  srand(time(0));
}

#define sendServiceMsg(D, ...) \
  do { \
    struct OutputList *__out = simple_allocator_alloc(&output_list_allocator); \
    struct OutputList __tmp = __VA_ARGS__; \
    __out->type = __tmp.type; \
    __out->lock = __tmp.lock; \
    __out->content = __tmp.content; \
    output_queue_push(D, __out, false); \
  } while (0)


#define node_for_each(var, node_type) \
  for (Node *var = (node_allocator_dump_init(node_type->allocator), (void*)0); \
       var = node_type->allocator->dump_ptr;  \
       node_allocator_dump_next(node_type->allocator))



// database.c
// void database_close(Database *D);
// enum DbError database_dump (Database *D);
// void database_collect_garbage(Database* D);
// void database_pause_service_thread(Database* D);
// void database_resume_service_thread(Database* D);
// enum DbError database_create_new_file (Database *D);
static void init_locks(Database *D);
static void init_node_types(Database *D);
static void destroy_node_types(Database *D);
static Database *database_alloc(const DatabaseType *type);
static uint64_t _generate_magic_nr();
static bool _database_new_file(Database *D, bool dump_begin, uint64_t magic_nr);


// database_create.h
// Database *database_create (const DatabaseType *type, const char *file, unsigned flags);
static int get_db_files(struct dirent*** files, const char* dir, const char* db_name_);
static void fix_pointers(Database *D, IdToNode *nodes);
static void fill_indexes(Database *D);
static bool load_data(Database *D, IdToNode *nodes);
static int load_file(Database *D, Reader *R, uint64_t *magic_nr,
                     IdToNode *nodes, bool first_file);


// handler.c
// Handler *db_handler_create(Database *D);
// void db_handler_free (Handler *H);
// Handler *db_handler_init(Database *D, Handler *H);
// void db_handler_destroy(Handler *H);


// node.c
// Node *tr_node_create (Handler *H, NodeType *type);
// bool tr_node_delete (Handler *H, Node *node);
// bool tr_node_read (Handler *H, Node *node, int attr, void *buffer);
// bool tr_node_write (Handler *H, Node *node, int attr, const void *value);
// int tr_attr_count(NodeType *type);
// const char *tr_attr_get_name(NodeType *type, int index);
// int tr_attr_get_index(NodeType *type, const char *attr);
// const NodeType *tr_node_get_type (Node *node);
// const int tr_attr_get_type(NodeType *type, int index);
static __attribute__((unused)) void node_offset_check();


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


// threads.c
static void process_transaction_log(TransactionLog *log, Database *D,
                                    Writer *W, size_t end_time);

static bool do_dump(Database *D, Writer *W, NodeType **dump_type);

static uint64_t get_time(Database *D);
static void collect_garbage(Database *D);
static void *service_thread(Database *D);


// transaction.c
// void _tr_retry_wait(int loop);
// void _tr_handler_rollback(Handler *H, struct Transaction *tr);
// void _tr_abort_main(Handler *H);
// bool _tr_commit_main(Handler *H, enum CommitType commit_type);
static void _tr_unlock(Handler* H, uint64_t ver);
static void handler_cleanup(Handler *H);
static void log_undo_item(Handler *H, struct LogItem *item, uint64_t end_time);
static void output_queue_push(Database *D, struct OutputList *O, bool has_lock);


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



#include "attributes/attributes.h"

#include "database.include/database.c"
#include "database.include/database_create.c"
#include "database.include/handler.c"
#include "database.include/node.c"
#include "database.include/read.c"
#include "database.include/threads.c"
#include "database.include/transaction.c"
#include "database.include/write.c"


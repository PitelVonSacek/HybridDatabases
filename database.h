#ifndef __DATABASE_INTERFACE_H__
#define __DATABASE_INTERFACE_H__

#include "database.types/attributes.h"
#include "database.types/database.h"
#include "database.types/enums.h"
#include "database.types/handler.h"
#include "database.types/index.h"
#include "database.types/node.h"
#include "utils/utils.h"          

/*
 * All following functions are wrapped (in database.include/type_magic.h)
 * into macros of the same name, so the can be used not only
 * on abstract types (like Database, Handler, ...), but on their
 * subclasses to (like MyDatabase, ...)
 */

/*************************
 *  Database functions   *
 *************************/

/*
 * flags:
 *   DB_CREATE - create database if it doesn't exist
 *   DB_READ_ONLY - do not modify database (changes are writen to /dev/null)
 *
 * on failure returns 0
 *
 * when database is corrupted 'only a little', returns database instance,
 * but with DB_READ_ONLY flag set
 */
Database *database_create(const DatabaseType *type, const char *file, unsigned flags);

void database_close(Database*);

static inline unsigned database_get_flags(Database*);

enum DbError database_dump(Database*);
void database_wait_for_dump(Database*);

enum DbError database_create_new_file(Database*);

void database_collect_garbage(Database*);

void database_pause_service_thread(Database*);
void database_resume_service_thread(Database*);

/*
  Macros:
 
#define dbCreate(Type, file, flags)

*/


/*************************
 *   Handler functions   *
 *************************/

Handler *db_handler_create(Database*);
void db_handler_free(Handler*);

Handler *db_handler_init(Database*, Handler*);
void db_handler_destroy(Handler*);

/*
  Macros:

#define dbHandlerCreate(database)

*/


/*************************
 * Transaction functions *
 *************************/

static inline void tr_begin(Handler *H);
static inline void tr_abort(Handler *H);
static inline bool tr_commit(Handler *H, enum CommitType commit_type);
static inline void tr_hard_abort(Handler *H);

// alomost internal function
static inline bool tr_is_main(Handler *H);

// checks that none of read set was modified by other transaction
static inline bool tr_validate(Handler *H);

/* Macros:

  macros for easier work with transactions, they restart transaction
  when it fails (cause of collision, not cause of trAbort)

  -- trBegin and trCommit() work as parethensis
#define trBegin
#define trCommit(commit_type, restart_outer) -- restart_outer is executed
  when transaction needs to be restarted, but isn't main transaction,
  so it needs to propagate failure, optional parameter, defaults to assert(0)

#define trAbort -- aborts transaction, execution continues after trCommit

*/


/*************************
 *     Node functions    *
 *************************/

Node *tr_node_create(Handler *H, NodeType *type);

// fails if node is referenced by other nodes
bool tr_node_delete(Handler *H, Node *node);

// Those functions are slow, use macros instead whenever possible
// returns true on success, false when transactions collide
bool tr_node_read(Handler *H, Node *node, int attr, void *buffer);
bool tr_node_write(Handler *H, Node *node, int attr, const void *value);
static inline bool tr_node_update_indexies(Handler *H, Node *node);

static inline bool tr_node_check(Handler *H, Node *node);


int tr_attr_count(NodeType*);
const char *tr_attr_get_name(NodeType*, int);
int tr_attr_get_index(NodeType*, const char*);

const NodeType *tr_node_get_type(Node *node);
const int tr_attr_get_type(NodeType *type, int index);

/* Macros:

#define trNodeCast(NodeType, node) -- cast node to NodeType, return 0 on fail
  
#define trFail goto tr_failed -- action taken when some operation on TM fails

  (for all following macros there is version suffixed with _, 
   that takes Handler* as first argument)
  (when operation fails, macro executes trFail)

#define trRead(node, AttrName) -- validates read value before returning
#defien trUncheckedRead(node, AttrName) -- skips validation
#define trWrite(node, AttrName, value)


#define trUpdateIndexies(node) -- must be called on each modified node before commiting

#define trCheck(node) -- validates lock for node

#define trNodeCreate(NodeType)
#define trNodeDelete(node)

#define trIndex(index_name, method, arguments...)

  for using in indexes:
#define trMemoryRead(object, atttribute) -- reads value (object attribute)
#define trMemoryWrite(object, attribute, value)

*/

#include "database.include/macros.h"
#include "database.include/inline.h"
// must be AFTER inline.h
#include "database.include/type_magic.h"
#include "attributes/attributes.inline.h"

#endif


#ifndef __DATABASE_INTERFACE_H__
#define __DATABASE_INTERFACE_H__

#include "database_types.h"


/*************************
 *  Database functions   *
 *************************/

Database *database_create(const DatabaseType *type, 
                          const char *dir, 
                          const char *file,
                          unsigned flags);

int database_close(Database*);

int database_dump(Database*);
void database_wait_for_dump(Database*);

int database_create_new_file(Database*);


/*************************
 *   Handler functions   *
 *************************/

Handler *db_handler_create(Database*);
void db_handler_free(Handler*);

Handler *db_handler_init(Database*, Handler*);
void db_handler_destroy(Handler*);


/*************************
 * Transaction functions *
 *************************/

static inline void tr_begin(Handler *H);
static inline void tr_abort(Handler *H);
static inline bool tr_commit(Handler *H, enum CommitType commit_type); 

static inline bool tr_validate(Handler *H);

/* Macros:

  -- trBegin and trCommit() work as parethensis
#define trBegin
#define trCommit(commit_type)

#define trAbort

*/


/*************************
 *     Node functions    *
 *************************/

Node *tr_node_create(Handler *H, NodeType *type);

// fails if node is referenced by other nodes
bool tr_node_delete(Handler *H, Node *node);

// Those functions are slow, use macros instead whenever possible
bool tr_node_read(Handler *H, Node *node, int attr, void *buffer);
bool tr_node_write(Handler *H, Node *node, int attr, const void *value);

int tr_attr_count(NodeType*);
const char *tr_attr_get_name(NodeType*, int;
int tr_attr_get_index(NodeType*, const char*);

const NodeType *tr_node_get_type(Node *node);
const AttributeType *tr_attr_get_type(NodeType *type, int index);

/* Macros:

#define trNodeCast(NodeType, node)
  
  (for all following macros there is version suffixed with _, 
   that takes Handler* as first argument)
  (when operation fails, macro executes trFail)


#define trFail goto tr_failed

#define trRead(node, AttrName)
#define trWrite(node, AttrName, value)
#define trCheck(node) -- validates lock for node

#define trNodeCreate(NodeType)
#define trNodeDelete(node)

*/

#include "database.include/inline.h"

#endif

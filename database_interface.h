#ifndef DATABASE_INTERFACE_H
#define DATABASE_INTERFACE_H

#include "database_types.h"

/*****************************
 * Database global functions *
 *****************************/

Database *database_create(DatabaseType* type, const char* dir, const char* file, unsigned int flags);
#define DatabaseCreate(Type, dir, file, flags) \
  ((Type*)database_create((DatabaseType*)DatabaseType__##Type##__desc, dir, file, flags))


// Errors: TRERR_TRANSACTION_RUNNING
int database_close_(Database* D);
#define database_close(D) database_close_(TrDBUncast(D))

// dumps database
// Errors: TRERR_DUMP_RUNNING
int database_dump_(Database* D, int sync);
#define database_dump(D, sync) database_dump_(TrDBUncast(D), sync)

// blocks until current dump ends
void database_wait_for_dump_(Database *D);
#define database_wait_for_dump(D) database_wait_for_dump_(TrDBUncast(D))

// creates new data file and strarts to write in it
int database_new_file_(Database* D);
#define database_new_file(D) database_new_file_(TrDBUncast(D))

/*************************
 * Transaction functions *
 *************************/

Transaction* tr_main_start_(Database*);
#define tr_main_start(D) tr_main_start_(TrDBUncast(D))

void tr_main_abort_(Transaction*);
#define tr_main_abort(tr) tr_main_abort_(TrUncast(tr))

// Errors: TRERR_COLLISION
int tr_main_commit_(Transaction*, int sync);
#define tr_main_commit(tr, s) tr_main_commit_(TrUncast(tr), s)

void tr_start_(Transaction*);
void tr_abort_(Transaction*);
void tr_commit_(Transaction*);

#define tr_start(tr) tr_start_(TrUncast(tr))
#define tr_abort(tr) tr_abort_(TrUncast(tr))
#define tr_commit(tr) tr_commit_(TrUncast(tr))


/******************
 * Node functions *
 ******************/

Node *node_create_(Transaction*, NodeType*);
#define NodeCreate_(tr, Type) ((Node(Type)*)node_create_(TrUncast((tr)), (NodeType*)(tr)->__database->__type->Type))
#define NodeCreate(Type) NodeCreate_(_transaction, Type)


// removes node from db, fails if there are references to it
// Errors:  TRERR_NONZERO_REF_COUNT
int node_remove_(Transaction*, Node*);
#define node_remove(n) node_remove_(TrUncast(_transaction), TrNodeUncast(n))

#endif // DATABASE_INTERFACE_H

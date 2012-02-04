#ifndef DATABASE_MACROS_H
#define DATABASE_MACROS_H

#include <time.h>

#define TrDebug(f, ...) TrDebug_("HybridDatabase debug: " f " at %s line %i\n", ## __VA_ARGS__, __FILE__, __LINE__)

#ifndef TrDebug_
#define TrDebug_(...) fprintf(stderr, __VA_ARGS__)
#endif

#define TrMainStart(db) do { \
  __label__ TR_RESTART_LABEL; \
  typeof(&*(db)->__dummy_transaction) _transaction; \
  int _tr_run = 0; \
  TR_RESTART_LABEL: \
  TRY { \
    _transaction = (void*)tr_main_start(db);


#define TrMainAbort THROW(TR_MAIN_ABORT, 0)

#define TrMainCommit(sync) \
    if (tr_main_commit(_transaction, sync)) { \
      _transaction = 0; \
      THROW(TR_FAIL, 0); \
    } \
  } CATCH(TR_MAIN_ABORT) { \
    tr_main_abort(_transaction); \
  } CATCH(TR_FAIL) { \
    if (_transaction) tr_main_abort(_transaction); \
    _tr_run++; \
    struct timespec delay = { .tv_sec = 0, .tv_nsec = (((unsigned)(rand() * rand())) % (1000 << (_tr_run > 19 ? 19 : _tr_run))) }; \
    TrDebug_("sleeping for %i\n", delay.tv_nsec);  \
    nanosleep(&delay, 0); \
    /* TODO wait random time exponential to _tr_run */ \
    goto TR_RESTART_LABEL; \
  } END \
} while (0);



#define TrStart TRY { tr_start(_transaction);

#define TrAbort THROW(TR_ABORT, 0)

#define TrCommit tr_commit(_transaction); \
  } CATCH(TR_ABORT) { tr_abort(_transaction); } END



#define TrIndex_(tr, index, fnc, ...) \
  (fnc(&tr->__database->index, TrUncast(tr), ## __VA_ARGS__))

#define TrIndex(index, fnc, ...) \
  TrIndex_(_transaction, index, fnc, ## __VA_ARGS__)


#define TrDBUncast(db) \
  __builtin_choose_expr(__builtin_types_compatible_p(typeof(db), Database*), (db), \
  __builtin_choose_expr(__builtin_types_compatible_p(typeof((db)->__database), Database), \
  (Database*)(db), 0.0))

#define TrNodeUncast(node) \
  __builtin_choose_expr(__builtin_types_compatible_p(typeof(node), Node*), (node), \
  __builtin_choose_expr(__builtin_types_compatible_p(typeof((node)->__node), Node), \
  (Node*)(node), 0.0))

#define TrUncast(tr) \
  __builtin_choose_expr(__builtin_types_compatible_p(typeof(tr), Transaction*), (tr), \
  __builtin_choose_expr(__builtin_types_compatible_p(typeof((tr)->__transaction), Transaction), \
  (Transaction*)(tr), 0.0))

// #include "database_interface.h"
// #include "database_inline.h"
// #include "database_node_macros.h"

#endif // DATABASE_MACROS_H

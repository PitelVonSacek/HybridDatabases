#include "database.h"
#include "database_create.include.c"

EXCEPTION_DEFINE(TR_FAIL, NONE);
EXCEPTION_DEFINE(TR_MAIN_ABORT, TR_FAIL);
EXCEPTION_DEFINE(TR_ABORT, TR_MAIN_ABORT);

int database_close_(Database* D) {
  int ret = TRERR_SUCCESS;

  if (!D) return ret;

  TRY {
    // check that no transaction is running
    ALWAYS(pthread_mutex_lock(&D->dump_mutex);
           pthread_mutex_lock(&D->mutex)) {

      for (struct TransactionList* list = D->tr_old; list; list = list->newer)
        if (list->running) THROW_;

      D->flags |= DB_SHUT_DOWN;
      D->dump_flags |= DB_SHUT_DOWN;

    } ALWAYS_END(pthread_mutex_unlock(&D->mutex);
                 pthread_mutex_unlock(&D->dump_mutex));

    pthread_cond_broadcast(&D->signal);
    pthread_cond_broadcast(&D->dump_signal);

    TrDebug_("Waiting for dump thread\n");
    pthread_join(D->dump_thread, 0);
    TrDebug_("Waiting for IO thread\n");
    pthread_join(D->io_thread, 0);
    TrDebug_("Waiting for GC thread\n");
    pthread_join(D->gc_thread, 0);
    TrDebug_("Waiting done\n");

    // tr_free(D->tr_new); NO gc thread took care of it

    // destroy nodes
    Node* node_next;
    for (Node* node = D->node_list; node; node = node_next) {
      node_next = node->_next;
      node->type->destroy(node);
      tr_free(node);
    }

    D->type->indexes_destroy(D);

    if (D->output) fclose(D->output);

    tr_free(D->filename);
    tr_free(D);
  } CATCH_ALL {
    ret = TRERR_TRANSACTION_RUNNING;
  } END;

  return ret;
}

// dumps database
int database_dump_(Database* D, int sync) {
  pthread_mutex_lock(&D->dump_mutex);

  if (D->dump_flags & DB_DUMP_RUNNING) {
    pthread_mutex_unlock(&D->dump_mutex);
    return TRERR_DUMP_RUNNING;
  }

  D->dump_flags |= DB_DO_DUMP;

  pthread_mutex_unlock(&D->dump_mutex);

  pthread_cond_broadcast(&D->dump_signal);

  if (sync) database_wait_for_dump(D);

  return TRERR_SUCCESS;
}

// blocks until current dump ends
void database_wait_for_dump_(Database *D) {
  TrDebug_("Waiting for dump to finish\n");
  int running = 1;

  pthread_mutex_lock(&D->dump_mutex);

  while (running) {
    if (!(D->dump_flags & DB_DUMP_RUNNING)) running = 0;
    else pthread_cond_wait(&D->dump_signal, &D->dump_mutex);
  }

  pthread_mutex_unlock(&D->dump_mutex);
}

static void data_file_write_headers(Database* D, uint64_t magic_nr) {
  Writer W[1];
  ALWAYS(writer_init(W)) {

    W_ARRAY {
      W_STR("HybridDatabase Data File");
      W_STR(D->type->name);
      W_STR(D->type->version);
    } W_ARRAY_END;
    W_CHECKSUM;

    fwrite(w_ptr(W), 1, w_length(W), D->output);

    w_discart(W);

    W_NUMBER(magic_nr);

    fwrite(w_ptr(W), 1, w_length(W), D->output);

  } ALWAYS_END(writer_destroy(W));
}

int database_new_file_(Database *D) { return database_new_file__(D, 0, 0); }

// creates new data file and strarts to write in it
static int database_new_file__(Database* D, int dump_begin, uint64_t magic_nr) {
  TrDebug_("Creating new file\n");
  int ret = TRERR_SUCCESS;
  char *buffer = tr_alloc(strlen(D->filename) + 30);

  TRY {
    // dump cannot start while we're holding lock :-)
    // io thread will block on dump_mutex too after writing our special message
    pthread_mutex_lock(&D->dump_mutex);

    if ((D->dump_flags & DB_DUMP_RUNNING) && !dump_begin) THROW_;

    if (D->output) {
      if (!magic_nr) {
        struct OutputQueue* Q = tr_alloc(sizeof(*Q));
        sem_t signal[1];
        sem_init(signal, 0, 0);
        Writer W[1];
        writer_init(W);

        W_STR("END OF FILE");

        pthread_mutex_lock(&D->mutex);
        oq_add(Q, D, W, 3, signal);
        pthread_mutex_unlock(&D->mutex);
        pthread_cond_broadcast(&D->signal);

        sem_wait(signal);

        writer_init(W);
        W_NUMBER(magic_nr); // 'magic' number
        fwrite(w_ptr(W), 1, w_length(W), D->output);

        writer_destroy(W);
      }

      fclose(D->output);
      D->output = 0;
    }

    D->current_file_index++;

    if (!magic_nr) magic_nr = ((uint64_t)(rand() * rand() * rand() * rand()));

    sprintf(buffer, "%s.%i", D->filename, D->current_file_index);

    D->output = ENSURE_(fopen(buffer, "wb"));

    data_file_write_headers(D, magic_nr);

    if (dump_begin) {
      Writer W[1];
      writer_init(W);
      w_discart(W);
      W_STR("DUMP BEGIN");
      fwrite(w_ptr(W), 1, w_length(W), D->output);
      writer_destroy(W);
    }

    fclose(D->output);
    ENSURE_(D->output = fopen(buffer, "ab"));

  } CATCH_ALL {
    ret = TRERR_DUMP_RUNNING;
    TrDebug_("Exception: %s Info: %s\n", exception->name, (char*)exception_info);
  } FINALLY {
   // pthread_mutex_unlock(&D->mutex);
    pthread_mutex_unlock(&D->dump_mutex);
    tr_free(buffer);
  } END;

  return ret;
}


/*************************
 * Transaction functions *
 *************************/

static void tr_delete(Transaction *tr) {
  __sync_fetch_and_sub(&tr->tr_list->running, 1);
  pthread_cond_broadcast(&tr->database->signal);

  stack_destroy(tr->memory_deleted);
  stack_destroy(tr->memory_new);
  stack_destroy(tr->nodes_deleted);

  stack_destroy(tr->write_log);
  stack_destroy(tr->tr_stack);

  // DON'T delete tr->W here

  tr_free(tr);
}

static void rollback_up_to(Transaction* tr, TrNestedTransaction* stop) {
  tr->read_set = stop->read_set;
  w_set_cur_pos(tr->W, stop->writer_pos);

  while (stack_size(tr->write_log) > stop->write_log) {
    TrLogItem *item = &stack_pop(tr->write_log);
    memcpy(item->ptr, item->data, item->size);
  }

  tr->nodes_deleted->ptr = tr->nodes_deleted->begin + stop->nodes_deleted;
  tr->memory_deleted->ptr = tr->memory_deleted->begin + stop->memory_deleted;

  while (stack_size(tr->memory_new) > stop->memory_new) {
    tr_free(stack_pop(tr->memory_new));
  }

  while (tr->nodes_new != stop->nodes_new) {
    Node *n = tr->nodes_new;
    tr->nodes_new = n->_next;

    n->type->destroy(n);
    tr_free(n);
  }

  if (!tr->nodes_new) tr->nodes_new_end = 0;
}

void tr_main_abort_(Transaction* tr) {
  TrNestedTransaction all = { {}, 0, 0, 0, 0, 0, 0 };

  if (!stack_empty(tr->write_log) || stack_empty(tr->nodes_deleted) || tr->nodes_new) {
    rollback_up_to(tr, &all);

    Database *db = tr->database;

    struct TransactionList* tlist = tr_alloc(sizeof(*tlist));

    tlist->newer = 0;
    tlist->node_list = 0;
    tlist->void_list = 0;
    tlist->running = 0;


    pthread_mutex_lock(&db->mutex);
    uint64_t tr_end = __sync_add_and_fetch(&tr->database->time, 2);

    tlist->time = tr_end;
    db->tr_new->newer = tlist;
    db->tr_new = tlist;
    pthread_mutex_unlock(&db->mutex);

    for (int i = 0; i < DB_LOCKS; i++) TrLockUnlock_(tr, i, tr_end);
  }

  writer_destroy(tr->W);

  tr_delete(tr);
}

int tr_main_commit_(Transaction* tr, int sync) {
  sem_t finished[1];
  sem_init(finished, 0, 0);

  if (!stack_empty(tr->write_log) || stack_empty(tr->nodes_deleted) || tr->nodes_new) {
    Database *db = tr->database;
    uint64_t tr_end;

    w_array_end(tr->W);
    w_checksum(tr->W);

    struct NodeStackList *nlist = tr_alloc(sizeof(*nlist));
    struct VoidStackList *vlist = tr_alloc(sizeof(*vlist));
    struct OutputQueue* Q = tr_alloc(sizeof(*Q));
    struct TransactionList* tlist = tr_alloc(sizeof(*tlist));

    tlist->newer = 0;
    tlist->node_list = 0;
    tlist->void_list = 0;
    tlist->running = 0;

    Q->next = 0;
    Q->data = w_ptr(tr->W);
    Q->size = w_length(tr->W);
    Q->flags = !!sync;
    Q->lock = (sync ? finished : (void*)0);

    stack_init(vlist->stack);
    stack_init(nlist->stack);

    pthread_mutex_lock(&db->mutex); {

      int read_set = 1;
      for (int i = 0; i < DB_LOCKS; i++)
        if (bit_array_isset(tr->read_set.read_set, i) && !TrLockCheck_(tr, i))
          read_set = 0;

      if (!read_set) {
        pthread_mutex_unlock(&db->mutex);
        tr_main_abort(tr);
        return TRERR_COLLISION;
      }

      tr_end = __sync_fetch_and_add(&tr->database->time, 2);

      if (tr->nodes_new) {
        tr->nodes_new_end->_next = db->node_list;
        if (db->node_list) db->node_list->_prev = tr->nodes_new_end;
        else db->node_list_end = tr->nodes_new_end;
        db->node_list = tr->nodes_new;
      }

      nlist->next = db->tr_new->node_list;
      stack_swap(nlist->stack, tr->nodes_deleted);
      db->tr_new->node_list = nlist;

      vlist->next = db->tr_new->void_list;
      stack_swap(vlist->stack, tr->memory_deleted);
      db->tr_new->void_list = vlist;

      if (db->oq_tail) db->oq_tail->next = Q;
      else db->oq_head = Q;
      db->oq_tail = Q;

      tlist->time = tr_end;
      db->tr_new->newer = tlist;
      db->tr_new = tlist;

    } pthread_mutex_unlock(&tr->database->mutex);

    pthread_cond_broadcast(&tr->database->signal);

    for (int i = 0; i < DB_LOCKS; i++) TrLockUnlock_(tr, i, tr_end);

    if (sync) sem_wait(finished);
  } else writer_free(tr->W);

  tr_delete(tr);

  return TRERR_SUCCESS;
}

void tr_start_(Transaction* tr) {
  TrNestedTransaction nt = {
    tr->read_set,
    stack_size(tr->write_log),
    stack_size(tr->nodes_deleted),
    stack_size(tr->memory_new),
    stack_size(tr->memory_deleted),
    tr->nodes_new,
    w_get_cur_pos(tr->W)
  };

  stack_push(tr->tr_stack, nt);
}

void tr_abort_(Transaction* tr) {
  rollback_up_to(tr, &stack_pop(tr->tr_stack));
}

void tr_commit_(Transaction* tr) { stack_pop(tr->tr_stack); }


/******************
 * Node functions *
 ******************/

Node* node_create_(Transaction * tr, NodeType* type) {
  Node *node = tr_alloc(type->size);
  type->init(node);
  *(NodeType**)&node->type = type;

  // atomicaly read db node_id_counter a increment it
  *(uint64_t*)&node->id = __sync_fetch_and_add(&tr->database->node_id_counter, 1);

  node->_prev = 0;
  node->_next = tr->nodes_new;
  tr->nodes_new = node;
  if (!tr->nodes_new_end) tr->nodes_new_end = node;

  tr->database->type->indexes_update(tr, CBE_NODE_CREATED, node);

  Writer *W = tr->W;

  W_ARRAY {
    W_NUMBER(CBE_NODE_CREATED);
    W_NUMBER(type->id);
    W_NUMBER(node->id);
  } W_ARRAY_END;

  return node;
}

// removes node from db, fails if there are references to it
int node_remove_(Transaction* tr, Node* node) {
  if (!TrLockLock_(tr, TrHash(node))) return TRERR_COLLISION;
  if (node->ref_count) return TRERR_NONZERO_REF_COUNT;

  if (node->type->destroy_pointers(tr, node) != TRERR_SUCCESS) return TRERR_COLLISION;
  stack_push(tr->nodes_deleted, node);

  tr->database->type->indexes_update(tr, CBE_NODE_DELETED, node);

  return TRERR_SUCCESS;
}

Transaction* tr_main_start_(Database* db) {
  Transaction* tr = tr_alloc(sizeof(*tr));

  *tr = (Transaction){
    .database = db,
    .tr_stack = { StackInit },
    .write_log = { StackInit },
    .nodes_new = 0,
    .nodes_new_end = 0,
    .nodes_deleted = { StackInit },
    .memory_new = { StackInit },
    .memory_deleted = { StackInit } //,
    //.W = writer_create()
  };

  writer_init(tr->W);
  w_array(tr->W);

  memset(&tr->read_set, 0, sizeof(tr->read_set));

  pthread_mutex_lock(&db->mutex);

  tr->tr_list = db->tr_new;
  __sync_fetch_and_add(&db->tr_new->running, 1);

  pthread_mutex_unlock(&db->mutex);

  tr->start_time = tr->tr_list->time;

  return tr;
}

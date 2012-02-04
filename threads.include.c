#include "database_macros.h"
#include <stdio.h>
#include "dictionary.h"
#include "exception.h"

static inline void oq_add(struct OutputQueue* Q, Database *db, Writer *W, int flush, sem_t* lock) {
  Q->next = 0;
  Q->data = w_ptr(W);
  Q->size = w_length(W);
  Q->flags = flush;
  Q->lock = lock;

  if (db->oq_tail) db->oq_tail->next = Q;
  else db->oq_head = Q;
  db->oq_tail = Q;
}

static void* dump_thread(Database* D) {
  TrDebug("Dump thread on");
  static const uint64_t LOCK_MAX = ~(uint64_t)0;
  sem_t signal[1];
  sem_init(signal, 0, 0);

  while (1) {
    TrDebug_("Dump thread loop\n");
    pthread_mutex_lock(&D->dump_mutex);

    get_job:
    if (D->dump_flags & DB_DO_DUMP) {
      D->dump_flags ^= DB_DO_DUMP | DB_DUMP_RUNNING;
    } else {
      if (D->dump_flags & DB_SHUT_DOWN) {
        pthread_mutex_unlock(&D->dump_mutex);
        return 0;
      }

      TrDebug_("Dump thread is waiting\n");
      pthread_cond_wait(&D->dump_signal, &D->dump_mutex);
      TrDebug_("Dump thread woke up\n");
      goto get_job;
    }

    pthread_mutex_unlock(&D->dump_mutex);

    // make sure that no one will mess with node_list until we finish
    // lock needed cause tr_old pointer

    TrDebug_("> Dump begins...\n");

    Node *node;

    pthread_mutex_lock(&D->mutex);
    __sync_fetch_and_add(&D->tr_old->running, 1);
    node = D->node_list;
    pthread_mutex_unlock(&D->mutex);

      // TODO create new file, write headers
    ENSURE_(database_new_file__(D, 1, 0) == TRERR_SUCCESS);

    while (node) {
      Writer W[1];
      uint64_t old_lock;
      unsigned lock = TrHash(node) % DB_LOCKS;
      struct OutputQueue *Q = tr_alloc(sizeof(*Q));

      writer_init(W);

      while (!(old_lock = l_lock_(&D->locks[lock], 0, LOCK_MAX))) {
        // we will wait until something happens
        // alternative way is to note that we skipped this node, go on and
        // return to this one later
        pthread_mutex_lock(&D->mutex);
        pthread_cond_wait(&D->signal, &D->mutex);
        pthread_mutex_unlock(&D->mutex);
      }

      // refcount == ~0 -> node was deleted
      if (node->ref_count != LOCK_MAX) {
        W_ARRAY; {
          W_STRING(0);
          node->type->store(W, node);
        } W_ARRAY_END;
        W_CHECKSUM;

        pthread_mutex_lock(&D->mutex);
        oq_add(Q, D, W, 0, signal);
        pthread_mutex_unlock(&D->mutex);
      }

      // we can restore old lock cause we didn't modify anything
      // lock was required only to ensure that node is not modified while
      // dumping it
      l_unlock(&D->locks[lock], 0, old_lock);

      node = node->_next;
    }

    __sync_fetch_and_sub(&D->tr_old->running, 1);
    pthread_cond_broadcast(&D->signal);

    {
      Writer W[1];
      writer_init(W);
      struct OutputQueue *Q = tr_alloc(sizeof(*Q));

      memcpy(W_STRING(8), "DUMP END", 8);
      W_CHECKSUM;

      pthread_mutex_lock(&D->mutex);
      oq_add(Q, D, W, 1, signal);
      pthread_mutex_unlock(&D->mutex);
    }

    pthread_mutex_lock(&D->dump_mutex);
    D->dump_flags ^= DB_DUMP_RUNNING;
    pthread_mutex_unlock(&D->dump_mutex);
    pthread_cond_broadcast(&D->dump_signal);

    TrDebug_("> Dump finished\n");
  }
}

static void* io_thread(Database* D) {
  struct OutputQueue *job;

  TrDebug("IO thread on");

  while (1) {
    job = 0;
    FILE* f;

    pthread_mutex_lock(&D->mutex);

    get_job:
    if (D->oq_head) {
      job = D->oq_head;
      D->oq_head = job->next;
      if (!job->next) D->oq_tail = 0;

      f = D->output;
    } else {
      if (D->flags & DB_SHUT_DOWN) {
        pthread_mutex_unlock(&D->mutex);
        return 0;
      }

      pthread_cond_wait(&D->signal, &D->mutex);
      goto get_job;
    }

    pthread_mutex_unlock(&D->mutex);

    fwrite(job->data, 1, job->size, f);

    if (job->flags & 1) fflush(f);

    if (job->lock) sem_post(job->lock);

    if (job->flags & 2) {
      // when creating new file, this delays io thread until
      // new file is prepared
      pthread_mutex_lock(&D->dump_mutex);
      pthread_mutex_unlock(&D->dump_mutex);
    }

    tr_free(job->data);
    tr_free(job);
  }
}

static void* gc_thread(Database *D) {
  TrDebug("GC thread on");

  struct TransactionList *job = 0;
  struct TransactionList _job;

  while (1) {
    pthread_mutex_lock(&D->mutex);

    get_job:
    if (D->tr_old && D->tr_old->running == 0 &&
        (D->tr_old != D->tr_new || D->tr_old->node_list || D->tr_old->void_list)) {
      job = D->tr_old;
      if (job->newer) D->tr_old = job->newer;
      else {
        _job = *job;
        job->node_list = 0;
        job->void_list = 0;
        job = &_job;
      }
    } else {
      if (D->flags & DB_SHUT_DOWN) {
        pthread_mutex_unlock(&D->mutex);
        tr_free(D->tr_new);
        return 0;
      }

      pthread_cond_wait(&D->signal, &D->mutex);
      goto get_job;
    }

    pthread_mutex_unlock(&D->mutex);

    {  // release memory blocks
       struct VoidStackList *list, *list_next;
       for (list = job->void_list; list; list = list_next) {
         list_next = list->next;

         while (!stack_empty(list->stack)) tr_free(stack_pop(list->stack));

         stack_destroy(list->stack);
         tr_free((void*)list);
       }
    }

    {  // release nodes
       struct NodeStackList *list, *list_next;
       for (list = job->node_list; list; list = list_next) {
         list_next = list->next;

         while (!stack_empty(list->stack)) {
           Node* n = stack_pop(list->stack);

           pthread_mutex_lock(&D->mutex);
           if (n->_prev) n->_prev->_next = n->_next;
           else D->node_list = n->_next;
           if (n->_next) n->_next->_prev = n->_prev;
           else D->node_list_end = n->_prev;
           pthread_mutex_unlock(&D->mutex);

           n->type->destroy(n);
           tr_free(n);
         }

         stack_destroy(list->stack);
         tr_free((void*)list);
       }
    }

    if (job != &_job) tr_free(job);
  }
}


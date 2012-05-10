// unlocks all locks held by H
static void _tr_unlock(Handler* H, uint64_t ver) {
#if INPLACE_NODE_LOCKS || INPLACE_INDEX_LOCKS
  while (!stack_empty(H->acquired_locks))
    l_unlock(stack_pop(H->acquired_locks), H, ver);
#else
  Lock * const locks = H->database->locks;

  while (!istack_empty(H->acquired_locks))
    l_unlock(locks + istack_pop(H->acquired_locks), H, ver);
#endif
}

void _tr_retry_wait(int loop) {
  uint64_t t = 10000;

  while (--loop > 0) t *= 2;

  if (t > 100000000) t = 100000000;

  t += rand() % 500000;

  struct timespec timeout = { 
    .tv_sec = 0, 
    .tv_nsec = t
  };

  nanosleep(&timeout, 0);
}

static void handler_cleanup(Handler *H) {
  atomic_write(&H->start_time, 0);

#if INPLACE_NODE_LOCKS || INPLACE_INDEX_LOCKS
  stack_erase(&H->read_set);
  stack_erase(H->acquired_locks);
#else
  bit_array_erase(&H->read_set);
  istack_erase(H->acquired_locks);
#endif

  fstack_erase(H->transactions);
 
  fstack_erase(H->log);

  H->commit_type = CT_ASYNC;
}

static void log_undo_item(Handler *H, struct LogItem *item, uint64_t end_time) {
  switch (item->type) {
    case LI_TYPE_RAW:
    case LI_TYPE_NODE_MODIFY:
      util_write(util_apply_offset(item->ptr, item->offset), item->data_old,
                 item->size, 0);
      break;
    case LI_TYPE_ATOMIC_RAW:
    case LI_TYPE_ATOMIC_NODE_MODIFY:
      util_write(util_apply_offset(item->ptr, item->offset), item->data_old,
                 item->size, 1);
      break;

    case LI_TYPE_NODE_ALLOC: {
      Node *n = (Node*)item->ptr;
      node_get_type(n)->destroy(H->database->tm_allocator, n, end_time);
      node_allocator_free(node_get_type(n)->allocator, n, end_time);
      break;
    }
    case LI_TYPE_MEMORY_ALLOC:
      generic_allocator_free(H->database->tm_allocator, item->ptr, end_time);
      break;
    case LI_TYPE_NODE_DELETE:
    case LI_TYPE_MEMORY_DELETE:
      break;
    default:
      assert(0);
  }
}

void _tr_handler_rollback(Handler *H, struct Transaction *tr) {
#if INPLACE_NODE_LOCKS || INPLACE_INDEX_LOCKS
  H->read_set.ptr = H->read_set.begin + tr->read_set;
#else
  H->read_set = tr->read_set;
#endif

  const uint64_t end_time = atomic_read(&H->database->time);

  if (tr->pos) while (&fstack_top(H->log) != tr->pos) {
    log_undo_item(H, &fstack_top(H->log), end_time);
    fstack_pop(H->log);
  } else while (!fstack_empty(H->log)) {
    log_undo_item(H, &fstack_top(H->log), end_time);
    fstack_pop(H->log);
  }

  H->commit_type = tr->commit_type;
}

void _tr_abort_main(Handler *H) {
  struct Transaction empty_transaction;
  memset(&empty_transaction, 0, sizeof(empty_transaction));

  _tr_handler_rollback(H, &empty_transaction);

  uint64_t end_time = atomic_add_and_fetch(&H->database->time, 2);
  _tr_unlock(H, end_time);
  
  handler_cleanup(H);
}

static void output_queue_push(Database *D, struct OutputList *O, bool has_lock) {
  O->next = 0;

  if (!has_lock) pthread_mutex_lock(&D->mutex);
  *(D->tail) = O;
  D->tail = &O->next;
  if (!has_lock) pthread_mutex_unlock(&D->mutex);

  sem_post(D->counter);
}

bool _tr_commit_main(Handler *H, enum CommitType commit_type) {
  Database * const db = H->database;
  uint64_t end_time;

  if (commit_type == CT_ASYNC) commit_type = H->commit_type;
  
  if (fstack_empty(H->log)) {
    uint64_t end_time = atomic_add_and_fetch(&H->database->time, 2);
    _tr_unlock(H, end_time);
    // read-only transaction
    handler_cleanup(H);

    if (commit_type == CT_SYNC) {
      sendServiceMsg(H->database, { 
        .type = DB_SERVICE__SYNC_ME,
        .lock = H->write_finished
      });
      sem_wait(H->write_finished);
    }

    return true;
  }

  struct OutputList *O = simple_allocator_alloc(&output_list_allocator);
 
  O->lock = ((commit_type == CT_SYNC) ? H->write_finished : H->pending_transactions);
  O->type = ((commit_type == CT_SYNC) ? DB_SERVICE__SYNC_COMMIT: DB_SERVICE__COMMIT);

#if !SIMPLE_SERVICE_THREAD
  fstack_swap(H->log, O->log);
#endif

#if FAST_COMMIT
  end_time = atomic_add_and_fetch(&db->time, 2);
# if !SIMPLE_SERVICE_THREAD || DB_DEFER_DEALLOC
  O->end_time = end_time;
# endif

  output_queue_push(db, O, false);

  if (!tr_validate(H)) {
# if !SIMPLE_SERVICE_THREAD
    fstack_swap(H->log, O->log);
# endif
    sem_post(&O->ready);
    _tr_abort_main(H);
    return false;
  }
#else
  pthread_mutex_lock(&db->mutex); {
    if (!tr_validate(H)) {
      // readset invalid, abort
      pthread_mutex_unlock(&db->mutex);

# if !SIMPLE_SERVICE_THREAD
      fstack_swap(H->log, O->log);
# endif
      simple_allocator_free(&output_list_allocator, O);
      _tr_abort_main(H);
      return false;
    }

    end_time = atomic_add_and_fetch(&db->time, 2);
# if !SIMPLE_SERVICE_THREAD || DB_DEFER_DEALLOC
    O->end_time = end_time;
# endif

    output_queue_push(db, O, 1);
  } pthread_mutex_unlock(&db->mutex);
#endif

#if SIMPLE_SERVICE_THREAD && !DB_DEFER_DEALLOC
  process_transaction_log(H->log, db, O->W, end_time);
#endif

  _tr_unlock(H, end_time);

#if SIMPLE_SERVICE_THREAD && DB_DEFER_DEALLOC
  process_transaction_log(H->log, db, O->W, end_time, O->garbage);
#endif

#if SIMPLE_SERVICE_THREAD || FAST_COMMIT
  sem_post(&O->ready);
#endif

  handler_cleanup(H);

  sem_wait((commit_type == CT_SYNC) ? H->write_finished : H->pending_transactions);

  return true;
}


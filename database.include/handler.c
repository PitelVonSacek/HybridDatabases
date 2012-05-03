Handler *db_handler_create(Database *D) {
  Handler *H = xmalloc(sizeof(Handler));
  db_handler_init(D, H);
  H->allocated = true;
  return H;
}

void db_handler_free (Handler *H) {
  db_handler_destroy(H);
  free(H);
}

Handler *db_handler_init(Database *D, Handler *H) {
  *H = (Handler){  
    .database = D,
    .start_time = 0,
    .allocated = false,

    .commit_type = CT_ASYNC,
#if INPLACE_NODE_LOCKS || INPLACE_INDEX_LOCKS
    .acquired_locks = { StackInit },
    .read_set = StackInit
#else
    .acquired_locks = { InlineStackInit }
#endif
  };

  sem_init(H->write_finished, 0, 0);
  sem_init(H->pending_transactions, 0, BD_MAX_PENDING_TRANSACTIONS_PER_HANDLER);

  fstack_init(H->transactions);
  fstack_init(H->log);

#if !INPLACE_NODE_LOCKS && !INPLACE_INDEX_LOCKS
  bit_array_init(&H->read_set);
#endif

  pthread_mutex_lock(D->handlers_mutex);
  stack_push(D->handlers, H);
  pthread_mutex_unlock(D->handlers_mutex);

  return H;
}

void db_handler_destroy(Handler *H) {
  if (H->start_time) _tr_abort_main(H);

  Database *D = H->database;

  pthread_mutex_lock(D->handlers_mutex);
  stack_for_each(i, D->handlers) if (*i == H) {
    *i = stack_top(D->handlers);
    stack_pop(D->handlers);
    /* *i = stack_pop(handlers) is WRONG cause pop can shrink stack */
    goto unlock;
  }

  dbDebug(E, "Destroying handler, but handler was NOT registered");

  unlock:
  pthread_mutex_unlock(D->handlers_mutex);

#if INPLACE_NODE_LOCKS || INPLACE_INDEX_LOCKS
  stack_destroy(&H->read_set);
  stack_destroy(H->acquired_locks);
#else
  bit_array_destroy(&H->read_set);
#endif
  
  sem_destroy(H->write_finished);
  sem_destroy(H->pending_transactions);

  fstack_destroy(H->transactions);
  fstack_destroy(H->log);
}


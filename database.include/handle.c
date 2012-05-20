/**
 * @file
 * @brief Funkce pro vytváření a ničení handlů.
 *
 *//*
 * Implmentované funkce:
 * Handle *db_handle_create(Database *D);
 * void db_handle_free (Handle *H);
 * Handle *db_handle_init(Database *D, Handle *H);
 * void db_handle_destroy(Handle *H);
 */

Handle *db_handle_create(Database *D) {
  Handle *H = xmalloc(sizeof(Handle));
  db_handle_init(D, H);
  H->allocated = true;
  return H;
}

void db_handle_free (Handle *H) {
  db_handle_destroy(H);
  free(H);
}

Handle *db_handle_init(Database *D, Handle *H) {
  *H = (Handle){  
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

  pthread_mutex_lock(D->handles_mutex);
  stack_push(D->handles, H);
  pthread_mutex_unlock(D->handles_mutex);

  return H;
}

void db_handle_destroy(Handle *H) {
  if (H->start_time) _tr_abort_main(H);

  Database *D = H->database;

  pthread_mutex_lock(D->handles_mutex);
  stack_for_each(i, D->handles) if (*i == H) {
    *i = stack_top(D->handles);
    stack_pop(D->handles);
    /* *i = stack_pop(handles) is WRONG cause pop can shrink stack */
    goto unlock;
  }

  dbDebug(E, "Destroying handle, but handle was NOT registered");

  unlock:
  pthread_mutex_unlock(D->handles_mutex);

#if INPLACE_NODE_LOCKS || INPLACE_INDEX_LOCKS
  stack_destroy(&H->read_set);
  stack_destroy(H->acquired_locks);
#else
  bit_array_destroy(&H->read_set);
#endif

  for (int i = 0; i < BD_MAX_PENDING_TRANSACTIONS_PER_HANDLER; i++)
    sem_wait(H->pending_transactions);

  sem_destroy(H->write_finished);
  sem_destroy(H->pending_transactions);

  fstack_destroy(H->transactions);
  fstack_destroy(H->log);
}


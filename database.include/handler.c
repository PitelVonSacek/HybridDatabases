Handler *db_handler_create(Database *D) {
  Handler *H = malloc(sizeof(Handler));
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
    .acquired_locks = { InlineStackInit }
  };

  sem_init(H->write_finished, 0, 0);

  fstack_init(H->transactions, &transaction_allocator);
  fstack_init(H->log, &log_allocator);

  bit_array_init(&H->read_set);

  pthread_mutex_lock(D->handlers_mutex);
  stack_push(D->handlers, H);
  pthread_mutex_unlock(D->handlers_mutex);

  return H;
}

void db_handler_destroy(Handler *H) {
  if (H->start_time) _tr_abort_main(H);

  typeof(&*H->database->handlers) handlers = H->database->handlers;

  pthread_mutex_lock(H->database->handlers_mutex); {
    stack_for_each(i, handlers) if (*i == H) {
      *i = stack_top(handlers);
      stack_pop(handlers);
      /* *i = stack_pop(handlers) is WRONG cause pop can shrink stack */
      break;
    }
  } pthread_mutex_unlock(H->database->handlers_mutex);

  bit_array_destroy(&H->read_set);
  
  sem_destroy(H->write_finished);

  fstack_destroy(H->transactions);
  fstack_destroy(H->log);
}


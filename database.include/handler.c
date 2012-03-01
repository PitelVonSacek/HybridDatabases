Handler *db_handler_create(Database *D) {
  Handler *H = db_alloc(sizeof(Handler));
  db_handler_init(D, H);
  return H;
}

void db_handler_free(Handler *H) {
  db_handler_destroy(H);
  db_free(H);
}

void db_handler_init(Database *D, Handler *H) {
  *H = (Handler){  
    .database = D,
    .start_time = 0,
    
    .commit_type = CT_ASYNC,
    .acquired_locks = { InlineStackInit }
  };

  fstack_init(H->transactions);
  fstack_init(H->log);

  memset(H->read_set, 0, sizeof(H->read_set));

  pthread_mutex_lock(D->handlers_mutex);
  stack_insert(D->hanndlers, H);
  pthread_mutex_unlock(D->handlers_mutex);

  return H;
}

void db_handler_destroy(Handler *H) {
  if (H->start_time) _tr_abort_main(H);

  typeof(&*H->database->handlers) handlers = H->database->handlers;

  pthread_mutex_lock(H->database->handlers_mutex); {
    stack_for_each(i, handlers) if (*i == H) {
      *i = stack_pop(handlers);
      break;
    }
  } pthread_mutex_unlock(H->database->handlers_mutex);

  fstack_destroy(H->transactions);
  fstack_destroy(H->log);
}


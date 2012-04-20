static void init_locks(Database *D) {
#if !INPLACE_NODE_LOCKS || !INPLACE_INDEX_LOCKS
  for (int i = 0; i < DB_LOCKS; i++)
    D->locks[i].value = 1;
#endif
}

static void init_node_types(Database *D) {
  const DatabaseType *type = D->type;
  
  D->node_types_count = type->node_types_count;

  for (int i = 0; i < D->node_types_count; i++) {
    D->node_types[i] = *type->node_types[i];
    D->node_types[i].update_indexes = type->update_indexes[i];
    D->node_types[i].id = i;
    node_allocator_init(D->node_types[i].allocator, D->vpage_allocator,
                        &D->node_types[i]);
  }
}

static void destroy_node_types(Database *D) {
  for (int i = 0; i < D->node_types_count; i++) 
    node_allocator_destroy(D->node_types[i].allocator);
}

static Database *database_alloc(const DatabaseType *type) {
  Database *D = xmalloc(type->size);

  *(const DatabaseType**)D = type;
  D->filename = 0;
  D->current_file_index = 0;
  D->flags = 0;

  D->time = 1;
  D->node_id_counter = 1;

  init_locks(D);

  stack_init(D->handlers);

  sem_init(&D->service_thread_pause, 0, 0);

  pthread_mutex_init(&D->sync_helper_mutex, 0);
  D->sync_helper_period = 0;

#if !LOCKLESS_COMMIT
  pthread_mutex_init(&D->mutex, 0);
#endif

  vpage_allocator_init(D->vpage_allocator,
    DB_VPAGE_ALLOCATOR_CACHE, (uint64_t(*)(void*))&get_time, D);
  generic_allocator_init(D->tm_allocator,
    DB_GENERIC_ALLOCATOR_CACHE, (uint64_t(*)(void*))&get_time, D);

  D->file = 0;
  sem_init(D->counter, 0, 0);
  pthread_mutex_init(D->dump_running, 0);
  D->head = 0;
  D->tail = &D->head;

  init_node_types(D);
  type->init_indexes(D);

  return D;
}


void database_close(Database *D) {
  if (!stack_empty(D->handlers)) {
    dbDebug(DB_WARNING, "Destroying database, but some Handlers still exist");
    stack_for_each(H, D->handlers) {
      if (H[0]->allocated) db_handler_free(*H);
      else db_handler_destroy(*H);
    }
  }

  sem_post(D->counter);

  dbDebug(DB_INFO, "Waiting for service thread");
  pthread_join(D->service_thread, 0);
  dbDebug(DB_INFO, "Waiting done");

  stack_destroy(D->handlers);

  NodeType *type = &D->node_types[0];
  for (; type - D->node_types < D->node_types_count; type++)
    node_for_each(node, type) {
      type->destroy(D->tm_allocator, node, 0);
      // no need to call node_allocator_free, cause it keeps
      // all blocks in list and will destroy them
    }

  D->type->destroy_indexes(D);
  destroy_node_types(D);

  if (D->file) fclose(D->file);

  free((void*)D->filename);

  generic_allocator_destroy(D->tm_allocator);
  vpage_allocator_destroy(D->vpage_allocator);

  sem_destroy(D->counter);
  pthread_mutex_destroy(D->dump_running);

  database_set_sync_period(D, 0.0);
  pthread_mutex_destroy(&D->sync_helper_mutex);
  
#if !LOCKLESS_COMMIT
  pthread_mutex_destroy(&D->mutex);
#endif
  sem_destroy(&D->service_thread_pause);

  free(D);
}


enum DbError database_dump (Database *D) {
  sem_t signal;
  uint64_t ans = 0;

  sem_init(&signal, 0, 0);

  sendServiceMsg(D, {
    .type = DB_SERVICE__START_DUMP,
    .lock = &signal,
    .content.answer = &ans
  });

  sem_wait(&signal);
  sem_destroy(&signal);

  return ans;
}


void database_wait_for_dump (Database *D) {
  pthread_mutex_lock(D->dump_running);
  pthread_mutex_unlock(D->dump_running);
}

void database_collect_garbage(Database* D) {
  sendServiceMsg(D, {
    .type = DB_SERVICE__COLLECT_GARBAGE
  });
}

void database_pause_service_thread(Database* D) {
  sendServiceMsg(D, {
    .type = DB_SERVICE__PAUSE,
    .lock = &D->service_thread_pause
  });
}

void database_resume_service_thread(Database* D) {
  sem_post(&D->service_thread_pause);
}

enum DbError database_create_new_file (Database *D) {
  sem_t signal;
  uint64_t ans = 0;

  sem_init(&signal, 0, 0);

  sendServiceMsg(D, {
    .type = DB_SERVICE__CREATE_NEW_FILE,
    .lock = &signal,
    .content.answer = &ans
  });

  sem_wait(&signal);
  sem_destroy(&signal);

  return ans;
}


static uint64_t _generate_magic_nr() {
  uint64_t magic_nr;

  do {
    magic_nr = 0;
    for (int i = 0; i < sizeof(magic_nr); i++) 
      magic_nr = (magic_nr << 8) | (rand() & 0xFF);
  } while (!magic_nr);

  return magic_nr;
}


static bool _database_new_file(Database *D, bool dump_begin, uint64_t magic_nr) {
  dbDebug(DB_INFO, "Creating new file (dump begin = %s, magic_nr = %Li)", 
          (dump_begin ? "true" : "false"), (unsigned long long)magic_nr);

  if (D->flags & DB_READ_ONLY) {
    dbDebug(W, "Trying to create new file while database is open read-only");
    return true;
  }

  Writer W[1];
  writer_init(W);

#if LOCKLESS_COMMIT
  {
    struct OutputList *out = node_alloc(D->allocator);
    out->flags = DB_OUTPUT__DUMP_SIGNAL;
    output_queue_push(D, out, 0);
    util_signal_signal(D->io_signal);
  }

  utilSignalWaitUntil(D->dump.signal, 
                      atomic_read(&D->dump.flags) & DB_DUMP__IO_THREAD_WAITS);

  atomic_add(&D->dump.flags, -DB_DUMP__IO_THREAD_WAITS);
#endif

  if (D->file) { 
    // first close current file
    write_file_footer(W, magic_nr);
    util_fwrite(writer_ptr(W), writer_length(W), D->file);

    fclose(D->file);

    writer_discart(W);
  }

  D->current_file_index++;

  FILE *new_file;
  {
    char *buffer = xmalloc(strlen(D->filename) + 30);
    sprintf(buffer, "%s.%i", D->filename, D->current_file_index);
    dbDebug(DB_INFO, "Opening new file '%s'", buffer);

    if (!(new_file = fopen(buffer, "wb"))) {
      dbDebug(DB_ERROR, "Opening file '%s' failed", buffer);
      writer_destroy(W);
      free(buffer);
      // FIXME ?? unblock io thread ??
      return false;
    }
    free(buffer);
  }

  write_file_header(W, D, magic_nr);
  if (dump_begin) write_dump_begin(W);
  util_fwrite(writer_ptr(W), writer_length(W), new_file);
  writer_destroy(W);

  fflush(new_file);

  D->file = new_file;

#if LOCKLESS_COMMIT
  atomic_add(&D->dump.flags, DB_DUMP__RESUME_IO_THREAD);
  util_signal_signal(D->dump.signal);
#endif

  return true;
}

double database_get_sync_period(Database *D) {
  return atomic_read(&D->sync_helper_period);
}

void database_set_sync_period(Database *D, double period) {
  bool running;
  if (period < 0.001) period = 0.0;

  pthread_mutex_lock(&D->sync_helper_mutex);
  running = (D->sync_helper_period != 0);

  if (running && period < D->sync_helper_period) {
    pthread_cancel(D->sync_helper_thread);
    pthread_join(D->sync_helper_thread, 0);
    running = false;
  }

  atomic_write(&D->sync_helper_period, period);

  if (period != 0 && !running) {
    if (pthread_create(&D->sync_helper_thread, 0, (void*(*)(void*))&sync_thread, D))
      utilDie("Thread creation failed");
  }

  pthread_mutex_unlock(&D->sync_helper_mutex);
}


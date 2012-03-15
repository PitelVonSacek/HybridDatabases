static void init_locks(Database *D) {
  for (int i = 0; i < DB_LOCKS; i++)
    D->locks[i].value = 1;
}

static void init_node_types(Database *D) {
  const DatabaseType *type = D->type;
  
  D->node_types_count = type->node_types_count;

  for (int i = 0; i < D->node_types_count; i++) {
    D->node_types[i] = *type->node_types[i];
    D->node_types[i].update_indexes = type->update_indexes[i];
    D->node_types[i].id = i;
  }
}

static void destroy_node_types(Database *D) {
  for (int i = 0; i < D->node_types_count; i++) 
    node_allocator_destroy(D->node_types[i].allocator_info);
}

static Database *database_alloc(const DatabaseType *type) {
  Database *D = malloc(type->size);

  *(const DatabaseType**)D = type;
  D->filename = 0;
  D->current_file_index = 0;
  D->flags = 0;

  D->time = 1;
  D->node_id_counter = 1;

  init_locks(D);

  list_init_head(&D->node_list);

  stack_init(D->handlers);

  sem_init(&D->service_thread_pause, 0, 0);

#ifndef LOCKLESS_COMMIT
  pthread_mutex_init(&D->mutex, 0);
#endif

  node_allocator_init(D->output.allocator, sizeof(struct OutputList));
  generic_allocator_init(D->tm_allocator);

  D->output.file = 0;
  sem_init(D->output.counter, 0, 0);
  pthread_mutex_init(D->output.dump_running, 0);
  D->output.head = 0;
  D->output.tail = &D->output.head;

  init_node_types(D);
  type->init_indexes(D);

  return D;
}


void database_close(Database *D) {
#ifdef SINGLE_SERVICE_THREAD
  sem_post(D->output.counter);

  dbDebug(DB_INFO, "Waiting for service thread");
  pthread_join(D->service_thread, 0);
#else
  {
    struct OutputList *out = node_alloc(D->output.allocator);
    out->flags = DB_OUTPUT__SHUTDOWN;
    output_queue_push(D, out);
    util_signal_signal(D->output.io_signal);
  }

  dbDebug(DB_INFO, "Waiting for IO thread");
  pthread_join(D->io_thread, 0);
  dbDebug(DB_INFO, "Waiting for GC thread");
  pthread_join(D->gc_thread, 0);
#endif
  dbDebug(DB_INFO, "Waiting done");

  struct List *item;
  while ((item = D->node_list.prev) != &D->node_list) {
    list_remove(item);
    Node *node = listGetContainer(Node, __list, item);

    node->type->destroy(D->tm_allocator, node, 0);
    node_free(node->type->allocator_info, node, 0);
  }

  D->type->destroy_indexes(D);
  destroy_node_types(D);

  if (D->output.file) fclose(D->output.file);

  free((void*)D->filename);

  node_allocator_destroy(D->output.allocator);
  generic_allocator_destroy(D->tm_allocator);

  sem_destroy(D->output.counter);
  pthread_mutex_destroy(D->output.dump_running);
  
  pthread_mutex_destroy(&D->mutex);
  sem_destroy(&D->service_thread_pause);

  if (!stack_empty(D->handlers)) {
    dbDebug(DB_WARNING, "Destroying database, but some Handlers still exist");
    while (!stack_empty(D->handlers)) {
      Handler *H = stack_pop(D->handlers);
      if (H->allocated) db_handler_free(H);
      else db_handler_destroy(H);
    }
  }
  stack_destroy(D->handlers);

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
  pthread_mutex_lock(D->output.dump_running);
  pthread_mutex_unlock(D->output.dump_running);
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

#if !defined(SINGLE_SERVICE_THREAD) || defined(LOCKLESS_COMMIT)  
  {
    struct OutputList *out = node_alloc(D->output.allocator);
    out->flags = DB_OUTPUT__DUMP_SIGNAL;
    output_queue_push(D, out, 0);
    util_signal_signal(D->output.io_signal);
  }

  utilSignalWaitUntil(D->dump.signal, 
                      atomic_read(&D->dump.flags) & DB_DUMP__IO_THREAD_WAITS);

  atomic_add(&D->dump.flags, -DB_DUMP__IO_THREAD_WAITS);
#endif

  if (D->output.file) { 
    // first close current file
    write_file_footer(W, magic_nr);
    fwrite(writer_ptr(W), 1, writer_length(W), D->output.file);

    fclose(D->output.file);

    writer_discart(W);
  }

  D->current_file_index++;

  FILE *new_file;
  {
    char *buffer = malloc(strlen(D->filename) + 30);
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
  fwrite(writer_ptr(W), 1, writer_length(W), new_file);
  writer_destroy(W);

  fflush(new_file);

  D->output.file = new_file;

#if !defined(SINGLE_SERVICE_THREAD) || defined(LOCKLESS_COMMIT)
  atomic_add(&D->dump.flags, DB_DUMP__RESUME_IO_THREAD);
  util_signal_signal(D->dump.signal);
#endif

  return true;
}


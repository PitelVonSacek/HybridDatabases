static void database_alloc(DatabaseType *type);

static void database_free(Database *database) {
  {
    struct OutputList *out = node_alloc(D->output.allocator);
    out->flags = DB_OUTPUT__SHUTDOWN;
    output_queue_push(D, out);
    util_signal_signal(D->output.io_signal);
  }

#ifdef SINGLE_SERVICE_THREAD
  dbDebug(DB_INFO, "Waiting for service thread");
  pthread_join(D->service_thread, 0);
#else
  dbDebug(DB_INFO, "Waiting for IO thread");
  pthread_join(D->io_thread, 0);
  dbDebug(DB_INFO, "Waiting for GC thread");
  pthread_join(D->gc_thread, 0);
#endif
  dbDebug(DB_INFO, "Waiting done");

  struct List item;
  while ((item = D->node_list.prev) != &D->node_list) {
    list_remove(item);
    Node *node = listGetContainer(Node, __list, item);
    node->type->destroy(node);
    node_free(node->type->allocator_info, node, 0);
  }

  D->type->indexes_destroy(D);

  for (int i = 0; i < D->node_types_count; i++) 
    node_free_nodes(D->node_types[i].allocator_info, 0, ~(uint64_t)0);

  if (D->output.file) fclose(D->output.file);

  free(D->filename);
  free(D);
}

static bool _database_new_file(Database *D, bool dump_begin, uint64_t magic_nr) {
  dbDebug(DB_INFO, "Creating new file (dump begin = %s, magic_nr = %Li)", 
          (dump_begin ? "true" : "false"), (unsigned long long)magic_nr);

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
  writer_destoy(W);

  fflush(new_file);

  D->output.file = new_file;

#if defined(SINGLE_SERVICE_THREAD) && !defined(LOCKLESS_COMMIT)
  atomic_add(&D->dump.flags, DB_DUMP__RESUME_IO_THREAD);
  util_signal_signal(D->dump.signal);
#endif

  return true;
}



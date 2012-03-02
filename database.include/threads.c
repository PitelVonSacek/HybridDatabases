static void *io_thread(Database *D) {
  struct OutputList *job;
  Writer W[1];

  dbDebug(DB_INFO, "IO thread on");
  writer_init(W);

  utilSignalWaitUntil(D->output.io_signal, atomic_read(&D->output.head));
 
  for (job = D->output.head; 1; job = job->next) {
    if (atomic_read(&job->flags) & DB_OUTPUT__SHUTDOWN) break;

#ifdef LOCKLESS_COMMIT
    while (!(atomic_read(&job->flags) & DB_OUTPUT__READY)) ;

    if (job->flags & DB_OUTPUT__CANCELED) goto end;
#endif    

    if (job->flags & DB_OUTPUT__DUMP_SIGNAL) {
      // FIXME sync with new file creation
      atomic_add(&D->dump.flags, DB_DUMP__IO_THREAD_WAITS);
      util_signal_signal(D->dump.singal);

      utilSignalWaitUntil(D->dump.signal, 
          atomic_read(&D->dump.flags) & DB_DUMP__RESUME_IO_THREAD);

      atomic_add(&D->dump.flags, -DB_DUMP__RESUME_IO_THREAD);

      goto end;
    }

    if (job->flags & DB_OUTPUT__RAW) 
      fwrite(job->data, 1, job->size, D->output.file);
    else {
      write_log(W, job->log);
      fwrite(write_ptr(W), 1, writer_length(W), D->output.file);
      writer_discart(W);
    }

    if (job->flags & DB_OUTPUT__SYNC_COMMIT) fflush(D->output.file);
    if (job->lock) sem_post(job->lock);

    end:
    atomic_inc(&D->output.garbage_counter);
    util_signal_signal(D->output.gc_signal);

    utilSignalWaitUntil(D->output.io_signal, atomic_read(&job->next));
  }

  // pass the message to gc thread
  atomic_inc(&D->output.garbage_counter);
  util_signal_signal(D->output.gc_signal);

  write_destroy(W);
  dbDebug(DB_INFO, "IO thread off");
  return 0;
}

static void *gc_thread(Database *D) {
  pthread_mutex_t mutex;
  struct OutputList *job;

  TrDebug("GC thread on");
  
  pthread_mutex_init(&mutex, 0);

  gc_thread_wait(D, &mutex);

  while (job = D->output.head) {
    atomic_dec(&D->output.garbage_counter);
#ifdef LOCKLESS_COMMIT
    if (job->output.flags & DB_OUTPUT_CANCELED) goto end;
#endif

    tr_free(job->output.data);

    stack_destroy(job->garbage.memory);

    while (!stack_empty(job->garbage.nodes)) {
      Node *node = stack_pop(job->garbage.nodes);
      
      list_remove(node->__list);
      
      node->type->destroy(D, node, job->end_time);
      node_free(node->type->allocator_info, node);
    }

    stack_destroy(job->garbage.nodes);

    list_join_lists(&D->node_list, &job->new_nodes);

#ifdef LOCKLESS_COMMIT
    end:
#endif

    gc_thread_wait(D, &mutex);

    D->output.head = job->next;
    node_free(D->output.allocator, job);
  }

  pthread_mutex_destroy(&mutex);

  TrDebug("GC thread off");
  return 0;
}

static void gc_thread_wait(Database *D, pthread_mutex_t *mutex) {
  do {
    unsigned flags = atomic_read(&D->dump_flags);

    if (flags & DB_DUMP__DO_GC) {
      atomic_add(&D->dump.flags, DB_DUMP__GC_RUNNING - DB_DUMP__DO_GC);
      do_gc(D);
    }

    if (flags & DB_DUMP__DO_DUMP) {
      atomic_add(&D->dump.flags, DB_DUMP__DUMP_RUNNING - DB_DUMP__DO_DUMP);
      do_dump(D);
    }

    if (atomic_read(&D->output.garbage_counter) > 0) return;
    
    pthread_mutex_lock(mutex);
    pthread_cond_wait(D->output.garbage_signal, mutex);
    pthread_mutex_unlock(mutex);
  } while (1);
}


static uint64_t update_safe_time(Database *D) {
  uint64_t safe_time = ~(uint64_t)0;

  Handler **ptr, **end;

  pthread_mutex_lock(db->handlers_mutex); {
    ptr = D->handlers->begin;
    end = D->handler->ptr;

    for (; ptr < end; ptr++) {
      uint64_t time = atomic_read(ptr[0]->start_time);
      if (time && time < safe_time) safe_time = time;
    }
  } pthread_mutex_unlock(db->handlers_mutex);

  return safe_time;
}


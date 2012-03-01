static void *io_thread(Database *D) {
  struct OutputList *job;

  TrDebug("IO thread on");

  sem_wait(D->output.writer_sem);
 
  for (job = D->output.head; job; job = job->next) {
#ifdef LOCKLESS_COMMIT
    while (!(atomic_read(job->output.flags) & DB_OUTPUT__READY)) ;

    if (job->output.flags & DB_OUTPUT__CANCELED) goto end;
#endif

    fwrite(job->output.data, 1, job->output.size, D->output.file);

    if (job->output.flags & DB_OUTPUT__SYNC_COMMIT) fflush(D->output.file);
    if (job->output.lock) sem_post(job->output.lock);

    if (job->output.flags & DB_OUTPUT__DUMP_SIGNAL) {
      // FIXME sync with new file creation
      pthread_mutex_lock(D->dump.mutex);
      D->dump.flags |= DB_DUMP__IO_THREAD_WAITS;
      pthread_cond_wait(D->dump.signal, D->dump.mutex);
      pthread_mutex_unlock(D->dump.mutex);
    }

#ifdef LOCKLESS_COMMIT
    end:
#endif

    atomic_inc(&D->output.garbage_counter);
    pthread_cond_broadcast(D->output.garbage_signal);

    sem_wait(D->output.writer_sem);
  }

  TrDebug("IO thread off");
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


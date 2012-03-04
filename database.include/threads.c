#ifdef SINGLE_SERVICE_THREAD

static void process_transaction_log(TransactionLog *log, Database *D, 
                                    Writer *W, size_t end_time, Node **dump_ptr) {
  wArray {
    fstack_for_each(item, log) {
      Node *node = item->ptr;
      switch (item->type) {
        case LI_TYPE_NODE_MODIFY:
        case LI_TYPE_ATOMIC_NODE_MODIFY: 
          write_node_modify(W, node->id, item->index, 
                            item->attr_type, item->data_new);
          break;

        case LI_TYPE_NODE_ALLOC: 
          write_node_alloc(W, node->type, node->id);
          list_add_end(&D->node_list, &node->__list);
          break;

        case LI_TYPE_NODE_DELETE:
          write_node_delete(W, node->id);

          if (*dump_ptr == node) 
            *dump_ptr = listGetContainer(Node, __list, node->__list.next);

          list_remove(&node->__list);
          node->type->destroy(node);
          node_free(node->type->allocator_info, node, end_time);
          break;

        case LI_TYPE_MEMORY_DELETE:
          generic_free(D->tm_allocator, node, end_time);
      }
    }
  } wArrayEnd;
  
  wFinish(1);

  fstack_destroy(log);
}

static void dump_node(Database *D, Writer *W, Node *node) {
  Lock *lock = D->locks + hash_ptr(node);
  uint64_t version = l_lock_(lock, 0, ~(uint64_t)0);

  if (!version) {
    // may be sleep a while ?
    return;
  }

  wArray {
    wNumber(0);
    node->type->store(W, node);
  } wArrayEnd;

  wFinish(1);

  l_unlock(lock, 0, version);
}

static void *service_thread(Database *D) {
  struct OutputList *job, *job_next;
  Writer W[1];
  bool dump_running = false;
  Node *dump_ptr = 0;

  writer_init(W);

  sem_wait(D->output.counter);

  for (job = D->output.head; job; job = job_next) {
    switch (job->type) {
      case DB_SERVICE__COMMIT:
      case DB_SERVICE__SYNC_COMMIT:
        process_transaction_log(job->log, D, W, job->end_time, &dump_ptr);
        fwrite(writer_ptr(W), 1, writer_length(W), D->output.file);
        writer_discart(W);

        if (job->type == DB_SERVICE__SYNC_COMMIT) fflush(D->output.file);
        if (job->lock) sem_post(job->lock);
        break;

      case DB_SERVICE__CREATE_NEW_FILE: {
        if (dump_running) {
          dbDebug(DB_ERROR, "Cannot create new file because dump is running");
          break;
        }

        uint64_t magic_nr = 0;
        for (int i = 0, i < sizeof(magic_nr); i++) 
          magic_nr = (magic_nr << 8) | (rand() & 0xFF);

        // FIXME what if new_file() fails ???
        _database_new_file(D, false, magic_nr);
        break;
      }

      case DB_SERVICE__START_DUMP: {
        uint64_t magic_nr = 0;
        for (int i = 0, i < sizeof(magic_nr); i++) 
          magic_nr = (magic_nr << 8) | (rand() & 0xFF);
 
        pthread_mutex_lock(D->output.dump_running);
        // FIXME what if new_file() fails ???
        _database_new_file(D, true, magic_nr);
        dump_running = true;
        dump_ptr = listGetContainer(Node, __list, D->node_list.next);
      }  
    }

    if (dump_running) {
      do {
        for (int i = 0; i < DUMP__NODES_PER_TRANSACTION; i++) {
          if (&dump_ptr->__list == &D->node_list) { // dump finished
            dump_running = false;
            
            write_dump_end(W);
            fwrite(writer_ptr(W), 1, writer_length(W), D->output.file);
            writer_discart(W);
            fflush(D->output.file);
            
            pthread_mutex_unlock(D->output.dump_running);
            goto dump_finished;
          }

          dump_node(D, W, dump_ptr);
          fwrite(writer_ptr(W), 1, writer_length(W), D->output.file);
          writer_discart(W);

          dump_ptr = listGetContainer(Node, __list, dump_ptr->__list.next);
        }

        // FIXME dirty, trywait() can fail for other reason than
        // semaphore having value 0 !!!
      } while (sem_trywait(D->output.counter));
    } else {
      dump_finished:
      sem_wait(D->output.counter);
    }
      
    job_next = job->next;
    node_free(D->output.allocator, job, 0);
  }

  writer_destroy(W);
  return 0;
}

#else


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

static void gc_destroy_tr_log(TransactionLog *log, Database *D, size_t end_time) {
  fstack_for_each(item, log) switch (item->type) {
    case LI_TYPE_MEMORY_DELETE:
      generic_free(D->tm_allocator, item->ptr, end_time);
      break;
    case LI_TYPE_NODE_DELETE: {
      Node *node = item->ptr;
      list_remove(node->__list);
      node->type->destroy(node);
      node_free(node->type->allocator_info, node, end_time);
      break;
    }

    case LI_TYPE_NODE_ALLOC: {
      Node *node = item->ptr;
      list_add_end(&D->node_list, &node->__list);
    }  
  }

  fstack_destroy(log);
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

#endif


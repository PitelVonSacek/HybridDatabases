static void dump_get_next_node(Database *D, NodeType **type, Node **node) {
  if (!D->node_types_count) return;

  if (!*type) {
    *type = &D->node_types[0];
    *node = node_get_first(*type);
  } else *node = node_get_next(*type, *node);

  while (!*node) {
    if (++*type - D->node_types >= D->node_types_count) {
      *type = 0;
      *node = 0;
      return;
    }
    *node = node_get_first(*type);
  }
}

static void process_transaction_log(TransactionLog *log, Database *D,
                                    Writer *W, size_t end_time,
                                    NodeType **dump_type, Node **dump_ptr) {
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
          write_node_alloc(W, node_get_type(node), node->id);
          break;

        case LI_TYPE_NODE_DELETE:
          write_node_delete(W, node->id);

          if (*dump_ptr == node) dump_get_next_node(D, dump_type, dump_ptr);

          node_get_type(node)->destroy(D->tm_allocator, node, end_time);
          node_allocator_free(node_get_type(node)->allocator, node, end_time);
          break;

        case LI_TYPE_MEMORY_DELETE:
          generic_allocator_free(D->tm_allocator, node, end_time);
      }
    }
  } wArrayEnd;
  
  wFinish(1);

  fstack_destroy(log);
}

static bool dump_node(Database *D, Writer *W, Node *node) {
  Lock *lock = D->locks + hash_ptr(node);
  uint64_t version = l_lock_(lock, 0, ~(uint64_t)0);

  if (!version) {
    dbDebug(I, "Dump collision");
    return false;
  }

  wArray {
    wNumber(0);
    write_node_store(W, node);
  } wArrayEnd;

  wFinish(1);

  l_unlock(lock, 0, version);

  return true;
}

// true means dump finished
static bool do_dump(Database *D, Writer *W, NodeType **dump_type, Node **dump_ptr) {
  do {
    for (int i = 0; i < DUMP__NODES_PER_TRANSACTION; i++) {
      if (!*dump_ptr) goto dump_finish;

      if (dump_node(D, W, dump_ptr[0])) {
        util_fwrite(writer_ptr(W), writer_length(W), D->output.file);
        writer_discart(W);

        dump_get_next_node(D, dump_type, dump_ptr);
      }
    }

    // FIXME dirty, trywait() can fail for other reason than
    // semaphore having value 0 !!!
  } while (sem_trywait(D->output.counter));

  return false;

  dump_finish:
  write_dump_end(W);
  util_fwrite(writer_ptr(W), writer_length(W), D->output.file);
  writer_discart(W);
  fflush(D->output.file);
  
  dbDebug(DB_INFO, "Dump finished");
  pthread_mutex_unlock(D->output.dump_running);
  return true;
}

static uint64_t get_time(Database *D) {
  uint64_t current_time = ~(uint64_t)0;

  stack_for_each(H, D->handlers) {
    uint64_t t = atomic_read(&H[0]->start_time);
    if (t && t < current_time) current_time = t; 
  }

  return current_time;
}

static void collect_garbage(Database *D) {
  dbDebug(I, "Collecting garbage...");

  dbDebug(I, "Collecting garbage done");
}

static void *service_thread(Database *D) {
  struct OutputList *job, *job_next;
  Writer W[1];
  bool dump_running = false;
  Node *dump_ptr = 0;
  NodeType *dump_type = 0;

  dbDebug(DB_INFO, "Service thread started");

  writer_init(W);

  sem_wait(D->output.counter);

  for (job = D->output.head; job; job = job_next) {
    switch (job->type) {
      case DB_SERVICE__COMMIT:
      case DB_SERVICE__SYNC_COMMIT:
        process_transaction_log(job->content.log, D, W, job->end_time,
                                &dump_type, &dump_ptr);
        util_fwrite(writer_ptr(W), writer_length(W), D->output.file);
        writer_discart(W);

        if (job->type == DB_SERVICE__SYNC_COMMIT) fflush(D->output.file);
        if (job->lock) sem_post(job->lock);
        break;

      case DB_SERVICE__SYNC_ME:
        fflush(D->output.file);
        if (job->lock) sem_post(job->lock);
        break;

      default: goto unusual_type;
    }

    resume:
    if (dump_running) {
      if (do_dump(D, W, &dump_type, &dump_ptr)) dump_running = false;
      else goto next;
    } 
    
    sem_wait(D->output.counter);
    
    next: 
    job_next = job->next;
    simple_allocator_free(&output_list_allocator, job);
  }

  if (dump_running) {
    dbDebug(DB_WARNING, "Exit requests while doing dump");

    while (!do_dump(D, W, &dump_type, &dump_ptr)) ;
  }

  writer_destroy(W);

  dbDebug(DB_INFO, "Service thread stopping");

  return 0;

  unusual_type:
  switch (job->type) {
    case DB_SERVICE__CREATE_NEW_FILE:
      if (dump_running) {
        dbDebug(O, "Cannot create new file because dump is running");
        *job->content.answer = DB_ERROR__DUMP_RUNNING;
      } else if (!_database_new_file(D, false, _generate_magic_nr())) {
        dbDebug(O, "Cannot create new file");
        *job->content.answer = DB_ERROR__CANNOT_CREATE_NEW_FILE;
      } else {
        *job->content.answer = DB_SUCCESS;
        dbDebug(I, "New file created");
      }

      sem_post(job->lock);
      goto resume;

    case DB_SERVICE__START_DUMP:
      if (dump_running) {
        dbDebug(DB_OOPS, "Cannot start dump, dump already running");
        *job->content.answer = DB_ERROR__DUMP_RUNNING;
      } else if (!_database_new_file(D, true, _generate_magic_nr())) {
        dbDebug(DB_OOPS, "Dump failed, cannot create new file");
        *job->content.answer = DB_ERROR__CANNOT_CREATE_NEW_FILE;
      } else {
        *job->content.answer = DB_SUCCESS;

        pthread_mutex_lock(D->output.dump_running);
        dbDebug(DB_INFO, "Dump started");
        dump_running = true;
        dump_get_next_node(D, &dump_type, &dump_ptr);
      }

      sem_post(job->lock);
      goto resume;

    case DB_SERVICE__COLLECT_GARBAGE:
      collect_garbage(D);
      goto resume;

    case DB_SERVICE__PAUSE:
      dbDebug(I, "Service thread paused");
      sem_wait(job->lock);
      dbDebug(I, "Service thread resumed");
      goto resume;

    case DB_SERVICE__HANDLER_REGISTER:
      stack_push(D->handlers, job->content.handler);
      sem_post(job->lock);
      goto resume;

    case DB_SERVICE__HANDLER_UNREGISTER:
      stack_for_each(i, D->handlers) if (*i == job->content.handler) {
        *i = stack_top(D->handlers);
        stack_pop(D->handlers);
        /* *i = stack_pop(handlers) is WRONG cause pop can shrink stack */

        sem_post(job->lock);
        goto resume;
      }
      utilDie();

    default: utilDie();
  }
}


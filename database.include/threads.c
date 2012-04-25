static void process_transaction_log(TransactionLog *log, Database *D,
                                    Writer *W, size_t end_time) {
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

          node_get_type(node)->destroy(D->tm_allocator, node, end_time);
          node_allocator_free(node_get_type(node)->allocator, node, end_time);
          break;

        case LI_TYPE_MEMORY_DELETE:
          generic_allocator_free(D->tm_allocator, node, end_time);
      }
    }
  } wArrayEnd;

  wFinish(1);

  fstack_erase(log);
}

static inline void buffered_write(Writer *W, const void *buffer, size_t length, int fd) {
  long diff = (long)writer_length(W) + (long)length - (long)DB_WRITE_BUFFER_SIZE;

  if (diff <= 0) {
    if (length) writer_direct_write(W, buffer, length);
  } else {
    if (length > DB_WRITE_BUFFER_SIZE / 2) {
      util_fd_write(writer_ptr(W), writer_length(W), fd);
      writer_discart(W);
      util_fd_write(buffer, length, fd);
    } else {
      util_fd_write(writer_ptr(W), writer_length(W), fd);
      writer_discart(W);

      if (length) writer_direct_write(W, buffer, length);
    }
  }
}

static inline void buffered_sync(Writer *W, int fd) {
  if (writer_length(W)) {
    util_fd_write(writer_ptr(W), writer_length(W), fd);
    writer_discart(W);
  }

  fsync(fd);
}

// true means dump finished
static bool do_dump(Database *D, Writer *W, NodeType **dump_type) {
  do {
    for (int i = 0; i < DUMP__NODES_PER_TRANSACTION; i++) {
      Node *node;
      Lock *lock;
      uint64_t version;

      get_node:
      if (!*dump_type) goto dump_finish;

      pthread_mutex_lock(&dump_type[0]->allocator->mutex);
      if (node = dump_type[0]->allocator->dump_ptr) {
        lock = &_nodeGetLock(D, node);
        version = l_lock_(lock, 0, ~(uint64_t)0);
      }
      pthread_mutex_unlock(&dump_type[0]->allocator->mutex);

      if (!node) {
        if (++*dump_type - D->node_types < D->node_types_count)
          node_allocator_dump_init(dump_type[0]->allocator);
        else *dump_type = 0;
        goto get_node;
      }

      if (!version) {
        dbDebug(I, "Dump collision");
      } else {
        wArray {
          wNumber(0);
          write_node_store(W, node);
        } wArrayEnd;

        wFinish(1);

        l_unlock(lock, 0, version);

        buffered_write(W, 0, 0, D->file_desc);

        node_allocator_dump_next(dump_type[0]->allocator);
      }
    }

    // FIXME dirty, trywait() can fail for other reason than
    // semaphore having value 0 !!!
  } while (sem_trywait(D->counter));

  return false;

  dump_finish:
  write_dump_end(W);
  buffered_sync(W, D->file_desc);

  dbDebug(DB_INFO, "Dump finished");
  pthread_mutex_unlock(D->dump_running);
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
  NodeType *dump_type = 0;

  dbDebug(DB_INFO, "Service thread started");

  writer_init(W);

  sem_wait(D->counter);

  for (job = D->head; job; job = job_next) {
    switch (job->type) {
      case DB_SERVICE__COMMIT:
      case DB_SERVICE__SYNC_COMMIT:
#if SIMPLE_SERVICE_THREAD || FAST_COMMIT
        sem_wait(&job->ready);
#endif
#if SIMPLE_SERVICE_THREAD
        buffered_write(W, writer_ptr(job->W), writer_length(job->W), D->file_desc);
        writer_discart(job->W);
#else
        process_transaction_log(job->log, D, W, job->end_time);
        buffered_write(W, 0, 0, D->file_desc);
#endif

        if (job->type == DB_SERVICE__SYNC_COMMIT) buffered_sync(W, D->file_desc);
        if (job->lock) sem_post(job->lock);
        break;

      case DB_SERVICE__SYNC_ME:
        buffered_sync(W, D->file_desc);
        if (job->lock) sem_post(job->lock);
        break;

      default: goto unusual_type;
    }

    resume:
    if (dump_running) {
      if (do_dump(D, W, &dump_type)) dump_running = false;
      else goto next;
    }

    sem_wait(D->counter);

    next:
    job_next = job->next;
    simple_allocator_free(&output_list_allocator, job);
  }

  if (dump_running) {
    dbDebug(DB_WARNING, "Exit requests while doing dump");

    while (!do_dump(D, W, &dump_type)) ;
  }

  buffered_sync(W, D->file_desc);

  writer_destroy(W);

  dbDebug(DB_INFO, "Service thread stopping");

  return 0;

  unusual_type:
  switch (job->type) {
    case DB_SERVICE__CREATE_NEW_FILE:
      buffered_sync(W, D->file_desc);

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
      buffered_sync(W, D->file_desc);

      if (dump_running) {
        dbDebug(DB_OOPS, "Cannot start dump, dump already running");
        *job->content.answer = DB_ERROR__DUMP_RUNNING;
      } else if (!_database_new_file(D, true, _generate_magic_nr())) {
        dbDebug(DB_OOPS, "Dump failed, cannot create new file");
        *job->content.answer = DB_ERROR__CANNOT_CREATE_NEW_FILE;
      } else {
        *job->content.answer = DB_SUCCESS;

        pthread_mutex_lock(D->dump_running);
        dbDebug(DB_INFO, "Dump started");
        dump_running = true;
        if (D->node_types_count) {
          dump_type = &D->node_types[0];
          node_allocator_dump_init(dump_type->allocator);
        }
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
        (void)stack_pop(D->handlers);
        /* *i = stack_pop(handlers) is WRONG cause pop can shrink stack */

        sem_post(job->lock);
        goto resume;
      }
      utilDie();

    default: utilDie();
  }
}

static inline void double_to_timespec(double d, struct timespec *ts) {
  ts->tv_sec = (time_t)d;
  ts->tv_nsec = ((long)((d - ts->tv_sec) * 1000 * 1000 * 1000));
  if (ts->tv_nsec > 999999999) ts->tv_nsec = 999999999;
}

static void *sync_thread(Database *D) {
  double period;
  struct timespec x;

  while (true) {
    sendServiceMsg(D, {
      .type = DB_SERVICE__SYNC_ME,
      .lock = 0
    });

    period = atomic_read(&D->sync_helper_period);
    double_to_timespec(period, &x);
    nanosleep(&x, 0);
  }

  return 0;
}


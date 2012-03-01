// unlocks all locks held by H
static void _tr_unlock(Handler* H, uint64_t ver) {
  Lock * const locks = H->database->locks;

  while (!istack_empty(H->acquired_locks))
    l_unlock(locks + istack_pop(H->acquired_locks), H, ver);
}

static void handler_cleanup(Handler *H) {
  atomic_write(&H->start_time, 0);

  memset(&H->read_set, 0, sizeof(H->read_set));

  fstack_erase(H->transactions);
  
  fstack_erase(H->log);
  list_init_head(&H->nodes_new);
  istack_erase(H->acquired_locks);

  H->commit_type = CT_ASYNC;
}

static void log_undo_item(Hanler *H, struct LogItem *item, uint64_t end_time) {
  switch (item->type) {
    case LI_TYPE_RAW:
    case LI_TYPE_NODE_MODIFY:
      util_write(util_offset(item->ptr, item->offset), item->data_old,
                 item->size, 0);
      break;
    case LI_TYPE_ATOMIC_RAW:
    case LI_TYPE_ATOMIC_NODE_MODIFY:
      util_write(util_offset(item->ptr, item->offset), item->data_old,
                 item->size, 1);
      break;

    case LI_TYPE_NODE_ALLOC: {
      Node *n = (Node*)item->ptr;
      n->type->destroy(n);
      node_free(n->type->allocator_info, n, end_time);
      break;
    }
    case LI_TYPE_MEMORY_ALLOC:
      generic_free(H->database->tm_allocator, item->ptr, end_time);
      break;
    case LI_TYPE_NODE_DELETE:
    case LI_TYPE_MEMORY_DELETE:
      break;
    default:
      assert(0);
  }
}

static void handler_rollback(Handler *H, struct Transaction *tr) {
  H->read_set = tr->read_set;

  const uint64_t end_time = atomic_read(&H->database->time);
 
  while (&fstack_top(H->log) != tr->pos) {
    log_undo_item(H, fstack_top(H->log), end_time);
    fstack_pop(H->log);
  }

  H->commit_type = tr->commit_type;
  istack_shrink(H->acquired_locks, tr->acquired_locks);
}

void _tr_abort_main(Handler *H) {
  const struct Transaction empty_transaction;
  memset(&empty_transaction, 0, sizeof(empty_transaction));

  handler_rollback(H, &empty_transaction);

  uint64_t end_time = atomic_add_and_fetch(&db->time, 2);
  _tr_unlock(H, end_time);
  
  handler_cleanup(H);
}

bool _tr_commit_main(Handler *H, enum CommitType commit_type) {
  Database * const db = H->database;
  uint64_t end_time;

  if (fstack_empty(H->log)) {
    // read-only transaction
    handler_cleanup(H);

    return true;
  }
  
  if (commit_type == CT_ASYNC) commit_type = H->commit_type;
  sem_t finished[1];
  if (commit_type == CT_SYNC) sem_init(finished, 0, 0);

  struct OutputList *O = node_alloc(db->output.allocator);
  *O = (struct OutputList){
    .next = 0,
    
    .flags = ((commit_type == CT_SYNC) ? DB_DUMP__SYNC_COMMIT : 0),
    .lock = ((commit_type == CT_SYNC) ? finished : 0)
  };

  fstack_init(O->log, H->log->allocator);
  fstack_swap(O->log, H->log);

#ifdef LOCKLESS_COMMIT
  end_time = atomic_add_and_fetch(&db->time, 2);
  O->end_time = end_time;

  while (!atomic_cmpswp(atomic_read(&db->output.tail), 0, O)) ;
  atomic_write(&db->output.tail, &O->next);

  if (!tr_validate(H)) {
    atomic_add(&O->output.flags, BD_OUTPUT__READY | DB_OUTPUT__CANCELED);

    if (commit_type == CT_SYNC) sem_destroy(finished);
    fstack_swap(O->log, H->log);
    _tr_abort_main(H);
    return false;
  }

  atomic_add(&O->output.flags, DB_OUTPUT__READY);
#else
  pthread_mutex_lock(&db->mutex); {
    if (!tr_validate(H)) {
      // readset invalid, abort
      pthread_mutex_unlock(&db->mutex);
      
      if (commit_type == CT_SYNC) sem_destroy(finished);
      fstack_swap(O->log, H->log);
      node_free(db->output.allocator, O);
      _tr_abort_main(H);
      return false;
    }

    end_time = atomic_add_and_fetch(&db->time, 2);

    O->garbage.end_time = end_time;

    *(db->output.tail) = O;
    db->output.tail = &O->next;
  } pthread_mutex_unlock(&db->mutex);
#endif

  pthread_cond_broadcast(db->output.it_signal);

  _tr_unlock(H, end_time);

  handler_cleanup(H);

  if (commit_type == CT_SYNC) {
    sem_wait(finished);
    sem_destroy(finished);
  }

  return true;
}


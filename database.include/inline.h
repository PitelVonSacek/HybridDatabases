static inline enum DbFlags database_get_flags(Database *D) {
  return D->flags;
}

void _tr_abort_main(Handler*);
bool _tr_commit_main(Handler*, enum CommitType);
void _tr_handler_rollback(Handler *H, struct Transaction *tr);

static inline void *_tr_memory_alloc_no_release(Handler *H, size_t size) {
  return generic_allocator_alloc(H->database->tm_allocator, size);
}

static inline void *_tr_memory_alloc(Handler *H, size_t size) {
  struct LogItem item = {
    .type = LI_TYPE_MEMORY_ALLOC,
    .ptr = generic_allocator_alloc(H->database->tm_allocator, size)
  };

  fstack_push(H->log, item);

  return item.ptr;
}

static inline void _tr_memory_free(Handler *H, void *ptr) {
  struct LogItem item = {
    .type = LI_TYPE_MEMORY_DELETE,
    .ptr = ptr
  };

  fstack_push(H->log, item);
}

static inline void tr_hard_abort(Handler *H) {
  _tr_abort_main(H);
}

static inline bool tr_is_main(Handler *H) {
  return fstack_empty(H->transactions);
}

static inline void tr_begin(Handler *H) {
  if (H->start_time) { // create nested transaction
    struct Transaction nt = {
#if INPLACE_NODE_LOCKS || INPLACE_INDEX_LOCKS
      .read_set = stack_size(&H->read_set),
      .acquired_locks = stack_size(H->acquired_locks),
#else
      .read_set = H->read_set,
      .acquired_locks = istack_size(H->acquired_locks),
#endif
      .pos = &fstack_top(H->log),
      .commit_type = H->commit_type
    };

    fstack_push(H->transactions, nt);
  } else { // start main transaction
    atomic_write(&H->start_time, atomic_read(&(H->database->time)));
  }
}

static inline void tr_abort(Handler *H) {
  if (fstack_empty(H->transactions)) _tr_abort_main(H);
  else {  
    _tr_handler_rollback(H, &fstack_top(H->transactions));
    fstack_pop(H->transactions);
  }
}

static inline bool tr_commit(Handler *H, enum CommitType commit_type) {
  if (fstack_empty(H->transactions)) return _tr_commit_main(H, commit_type);
  
  // commit nested transaction
  fstack_pop(H->transactions);
  
  switch (commit_type) {
    case CT_SYNC:
      H->commit_type = CT_SYNC;
      break;
    case CT_FORCE_ASYNC:
      H->commit_type = CT_ASYNC;
    case CT_ASYNC: ;
  }

  return true;
}

static inline bool tr_validate(Handler *H) {
  const uint64_t start_time = H->start_time;

#if INPLACE_NODE_LOCKS || INPLACE_INDEX_LOCKS
  for (int i = 0; i < stack_size(&H->read_set); i++)
    if (!l_check(stack_at(&H->read_set, i), H, start_time)) return false;
#else
  Lock *locks = H->database->locks;

  bitArrayFor(i, &H->read_set) {
    if (!l_check(locks + i, H, start_time)) return false;
  } bitArrayForEnd;
#endif

  return true;
}

static inline bool tr_node_update_indexes(Handler *H, Node *node) {
  return node_get_type(node)->update_indexes(H, CBE_NODE_MODIFIED, node);
}

static inline bool tr_node_check(Handler *H, Node *node) {
  return l_check(&_nodeGetLock(H->database, node), H, H->start_time);
}

static inline bool _node_ref_count_increase(Handler *H, Node *node) {
  if (!util_lock(H, &_nodeGetLock(H->database, node))) return false;
  trMemoryInternalWrite_(H, &node->ref_count, node->ref_count + 1);
  return true;
}

static inline bool _node_ref_count_decrease(Handler *H, Node *node) {
  if (!util_lock(H, &_nodeGetLock(H->database, node))) return false;
  assert(node->ref_count > 0);
  trMemoryInternalWrite_(H, &node->ref_count, node->ref_count - 1);
  return true;
}



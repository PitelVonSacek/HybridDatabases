void _tr_abort_main(Handler*);
bool _tr_commit_main(Handler*, enum CommitType);

static inline void tr_begin(Handler *H) {
  if (H->start_time) { // create nested transaction
    struct Transaction nt = {
      .read_set = H->read_set,
      .pos = &fstack_top(H->log),
      .acquired_locks = istack_size(H->acquired_locks),
      .commit_type = H->commit_type
    };

    fstack_push(H->transactions, nt);
  } else { // start main transaction
    atomic_write(&H->start_time, atomic_read(&(H->database->time)));
    w_array(H->W);
  }
}

static inline void tr_abort(Handler *H) {
  if (stack_empty(H->transactions)) _tr_abort_main(H);
  else handler_rollback(H, &stack_pop(H->transactions));
}

static inline bool tr_commit(Handler *H, enum CommitType commit_type) {
  if (stack_empty(H->transactions)) return _tr_commit_main(H, commit_type);
  
  // commit nested transaction
  stack_pop(H->transactions);
  
  switch (commit_type) {
    case CT_SYNC:
      H->commit_type = CT_SYNC;
      break;
    case CT_FORCE_ASYNC:
      H->commit_type = CT_ASYNC;
  }

  return true;
}

static inline bool tr_validate(Handler *H) {
  bool valid = true;
  Lock *locks = H->database->locks;
  const uint64_t start_time = H->start_time;

  for (unsigned i = 0; i < DB_LOCKS; i++)
    if (bit_array_isset(H->read_set.read_set, i) && !l_check(locks + i, H, start_time))
      valid = false;

  return valid;
}


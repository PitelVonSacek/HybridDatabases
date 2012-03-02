#define trRead(...) trRead_(H, __VA_ARGS__)
#define trUncheckedRead(...) trUncheckedRead_(H, __VA_ARGS__)
#define trWrite(...) trWrite_(H, __VA_ARGS__)
#define trCheck(...) trCheck_(H, __VA_ARGS__)

#define trInternalRead(...) trInternalRead_(H, __VA_ARGS__)
#define trInternalWrite(...) trInternalWrite_(H, __VA_ARGS__)

#define trNodeCreate(...) trNodeCreate_(H, __VA_ARGS__)
#define trNodeDelete(...) trNodeDelete_(H, __VA_ARGS__)

#define trNodeCast(Type, node) \
  static_if(types_equal(Type*, typeof(node)), \
    (node), \
    static_if(types_equal(Type, Node), \
      static_if(types_equal(typeof((node)->__ancestor), Node), \
        (Node*)(node), \
        (void)0, \
      ), \
      ({ \
        Node *__node = (node);
        (__node->type->name == Type##_desc->name) ? \
          ((Type*)__node) : (Type*)0;
      }) \
    ) \
  )


#define trInternalRead_(H, node, AttrName) \
  ((const (typeof((node)->AttrName.value.value)))((node)->AttrName.value.value)) 


#define trInternalWrite_(H, __node, AttrName, value_) \
  ({ \
    typeof(value) __value = (value_); \
    bool __ret = true; \
    struct LogItem __log_item = { \
      .ptr = __node, \
      .type = LI_TYPE_NODE_MODIFY, \
      .size = sizeof(__node->AttrName.value.value), \
      .index = StaticGetInt(__node->_internal_##AttrName##__index), \
      .offset = utilGetOffset(__node, &__node->AttrName.value.value), \
      .attr_type = StaticGetInt(__node->AttrName.type_index) \
    }; \      
    *(typeof(&__node->AttrName.value.value))&__log_item.data_old = \
        __node->AttrName.value.value; \
    if (StaticIsTrue((node)->AttrName.is_primitive)) { \    
      *(typeof(&__node->AttrName.value.value))&__log_item.data_new = __value; \
      __node->AttrName.value.value = __value; \
    } else { \
      __ret = attribute_write(StatiGetInt(__node->AttrName.type_index), \
        H, atomic_read(&H->database->time), &__node->AttrName.value, &__value); \
      *(typeof(&__node->AttrName.value.value))&__log_item.data_new = \
          __node->AttrName.value.value; \
    } \
    fstack_push(H->log, __log_item); \
    __ret; \
  })


#define trUncheckedRead_(H, node, AttrName) \
  ({ \
    typeof(node) __node = (node); \
    bit_array_set(H->read_set.read_set, hash_ptr(__node)); \
    trInternalRead_(H, __node, AttrName); \
  })

#define trRead_(H, node, AttrName) \
  ({ \
    typeof(node) __node = (node); \
    const uint64_t __hash = hash_ptr(__node); \
    bit_array_set(H->read_set.read_set, __hash); \
    typeof(trInternalRead_(H, __node, AttrName)) __ret = \
      trInternalRead_(H, __node, AttrName); \
    if (!l_check(H->database->locks + __hash, H, H->start_time)) trFail; \
    __ret; \
  })

#define trWrite_(H, node, AttrName, value) \
  do { \
    typeof(node) __node = (node); \
    if (!l_lock(H->database->locks + hash_ptr(__node), H, H->start_time) || \
        !trInternalWrite_(H, __node, AttrName, value))  trFail; \
  } while (0)

#define trCheck_(H, node) \
  do { \
    if (!l_check(H->database->locks + hash_ptr(node), H, H->start_time)) trFail; \
  } while (0)


#define trNodeCreate_(H, Type) \
  ({ \
    Node *__node = tr_node_create(H, Type##_desc); \
    if (!__node) trFail; \
    (Type*)__node;
  })

#define trNodeDelete_(H, node) \
  do { \
    if (!tr_node_delete(H, node)) trFail; \
  } while (0)


/**********************
 * Transaction macros *
 **********************/

void _tr_retry_wait(int loop);

#define trBegin trBegin_(H)
#define trCommit(type, restart_command) trCommit_(H, type, restart_command)
#define trAbort trAbort_(H)
#define trRestart trRestart_(H)

#define trBegin_(H) \
  do { \
    __label__ _tr_success_label; _tr_restart_label; \
    do {
      __label__ tr_failed, _tr_retry_label; \
      int _tr_retry = 0; \
      _tr_retry_label: \
      tr_begin(H); \
      do {

#define trCommit_(H, type, restart_command) \
      } while (0); \
      if (!tr_commit(H, type)) { \
        if (0) { \
          tr_failed: \
          if (!tr_is_main(H)) goto _tr_restart_label; \
          tr_hard_abort(H);
        } \
        _tr_retry_wait(++_tr_retry); \
        goto _tr_retry_label; \
      } \
    } while (0); \
    if (0) { _tr_restart_label: restart_command; } \
    _tr_success_label: ;
  } while (0);


#define trAbort_(H) do { tr_abort(H); goto _tr_success_label; } while (0)
#define trRestart_(H) goto tr_failed


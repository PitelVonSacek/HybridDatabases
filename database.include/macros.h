#define trFail goto tr_failed

#define trRead(...) trRead_(H, __VA_ARGS__)
#define trUncheckedRead(...) trUncheckedRead_(H, __VA_ARGS__)
#define trWrite(...) trWrite_(H, __VA_ARGS__)
#define trUpdateIndexies(...) trUpdateIndexies_(H, __VA_ARGS__)
#define trCheck(...) trCheck_(H, __VA_ARGS__)

#define trInternalRead(...) trInternalRead_(H, __VA_ARGS__)
#define trInternalWrite(...) trInternalWrite_(H, __VA_ARGS__)

#define trNodeCreate(...) trNodeCreate_(H, __VA_ARGS__)
#define trNodeDelete(...) trNodeDelete_(H, __VA_ARGS__)

#define trMemoryRead(...) trMemoryRead_(H, __VA_ARGS__)
#define trMemoryUncheckedRead(...) trMemoryUncheckedRead_(H, __VA_ARGS__)
#define trMemoryWrite(...) trMemoryWrite_(H, __VA_ARGS__)

#define nodeCast(Type, node) \
  static_if(types_equal(Type*, typeof(node)), \
    (node), \
    static_if(types_equal(Type, Node), \
      static_if(types_equal(typeof((node)->__ancestor), Node), \
        (Node*)(node), \
        (void)0 \
      ), \
      ({ \
        Node *__node = (node); \
        (node_get_type(__node)->name == Type##_desc.name) ? \
          ((Type*)__node) : (Type*)0; \
      }) \
    ) \
  )


#define trMemoryInternalRead_(H, ptr) (*(volatile typeof(ptr))(ptr))
#define trMemoryInternalWrite_(H, ptr_, val) \
  do { \
    struct LogItem __log_item = { \
      .ptr = (ptr_), \
      .type = LI_TYPE_RAW, \
      .offset = 0, \
      .size = sizeof(*(ptr_)) \
    }; \
    memcpy(__log_item.data_old, __log_item.ptr, sizeof(*(ptr_))); \
    *(volatile typeof(&*(ptr_)))__log_item.ptr = (val); \
    fstack_push(typeUncast(H)->log, __log_item); \
  } while (0)

#define trMemoryUncheckedRead_(H, obj, attr) \
  ({ \
    typeof(&*(obj)) __obj = (obj); \
    bit_array_set(&typeUncast(H)->read_set, hash_ptr(__obj)); \
    trMemoryInternalRead_(H, &(__obj attr)); \
  })

#define trMemoryRead_(H, obj, attr) \
  ({ \
    typeof(&*(obj)) __obj = (obj); \
    const uint64_t __hash = hash_ptr(__obj); \
    bit_array_set(&typeUncast(H)->read_set, __hash); \
    typeof(trMemoryInternalRead_(H, &(__obj attr))) __ret = \
      trMemoryInternalRead_(H, &(__obj attr)); \
    if (!l_check(H->database->locks + __hash, H, typeUncast(H)->start_time)) trFail; \
    __ret; \
  })

#define trMemoryWrite_(H, obj, attr, value) \
  do { \
    typeof(&*(obj)) __obj = (obj); \
    if (!utilLock(typeUncast(H), __obj)) trFail; \
    trMemoryInternalWrite_(H, &(__obj attr), value); \
  } while (0)


#define trInternalRead_(H, node, AttrName) \
  ((const typeof((node)->AttrName.value))((node)->AttrName.value)) 


#define trInternalWrite_(H, __node, AttrName, value_) \
  ({ \
    typeof((__node)->AttrName.value) __value = (value_); \
    bool __ret = true; \
    struct LogItem __log_item = { \
      .ptr = __node, \
      .type = LI_TYPE_NODE_MODIFY, \
      .size = attributeSize(__node->AttrName), \
      .index = StaticGetInt(__node->_internal_##AttrName##_index), \
      .offset = utilOffsetOf(typeof(*__node), AttrName), \
      .attr_type = attributeTypeId(__node->AttrName) \
    }; \
    memcpy(__log_item.data_old, &__node->AttrName.value, \
           sizeof(__node->AttrName.value)); \
    if (attributeIsPrimitive((__node)->AttrName)) { \
      memcpy(__log_item.data_new, &__value, \
             sizeof(__node->AttrName.value)); \
      __node->AttrName.value = __value; \
    } else { \
      __ret = attribute_write(attributeTypeId(__node->AttrName), \
        typeUncast(H), &__node->AttrName, &__value); \
      memcpy(__log_item.data_new, &__node->AttrName.value, \
             sizeof(__node->AttrName.value)); \
    } \
    fstack_push(typeUncast(H)->log, __log_item); \
    __ret; \
  })


#define trUncheckedRead_(H, node, AttrName) \
  ({ \
    typeof(node) __node = (node); \
    bit_array_set(&typeUncast(H)->read_set, hash_ptr(__node)); \
    trInternalRead_(H, __node, AttrName); \
  })

#define trRead_(H, node, AttrName) \
  ({ \
    typeof(node) __node = (node); \
    const uint64_t __hash = hash_ptr(__node); \
    bit_array_set(&typeUncast(H)->read_set, __hash); \
    typeof(trInternalRead_(H, __node, AttrName)) __ret = \
      trInternalRead_(H, __node, AttrName); \
    if (!l_check(H->database->locks + __hash, H, typeUncast(H)->start_time)) trFail; \
    __ret; \
  })

#define trWrite_(H, node, AttrName, value) \
  do { \
    typeof(node) __node = (node); \
    if (!utilLock(typeUncast(H), __node) || \
        !trInternalWrite_(H, __node, AttrName, value))  trFail; \
  } while (0)


#define trUpdateIndexies_(H, node) \
  do { \
    if (!tr_node_update_indexies(typeUncast(H), node)) trFail; \
  } while (0)


#define trCheck_(H, node) \
  do { \
    if (!l_check(H->database->locks + hash_ptr(node), H, typeUncast(H)->start_time)) \
      trFail; \
  } while (0)


#define trNodeCreate_(H, Type) \
  ({ \
    Node *__node = tr_node_create(typeUncast(H), \
                                  &H->my_database->node_types.Type##_desc); \
    if (!__node) trFail; \
    (Type*)__node; \
  })

#define trNodeDelete_(H, node) \
  do { \
    if (!tr_node_delete(typeUncast(H), node)) trFail; \
  } while (0)


#define trIndex(...) trIndex_(H, __VA_ARGS__)
#define trIndex_(H, index, method, ...) \
  ({ \
    typeof(H->my_database->indexes.index.functions.method##_return_t[0]) __return; \
    if (!H->my_database->indexes.index.functions.method( \
           &H->my_database->indexes.index.context, \
           typeUncast(H), &__return, ## __VA_ARGS__)) trFail; \
    __return; \
  })

/**********************
 * Transaction macros *
 **********************/

void _tr_retry_wait(int loop);

#define trBegin trBegin_(H)
#define trCommit(type, ...) trCommit_(H, type, __VA_ARGS__)
#define trAbort trAbort_(H)
#define trRestart trRestart_(H)

#define trBegin_(H) \
  do { \
    __label__ _tr_success_label, _tr_restart_label; \
    do { \
      __label__ tr_failed, _tr_retry_label; \
      int _tr_retry = 0; \
      _tr_retry_label: \
      tr_begin(H); \
      do {

#define trCommit_(H, type, ...) \
      } while (0); \
      if (!tr_commit(H, type)) { \
        if (0) { \
          tr_failed: __attribute__((unused)); \
          if (!tr_is_main(H)) { \
            tr_abort(H); \
            goto _tr_restart_label; \
          } \
          tr_hard_abort(H); \
        } \
        _tr_retry_wait(++_tr_retry); \
        goto _tr_retry_label; \
      } \
    } while (0); \
    if (0) { _tr_restart_label: \
      do { __VA_ARGS__; } while (0); \
      assert(0); \
    } \
    _tr_success_label: __attribute__((unused)); \
  } while (0)


#define trAbort_(H) do { tr_abort(H); goto _tr_success_label; } while (0)
#define trRestart_(H) goto tr_failed


/****************
 *   Database   *
 ****************/

#define dbCreate(Type, ...) ((Type*)database_create(&Type##_desc, __VA_ARGS__))

#define dbHandlerCreate(D) \
  ((typeof(D->__dummy_handler[0])*)db_handler_create(D))


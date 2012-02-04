#ifndef DATABASE_NODE_MACROS_H
#define DATABASE_NODE_MACROS_H

#include "database_types.h"

/*
  Macros defined here:
  --------------------

  Node(Type)

  NodeCast_(Type, node)
  NodeCase(Type, node)

  TrRead_(tr, node, Attr, var)
  TrWrite_(tr, node, Attr, val)
  TrVerify_(tr, node)

  TrRead(node, Attr)
  TrWrite(node, Attr, val)
  TrVerify(node)

  TrBlockRead_(tr, ptr, var)
  TrBlockWrite_(tr, ptr, var)
  TrBlockVerify_(tr, ptr)

  TrBlockRead(ptr)
  TrBlockWrite(ptr, val)
  TrBlockVerify(ptr)
*/

// private macros

#define TrLockLock_(tr, lock) l_lock(&tr->database->locks[lock % DB_LOCKS], tr, tr->start_time)
#define TrLockUnlock_(tr, lock, val) l_unlock(&tr->database->locks[lock % DB_LOCKS], tr, val)
#define TrLockCheck_(tr, lock) l_check(&tr->database->locks[lock % DB_LOCKS], tr, tr->start_time)

#define TrHash(ptr) (((size_t)ptr) * 0x01008041 /* FIXME */)

#define TrIncrementRefs_(tr, node) ({ \
  int _ret = TrLockLock_(tr, TrHash(node)); \
  if (_ret) (node)->ref_count++; \
  _ret;\
})
#define TrDecrementRefs_(tr, node) ({ \
  int _ret = TrLockLock_(tr, TrHash(node)); \
  if (_ret) (node)->ref_count--; \
  _ret;\
})

// end private macros


#define Node(Type) NodeType__##Type

#define NodeCast_(Type, node) ({ \
  Node* _node = (node); \
  (_node->type->name == NodeType__##Type##__desc->__type.name) ? \
  ((Node(Type)*)_node) : ((Node(Type)*)0); \
})

#define NodeCast(Type, node) ENSURE(NodeCast_(Type, node), TR_ABORT, 0)


#define TrRead_(tr, node, Attr, var) ({ \
  typeof(node) _node = (node); \
  var = _node->Attr.value; \
  TrVerify_(TrUncast(tr), _node); \
})

#define TrWrite_(tr, node, Attr, val) ({ \
  typeof(node) _node = (node); \
  typeof((node)->Attr.value) _val = (val); \
  int _ret; \
  if (_ret = TrLockLock_(TrUncast(tr), TrHash(_node))) { \
    _tr_undo_log(TrUncast(tr), &(_node->Attr), sizeof(_node->Attr)); \
    _ret = _ret && (AttrCopy(_node->Attr, TrUncast(tr), &_node->Attr.value, &_val) == TRERR_SUCCESS); \
    if (_ret) { \
      Writer *W = TrUncast(tr)->W; \
      W_ARRAY { \
        W_NUMBER(CBE_NODE_MODIFIED); \
        W_NUMBER(_node->__node.id); \
        W_NUMBER(_node->__type->Attr.index); \
        AttrStore(_node->Attr, W, &_node->Attr.value); \
      } W_ARRAY_END; \
     TrUncast(tr)->database->type->indexes_update(TrUncast(tr), CBE_NODE_MODIFIED, TrNodeUncast(_node)); \
    } \
  } \
  _ret; \
})

#define TrVerify_(tr, node) ({ \
  uint64_t _hash = TrHash(node) % DB_LOCKS; \
  bit_array_set(tr->read_set.read_set, _hash); \
  TrLockCheck_(tr, _hash); \
})


#define TrRead(node, Attr) ({ \
  typeof((node)->Attr.value) _ret; \
  if (!TrRead_(_transaction, node, Attr, _ret)) THROW(TR_FAIL, 0); \
  _ret; \
})

#define TrWrite(node, Attr, val) ({ \
  if (!TrWrite_(_transaction, node, Attr, val)) THROW(TR_FAIL, 0); \
  (void)0; \
})

#define TrVerify(node) ENSURE(TrVerify_(_transaction, (node)), TR_FAIL, 0)

#define TrBlockRead_(tr, ptr, var) ({ \
  typeof(ptr) _ptr = (ptr); \
  char *_ptr_base = (char*)(((size_t)ptr) & ((~(size_t)0) << 3)); \
  var = *_ptr; \
  TrBlockVerify_(tr, _ptr); \
})


static inline int TrBlockWrite_helper(Transaction* tr, char* ptr, size_t size) {
  for (char *ptr_ = (char*)(((size_t)ptr) & ((~(size_t)0) << 3));
       ptr_ < ptr + size;
       ptr_ += 8)
    if (!TrLockLock_(tr, TrHash(ptr_))) return 0;

  return 1;
}

#define TrBlockWrite_(tr, ptr, value) ({ \
  typeof(ptr) _ptr = (ptr); \
  if (sizeof(*_ptr) + ((size_t)_ptr) % 8 > 8)  \
    TrDebug("TrBlockRead requires mutliple locks, address %p, size %i", _ptr, (int)sizeof(*_ptr));  \
  int _ret = TrBlockWrite_helper(tr, (char*)_ptr, sizeof(*_ptr)); \
  if (_ret) { \
    *_ptr = (value); \
    for (int i = 0; i < sizeof(*_ptr); i+=8) \
      _tr_undo_log(tr, ((char*)_ptr) + i, (8 < sizeof(*_ptr) - i ? 8 : sizeof(*_ptr) - i)); \
  } \
  _ret; \
})

static inline int TrBlockVerify_helper(Transaction* tr, char* ptr, size_t size) {
  for (char *ptr_ = (char*)(((size_t)ptr) & ((~(size_t)0) << 3));
       ptr_ < ptr + size;
       ptr_ += 8)
    if (!TrLockCheck_(tr, TrHash(ptr_))) return 0;

  return 1;
}
#define TrBlockVerify_(tr, ptr) TrBlockVerify_helper(tr, (char*)(ptr), sizeof(*(ptr)))

#define TrBlockRead(ptr) ({ \
  typeof(*(ptr)) _var; \
  if (!TrBlockRead_(_transaction, ptr, _var)) THROW(TR_FAIL, 0); \
  _var; \
})
#define TrBlockWrite(ptr, val) ({ \
  if (!TrBlockWrite_(_transaction, ptr, val)) THROW(TR_FAIL, 0); \
})
#define TrBlockVerify(ptr) ENSURE(TrBlockVerify_(_transaction, (ptr)), TR_FAIL, 0)


#define TrMemoryAdd_(tr, ptr) stack_push(tr->memory_new, ptr)
#define TrMemoryDelete_(tr, ptr) stack_push(tr->memory_deleted, ptr)

#define TrMemoryAdd(ptr) TrMemoryAdd_(_transaction, ptr)
#define TrMemoryDelete(ptr) TrMemoryDelete_(_transaction, ptr)

#endif // DATABASE_NODE_MACROS_H

#ifndef ATTRIBUTES_INLINE_H
#define ATTRIBUTES_INLINE_H

#include "database_types.h"
#include "database_macros.h"

// Pointer
static inline void AttrType__Pointer__load(Reader* R, Pointer* ptr) {
  ((Pointer_t*)ptr)->id = R_NUMBER;
}

static inline void AttrType__Pointer__store(Writer* W, Node** ptr) {
  W_NUMBER(*ptr ? ptr[0]->id : 0);
}

static inline void AttrType__Pointer__init(Node** ptr) {
  *ptr = 0;
}

static inline void AttrType__Pointer__destroy(Node** ptr) { *ptr = 0; }

static inline int AttrType__Pointer__copy(Transaction* _transaction, Node** dest, Node *const* src) {
  if (
    (*dest && !TrDecrementRefs_(_transaction, *dest)) ||
    (*src  && !TrIncrementRefs_(_transaction, *src ))
  ) return TRERR_COLLISION;

  *dest = *src;
  return TRERR_SUCCESS;
}


// String

static inline void AttrType__String__load(Reader* R, String* ptr) {
  size_t l = R_LENGTH;
  *ptr = tr_alloc(l + 1);
  memcpy(*ptr, R_STRING(l), l);
  ptr[0][l] = '\0';
}

static inline void AttrType__String__store(Writer* W, String* ptr) {
  size_t l = strlen(*ptr);
  memcpy(W_STRING(l), *ptr, l);
}

static inline void AttrType__String__init(String* ptr) {
  *ptr = 0;
}

static inline void AttrType__String__destroy(String* ptr) {
  if (*ptr) tr_free(*ptr);
}

static inline int AttrType__String__copy(Transaction* _transaction, String* dest, const String* src) {
  if (*dest) stack_push(_transaction->memory_deleted, *dest);
  if (*src) {
    size_t l = strlen(*src);
    *dest = tr_alloc(l + 1);
    stack_push(_transaction->memory_new, *dest);
    memcpy(*dest, *src, l + 1);
  } else *dest = 0;

  return TRERR_SUCCESS;
}

// BinaryString
static inline void AttrType__BinaryString__load(Reader* R, BinaryString* ptr) {
  size_t l = R_LENGTH;
  *ptr = tr_alloc(l + sizeof(binary_string_t));
  memcpy(ptr[0]->data, R_STRING(l), l);
  ptr[0]->length = l;
}

static inline void AttrType__BinaryString__store(Writer* W, BinaryString* ptr) {
  memcpy(W_STRING(ptr[0]->length), ptr[0]->data, ptr[0]->length);
}

static inline void AttrType__BinaryString__init(BinaryString* ptr) {
  *ptr = 0;
}

static inline void AttrType__BinaryString__destroy(BinaryString* ptr) {
  if (*ptr) tr_free(*ptr);
}

static inline int AttrType__BinaryString__copy(Transaction* _transaction, BinaryString* dest, const BinaryString* src) {
  if (*dest) stack_push(_transaction->memory_deleted, *dest);
  if (*src) {
    *dest = tr_alloc(src[0]->length + sizeof(binary_string_t));
    stack_push(_transaction->memory_new, *dest);
    memcpy(*dest, *src, src[0]->length + sizeof(binary_string_t));
  } else *dest = 0;

  return TRERR_SUCCESS;
}

#endif // ATTRIBUTES_INLINE_H

#define DefineAttrType(name, ...) name,

enum {
#include "definitions.h"
  attribute_types_count
};

#undef DefineAttrType

#define DefineAttrType(name, C_type, atomic, read, init, destroy, copy, load, store) \
  typedef C_type name##_t; \
  typedef typeof(((name##_t*)0)->value) name##_value_t;
  static inline name##_t attribute_native_read_##name ()
  static inline void attribute_init_##name (void *attr) {  init; } \
  static inline void attribute_destroy_##name (struct GenericAllocatorInfo *allocator, uint64_t end_time, \
    void *attr) { destroy; } \
  static inline bool attribute_copy_##name (Handler *H, uint64_t end_time, \
    void *dest, const void *src) { struct GenericAllocatorInfo *allocator = H->database->tm_allocator; \
                                   copy; return true; } \
  static inline void attribute_store_##name (Writer *W, void *attr) { store; } \
  static inline bool attribute_load_##name (Reader *R, struct GenericAllocatorInfo *allocator, void *attr) \
    { load; return true; }

#include "definitions.h"

#undef DefineAttrType

#define DefineAttrType(name, ...) case name: attribute_init_##name(attr); break;
void attribute_init (int type, void *attr) {
  switch (type) {
#include "definitions.h"
    default: assert(0);
  }
}
#undef DefineAttrType

#define DefineAttrType(name, ...) case name: attribute_destroy_##name(allocator, end_time, attr); break;
void attribute_destroy (int type, struct GenericAllocatorInfo *allocator, uint64_t end_time, void *attr) {
  switch (type) {
#include "definitions.h"
    default: assert(0);
  }
}
#undef DefineAttrType

#define DefineAttrType(name, ...) case name: return attribute_copy_##name(H, end_time, dest, src); break;
bool attribute_copy (int type, Handler *H, uint64_t end_time, void *dest, const void *src) {
  switch (type) {
#include "definitions.h"
    default: assert(0);
  }
}
#undef DefineAttrType

#define DefineAttrType(name, ...) case name: attribute_store_##name(W, attr); break;
void attribute_store (int type, Writer *W, void *attr) {
  switch (type) {
#include "definitions.h"
    default: assert(0);
  }
}
#undef DefineAttrType

#define DefineAttrType(name, ...) case name: return attribute_load_##name(R, allocator, attr); break;
bool attribute_load (int type, Reader *R, struct GenericAllocatorInfo *allocator, void *attr) {
  switch (type) {
#include "definitions.h"
    default: assert(0);
  }
}
#undef DefineAttrType

#define DefineAttrType(name, ...) case name: return sizeof(name##_t);
static inline size_t attribute_size(int type) {
  switch (type) {
#include "definitions.h"
    default: assert(0);
  }
}
#undef DefineAttrType


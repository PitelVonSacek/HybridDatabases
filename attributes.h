#ifndef ATTRIBUTES_H
#define ATTRIBUTES_H

#include "database_types.h"

/*
typedef struct {
  const char* name;
  size_t size;

  void (*init)(void*);
  void (*destroy)(void*);

  void (*copy)(Transaction*, void* dest, const void* src);

  void (*load)(Reader*, void*);
  void (*store)(Writer*, void*);
} AttributeType;

Naming rules:
  eg. for attr. type Int there is
  Int -- type
  AttrType__Int -- descriptor

*/

#ifndef IMPLEMENT_ATTRIBUTES
#define BasicType(name, real_type, load_, store_) \
  typedef real_type name; \
  typedef struct __attribute__ ((__packed__)) { name value; } name##_t; \
  extern const AttributeType AttrType__##name[1]; \
  static inline void AttrType__##name##__load(Reader *R, name* ptr) { \
    load_; \
   } \
  static inline void AttrType__##name##__store(Writer *W, name* ptr) { \
    store_; \
  } \
  static inline void AttrType__##name##__init(name* ptr) { *ptr = 0; } \
  static inline void AttrType__##name##__destroy(name* ptr) {} \
  static inline int  AttrType__##name##__copy(Transaction* t, name* dest, const name *src) { \
    *dest = *src; \
    return TRERR_SUCCESS; \
  }

#else

#define BasicType(name_, ...) \
  const AttributeType AttrType__##name_[1] = {{ \
    .name = #name_, \
    .size = sizeof(name_), \
    .id = AttrType__##name_##__id, \
    .init = (void(*)(void*))&AttrType__##name_##__init, \
    .destroy = (void(*)(void*))&AttrType__##name_##__destroy, \
    .copy = (int(*)(Transaction*,void*,const void*))&AttrType__##name_##__copy, \
    .load = (void(*)(Reader*,void*))&AttrType__##name_##__load, \
    .store = (void(*)(Writer*,void*))&AttrType__##name_##__store \
  }};

#endif

#define BasicType_(name, type) BasicType(name, type, \
  *ptr = *(type*)R_STRING(sizeof(type)), \
  *(type*)W_STRING(sizeof(type)) = *ptr)

BasicType_(Int, int32_t)
BasicType_(Int8, int8_t)
BasicType_(Int16, int16_t)
BasicType_(Int64, int64_t)

BasicType(UInt, uint32_t, *ptr = R_NUMBER, W_NUMBER(*ptr))
BasicType(UInt8, uint8_t, *ptr = R_NUMBER, W_NUMBER(*ptr))
BasicType(UInt16, uint16_t, *ptr = R_NUMBER, W_NUMBER(*ptr))
BasicType(UInt64, uint64_t, *ptr = R_NUMBER, W_NUMBER(*ptr))

BasicType_(Float, float)
BasicType_(Double, double)
BasicType_(LDouble, long double)

#undef BasicType_
#undef BasicType

#ifndef IMPLEMENT_ATTRIBUTES

#define MAX_ATTR_SIZE (sizeof(UniversalAttribute))

typedef struct {
  size_t length;
  unsigned char data[0];
} binary_string_t;

typedef Node* Pointer;
typedef union { Node* value; uint64_t id; } Pointer_t;

typedef unsigned char* String;
typedef struct { String value; } String_t;

typedef binary_string_t* BinaryString;
typedef struct { BinaryString value; } BinaryString_t;

// #include "attributes_inline.h"

/* Macro hell defines:
  AttrLoad(Attr, ...)
  AttrStore(Attr, ...)

  AttrCopy(Attr, ...)

  AttrInit(Attr, ...)
  AttrDestroy(Attr, ...)

  MAX_ATTR_SIZE
*/

#include "attributes_macro_hell_output.h"


#define TypeDesc(name) extern const AttributeType AttrType__##name[1];
#else
#define TypeDesc(name_) \
  const AttributeType AttrType__##name_[1] = {{ \
    .name = #name_, \
    .size = sizeof(name_), \
    .id = AttrType__##name_##__id, \
    .init = (void(*)(void*))&AttrType__##name_##__init, \
    .destroy = (void(*)(void*))&AttrType__##name_##__destroy, \
    .copy = (int(*)(Transaction*,void*,const void*))&AttrType__##name_##__copy, \
    .load = (void(*)(Reader*,void*))&AttrType__##name_##__load, \
    .store = (void(*)(Writer*,void*))&AttrType__##name_##__store \
  }};
#endif


TypeDesc(Pointer)
TypeDesc(String)
TypeDesc(BinaryString)

/* all attr types are:
  Int, Int8, Int16, Int64,
  UInt, UInt8, UInt16, UInt64,
  Float, Double, LDouble,
  Pointer, String, BinaryString
*/

#undef TypeDesc

#endif // ATTRIBUTES_H

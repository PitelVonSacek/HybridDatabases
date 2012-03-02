#define DefineAttrType(name, ...) name,

enum {
#include "definitions.h"
  attribute_types_count
};

#undef DefineAttrType

#define DefineAttrType(name, Internal_type, P, init, destroy, copy, load, store) \
  typedef struct { \
    Internal_type value; \
    P is_primitive; \
    StaticInt(name) type_index; \
  } name##_t; \
  typedef typeof(((name##_t*)0)->value.value) name##_value_t; 

#include "definitions.h"

#undef DefineAttrType


#define DefineAttrType(name, ...) name##_t name##_value;
union UniversalAttribute {
#include "definitions.h"
};
#undef DefineAttrType


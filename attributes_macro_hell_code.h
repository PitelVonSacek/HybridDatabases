
#define Data "attributes_macro_hell_data.h"

// Load
#define Types(a) @define AttrLoad(Attr, ...) a
#define Type(T) __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), T##_t), AttrType__##T##__load(ARGS),
#define End )
#define Nothing __builtin_trap()

#include Data

#undef Types
#undef Type

// Store
#define Types(a) @define AttrStore(Attr, W, p) a
#define Type(T) __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), T##_t), AttrType__##T##__store(W, (void*)(p)),

#include Data

#undef Types
#undef Type

// Copy
#define Types(a) @define AttrCopy(Attr, tr, dest, src) a
#define Type(T) __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), T##_t), AttrType__##T##__copy(tr, (void*)(dest), (const void*)(src)),

#include Data

#undef Types
#undef Type

// Init
#define Types(a) @define AttrInit(Attr, ...) a
#define Type(T) __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), T##_t), AttrType__##T##__init(ARGS),

#include Data

#undef Types
#undef Type

// Destroy
#define Types(a) @define AttrDestroy(Attr, ...) a
#define Type(T) __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), T##_t), AttrType__##T##__destroy(ARGS),

#include Data

#undef Types
#undef Type
#undef End
#undef Nothing

// MAX_ATTR_SIZE
#define Types(a) typedef union { a } UniversalAttribute;
#define Type(T) T T##__item;
#define End
#define Nothing

#include Data

#undef Types
#undef Type

#define Types(a) a
#define Type(T) AttrType__##T##__id,

enum {
#include Data
  AttrType__types_count
};

#ifndef __BASIC_UTILS_H__
#define __BASIC_UTILS_H__

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include "static_if.h"
#include "type_magic.h"

#define utilOffsetOf(type, member) __builtin_offsetof(type, member)

#define utilContainerOf(ptr, type, member) \
  ((type*)(((char*)(ptr)) - utilOffsetOf(type, member)))

#define util_get_offset(ptr, ptr_to_member) \
  (((char*)(ptr_to_member)) - ((char*)(ptr)))

#define util_apply_offset(ptr, offset) \
  ((void*)(((char*)(ptr)) + (offset)))


// Casts pointer ptr to type Type* in way that
// -fstrict-aliasing cannot screw up
#define utilCast(Type, ptr) \
  ({ \
    union { \
      Type new_; \
      typeof(*(ptr)) old; \
    } *_type_cast_helper = (typeof(_type_cast_helper))(ptr); \
    &_type_cast_helper->new_; \
  })

#define util_read(ptr, dest, size, atomic) __UTIL_READ_UNIMPLEMENTED__
#define util_write(ptr, src, size, atomic) memcpy(ptr, src, size)


// macro so utilDie reports something useful
#define util_fwrite(ptr, length, file) \
  do { \
    size_t __length = (length); \
    if (fwrite((ptr), 1, __length, (file)) != __length) utilDie("Write failed"); \
  } while (0)


#define utilEnsure(expr, ...) \
  ({ \
    typeof(expr) __ret = (expr); \
    if (!__ret) utilDie(__VA_ARGS__); \
    __ret; \
  })

#define xmalloc(size) utilEnsure(malloc(size), "malloc failed")
#define xrealloc(ptr, size) utilEnsure(realloc(ptr, size), "realloc failed")

#define utilDie(...) \
  do { \
    fprintf(stderr, "utilDie at %s line %i", __FILE__, __LINE__); \
    fprintf(stderr, ": " __VA_ARGS__); \
    fprintf(stderr, "\n"); \
    abort(); \
  } while (0)

enum {
  DB_DBG_LEVEL_DB_ERROR = 0,
  DB_DBG_LEVEL_DB_OOPS = 1,
  DB_DBG_LEVEL_DB_WARNING = 2,
  DB_DBG_LEVEL_DB_INFO = 3,

  DB_DBG_LEVEL_E = 0,
  DB_DBG_LEVEL_O = 1,
  DB_DBG_LEVEL_W = 2,
  DB_DBG_LEVEL_I = 3
};

#define utilDebug(level, ...) \
  do { \
    const char *message_type[] = { \
      "Error", "Oops", "Warning", "Info" \
    }; \
    fprintf(stderr, "%s at '%s' line %i:", \
      message_type[DB_DBG_LEVEL_##level], __FILE__, __LINE__); \
    fprintf(stderr, " " __VA_ARGS__); \
    fprintf(stderr, "\n"); \
  } while (0)

#endif


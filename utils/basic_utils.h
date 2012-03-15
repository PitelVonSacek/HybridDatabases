#ifndef __BASIC_UTILS_H__
#define __BASIC_UTILS_H__

#include <assert.h>
#include <stdio.h>

#define utilOffsetOf(type, member) __builtin_offsetof(type, member)

#define utilContainerOf(ptr, type, member) \
  ((type*)(((char*)(ptr)) - utilOffsetOf(type, member)))

#define util_get_offset(ptr, ptr_to_member) \
  (((char*)(ptr_to_member)) - ((char*)(ptr)))

#define util_apply_offset(ptr, offset) \
  ((void*)(((char*)(ptr)) + (offset)))

#define util_read(ptr, dest, size, atomic) __UTIL_READ_UNIMPLEMENTED__
#define util_write(ptr, src, size, atomic) memcpy(ptr, src, size)

#define utilDie(...) \
  do { \
    fprintf(stderr, "utilDie at %s line %i", __FILE__, __LINE__); \
    fprintf(stderr, ": " __VA_ARGS__); \
    fprintf(stderr, "\n"); \
    abort(); \
  } while (0)

#endif


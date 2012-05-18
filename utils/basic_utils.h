#ifndef __BASIC_UTILS_H__
#define __BASIC_UTILS_H__

/**
 * @file
 * @brief Základní užitečnosti.
 *
 * Obsahuje includy nejčastěji používaných hlavičkových souborů
 * a definice užitečných maker.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sched.h>

#include "static_if.h"
#include "type_magic.h"


#define unlikely(expr) __builtin_expect((expr), 0)
#define likely(expr) __builtin_expect(!!(expr), 1)


/// Ošklivé aktivní čekání. Pro tuto funkci vypneme optimalizaci.
static __attribute__((optimize("O0"),unused))
void busy_wait(int loops)  {
  for (int i = 0; i < loops; i++) asm volatile ("" ::: "memory");
}

/// V závislosti na parametru buď aktivně čekáme, nebo se necháme odplánovat.
static inline void spin_or_yield(int try) {
  if (likely(try < 4)) busy_wait(200 << try);
  else sched_yield();
}


/// Vrací offset členu @c member ve struktuře typu @c type.
#define utilOffsetOf(type, member) __builtin_offsetof(type, member)

/// Z adresy členu získáme pointer na strukturu.
#define utilContainerOf(ptr, type, member) \
  ((type*)(((char*)(ptr)) - utilOffsetOf(type, member)))

/// Získáme rozdíl dvou pointerů v bajtech.
#define util_get_offset(ptr, ptr_to_member) \
  (((char*)(ptr_to_member)) - ((char*)(ptr)))

/// Posuneme pointer o @c offset bajtů.
#define util_apply_offset(ptr, offset) \
  ((void*)(((char*)(ptr)) + (offset)))


/**
 * Přetypuje pointer @c ptr na typ <em>Type*<em> způsobem, který
 * -fstrict-aliasing nerozbije.
 */
#define utilCast(Type, ptr) \
  ({ \
    union { \
      Type new_; \
      typeof(*(ptr)) old; \
    } *_type_cast_helper = (typeof(_type_cast_helper))(ptr); \
    &_type_cast_helper->new_; \
  })


/// Makro okolo write(), buď uspějeme, nebo zemřeme.
#define util_fd_write(ptr, length, file_desc) \
  do { \
    size_t __length = (length); \
    if (write((file_desc), (ptr), __length) != __length) utilDie("Write failed"); \
  } while (0)


/// Pokud je hodnota výrazu 0, zavolá utilDie(), jinak vrátí výsledek.
#define utilEnsure(expr, ...) \
  ({ \
    typeof(expr) __ret = (expr); \
    if (!__ret) utilDie(__VA_ARGS__); \
    __ret; \
  })

/// Malloc co vždy uspěje.
#define xmalloc(size) utilEnsure(malloc(size), "malloc failed")
/// Realloc co vždy uspěje.
#define xrealloc(ptr, size) utilEnsure(realloc(ptr, size), "realloc failed")


/// Vypíše chybu a ukončí program.
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

/// Vypíše debugovací hlášku.
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


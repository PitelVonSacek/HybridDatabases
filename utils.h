#ifndef __UTILS_H__
#define __UTILS_H__

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

#include <semaphore.h>
#include <pthread.h>

#include "lock.h"

static inline int ulog2(uint64_t n) {
  // FIXME there are much better ways to compute this
  int ret = -1;
  for (; n; n /= 2) ret++;
  return ret;
}

// From GNU libc documentation:
// The address of a block returned by malloc or realloc in the GNU system is
// always a multiple of eight (or sixteen on 64-bit systems).
//


#define utilLock(H, ptr) \
  do { \
    switch (l_lock(H->database->locks + hash_ptr(node), H, H->start_time)) { \
      case 0: return false; \
      case 1: istack_push(H->acquired_locks, hash_ptr(node)); \
    } \
  } while (0)

static inline unsigned hash_ptr(void *ptr) {
  return (((size_t)ptr) * 997) % DB_LOCKS; // find betterr prime :-)
}

#define util_read(ptr, dest, size, atomic)
#define util_write(ptr, src, size, atomic)

#define util_offset(ptr, offset) ((void*)(((char*)(ptr)) + (offset)))
#define utilOffset(ptr, offset) ((void*)(((char*)(ptr)) + (offset)))
#define utilGetOffset(a, b) (((char*)(b)) - ((char*)(a)))
#define TR_OFFSET(ptr, offset) ((void*)(((char*)ptr) + offset))

typedef struct {
  pthread_mutex_t mutex;
  pthread_cond_t cond;
} Signal;

#define utilSignalWaitUntil(signal, cond) \
  do { \
    if (cond) break; \
    util_signal_wait(signal); \
  } while (1)

static inline void util_signal_wait(Signal *signal) {
  pthread_mutex_lock(&signal->mutex);
  pthread_cond_wait(&signal->cond, &signal->mutex);
  pthread_mutex_unlock(&signal->mutex);
} 

static inline void util_signal_signal(Signal *signal) {
  pthread_cond_broadcast(&signal->cond);
}

static inline void util_signal_init(Signal *s) {
  pthread_mutex_init(&s->mutex, 0);
  pthread_cond_init(&s->cond, 0);
}

static inline void util_signal_destroy(Signal *s) {
  pthread_cond_destroy(&s->cond);
  pthread_mutex_destroy(&s->mutex);
}

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

#define dbDebug(level, ...) \
  do { \
    const char *message_type[] = { \
      "Error", "Oops", "Warning", "Info" \
    }; \
    fprintf(stderr, "%s at '%s' line %i: ", \
      message_type[DB_DBG_LEVEL_##level], __FILE__, __LINE__); \
    fprintf(stderr, "" __VA_ARGS__); \
    fprintf(stderr, "\n"); \
  } while (0)


#endif


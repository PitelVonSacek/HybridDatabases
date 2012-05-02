#ifndef __UTILS_H__
#define __UTILS_H__

#include <semaphore.h>
#include <pthread.h>

#include "basic_utils.h"
#include "lock.h"


#include "../database.types/enums.h"
static inline unsigned hash_ptr(const void *ptr) {
  return (((size_t)ptr) * 780119) % DB_LOCKS; // find betterr prime :-)
}

#include "../database.types/handler.h"
#include "../database.types/database.h"
static inline bool util_lock(Handler *H, Lock *lock) {
  switch (l_lock(lock, H, H->start_time)) {
    case 0: return false; 
    case 1:
#if INPLACE_NODE_LOCKS || INPLACE_INDEX_LOCKS
      stack_push(H->acquired_locks, lock);
#else
      istack_push(H->acquired_locks, lock - H->database->locks);
#endif
  }
  return true;
}


#define dbDebug(...) utilDebug(__VA_ARGS__)

#endif


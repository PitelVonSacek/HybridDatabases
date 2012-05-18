#ifndef __ATOMIC_H__
#define __ATOMIC_H__

/**
 * @file
 * @brief Definice atomických operací.
 *
 * Kousky vyžadující assembler jsou v souboru atomic_amd64.h
 *
 * Všechny makra fungují pro libovolné typy o velikosti 8, 16, 32 a 64 bitů.
 */

#include "atomic_amd64.h"

/// atomicky provede <em>*ptr</em>
#define atomic_read(ptr) (*((volatile typeof(ptr))(ptr)))
/// atomicky provede <em>*ptr = value</em>
#define atomic_write(ptr, value) (atomic_read(ptr) = (value))


/// atomicky provede <em>(*ptr == old) ? (*ptr = new_, 1) : 0</em>
#define atomic_cmpswp(ptr, old, new_) \
  _atomic_cmpxchg(ptr, old, new_, sizeof(*(ptr)), "lock; ")

/// atomicky provede <em>({ typeof(*ptr) _tmp = *ptr; *ptr = new_; _tmp; })</em>
#define atomic_swp(ptr, new_) \
  _atomic_xchg(new_, ptr, sizeof(*(ptr)))


/// atomicky provede <em>(*ptr += x)</em>
#define atomic_add_and_fetch(ptr, x) \
  ({ \
    typeof(*(ptr)) __x = (x); \
    __x + _atomic_xadd(ptr, __x, "lock; "); \
  })

/// atomicky provede <em>({ typeof(*ptr) _tmp = *ptr; *ptr += x; _tmp; })</em>
#define atomic_fetch_and_add(ptr, x) \
  _atomic_xadd(ptr, x, "lock; ")


/// atomicky provede <em>do { *ptr += x; } while(0)</em>
#define atomic_add(ptr, x) ({ _atomic_xadd(ptr, x, "lock; "); (void)0; })

/// atomicky provede <em>do { *ptr += 1; } while(0)</em>
#define atomic_inc(ptr) atomic_add(ptr, 1)
/// atomicky provede <em>do { *ptr -= 1; } while(0)</em>
#define atomic_dec(ptr) atomic_add(ptr, -1)

#endif


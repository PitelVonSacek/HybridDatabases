#ifndef __LOCK_H__
#define __LOCK_H__

/**
 * @file
 * @brief Implementace neblokujících verzovaných zámků.
 */

#include <stdint.h>

#include "atomic.h"

// Debugování zámků defaultně vypnuto.
#ifndef LOCK_DEBUG
#define LOCK_DEBUG 0
#endif

#if LOCK_DEBUG
#include <stdio.h>
#define lockDebug(...) fprintf(stderr, __VA_ARGS__)
#else
#define lockDebug(...) (void)0
#endif


/**
 * @brief Neblokující verzovaný zámek.
 *
 * Je-li zámek odemčený, obsahuje lichou hodnotu, která označuje
 * jeho současnou verzi. Je-li zamčený obsahuje ukazatel na toho,
 * kdo ho zamkl.
 *
 * Vyžaduje, aby ukazatele, které jsou předávány jako vlastník zámku
 * byly zarovnané alespoň na 2 bajty.
 */
typedef struct { uint64_t value; } Lock;


/// Inicializuje zámek.
static inline void l_init(Lock* l) { l->value = 1; }


/**
 * Zamkne zámek a vrátí jeho původní verzi.
 * Při selhání vrací 0.
 *
 * Pouze pro použití při vypisování databáze. (Vlákno vypisující databázi
 * potřebuje uzel zamknout, aby ho nikdo nemodifikoval, ale jelikož
 * ho samo nezmění může mu při odemknutí nechat starou verzi.)
 */
static inline uint64_t l_lock_(Lock* l, void* ptr, uint64_t version) {
  uint64_t v;

  while (1) {
    v = atomic_read(&l->value);
    if ((v & 1) ? (v > version) : (v != (uint64_t)ptr)) {
      lockDebug("Lock failed (read: %lx, ptr: %lx, version: %lx)\n",
                (long)v, (long)ptr, (long)version);
      return 0;
    }
    if (atomic_cmpswp(&l->value, v, (uint64_t)ptr)) return v;
  }
}


/**
 * Uzamkne zámek.
 *
 * Návratové hodnoty:
 *   0 ... selhání, zámek vlastní někdo jiný nebo má verzi vyšší než naše.
 *   1 ... úspěch, zamkli jsme zámek
 *   2 ... úspěch, zámek jsem již vlastnili
 */
static inline int l_lock  (Lock* l, void* ptr, uint64_t version) {
  uint64_t v;

  while (1) {
    v = atomic_read(&l->value);
    if ((v & 1) ? (v > version) : (v != (uint64_t)ptr)) {
      lockDebug("Lock failed (read: %lx, ptr: %lx, version: %lx)\n",
                (long)v, (long)ptr, (long)version);
      return 0;
    }
    if (atomic_cmpswp(&l->value, v, (uint64_t)ptr)) return 1 + (v == (uint64_t)ptr);
  }
}

/// Odemkne zámek.
static inline int l_unlock(Lock* l, void* ptr, uint64_t version) {
  return atomic_cmpswp(&l->value, (uint64_t)ptr, version);
}


/**
 * Zkontroluje, zda z dat střežených zámkem můžeme číst.
 *
 * Tedy je-li zámek odemčen a má verzi nejvýše rovnou naší
 * nebo ho vlastníme.
 */
static inline bool l_check (const Lock* l, void* ptr, uint64_t version) {
  uint64_t v = atomic_read(&l->value);
  bool ret =  (v & 1) ? (v <= version) : (v == (uint64_t)ptr);
  if (!ret) lockDebug("Lock failed (read: %lx, ptr: %lx, version: %lx)\n",
                (long)v, (long)ptr, (long)version);
  return ret;
}

#undef lockDebug

#endif


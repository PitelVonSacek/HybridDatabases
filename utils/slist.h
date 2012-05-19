#ifndef __SINGLE_LINKED_LIST_H__
#define __SINGLE_LINKED_LIST_H__

/**
 * @file
 * @brief Implementace jednosměrného spojového seznamu.
 *
 * Jednoduchý spoják s atomickými operacemi.
 */

#include "basic_utils.h"
#include "atomic.h"

/// Jednosměrný spoják.
struct SList {
  struct SList *next;
};

/// Inicializátor.
#define SListInit { .next = 0 }

/// @internal
#define slistGetFirst(var, list) \
  do { \
    var = atomic_swp(&(list)->next, (void*)-1); \
    if (unlikely(var == (void*)-1)) { \
      int __try = 0; \
      do { \
        spin_or_yield(__try++); \
        var = atomic_swp(&(list)->next, (void*)-1); \
      } while (unlikely(var == (void*)-1)); \
    } \
  } while (0)


/// Vloží prvek na začátek seznamu.
static inline void slist_push(struct SList *head, struct SList *item) {
  item->next = head->next;
  head->next = item;
}

/// Vyjme prvek ze začátku seznamu. Je-li prázdný vrátí NULL.
static inline struct SList *slist_pop(struct SList *head) {
  struct SList *item = head->next;
  head->next = (item ? item->next : 0);
  return item;
}


/// Prohodí dva seznamy.
static inline void slist_swap(struct SList *head_a, struct SList *head_b) {
  struct SList tmp = *head_a;
  *head_a = *head_b;
  *head_b = tmp;
}


/// Vrátí @c true, je-li seznam prázdný.
static inline bool slist_empty(struct SList *head) {
  return atomic_read(&head->next) == 0;
}


/// Atomicky vyjme prvek ze začátku seznamu. Je-li prázdný vrátí NULL.
static inline struct SList *slist_atomic_pop(struct SList *head) {
  struct SList *item;

  slistGetFirst(item, head);

  atomic_swp(&head->next, (item ? item->next : 0));
  return item;
}

/// Atomicky vloží prvek na začátek seznamu.
static inline void slist_atomic_push(struct SList *head, struct SList *item) {
  slistGetFirst(item->next, head);
  atomic_swp(&head->next, item);
}


/// Prohodí dva seznamy. Atomicky v prvním argumentu.
static inline void slist_atomic_swap(struct SList *head_a, struct SList *head_b) {
  struct SList tmp;

  slistGetFirst(tmp.next, head_a);

  atomic_swp(&head_a->next, head_b->next);
  *head_b = tmp;
}

#endif


#ifndef __SINGLE_LIST_H__
#define __SINGLE_LIST_H__

#include "atomic.h"
#include "type_magic.h"

struct SList {
  struct SList *next;
  DummyAncestor __ancestor;
};

#define SList(content) SList_(content, __COUNTER__)
#define SList_(c, $) SList__(c, $)
#define SList__(c, $) \
  union SListHead_##$ { \
    struct SList_##$ { \
      union { \
        struct SList_##$ *next; \
        struct SList __ancestor; \
      }; \
      c; \
    } *next; \
    struct SList __ancestor; \
  }

#define SListItem(var) typeof(((typeof(var))0)->next[0])

#define SListInit { .next = 0 }

#define slistGetFirst(var, list) \
  do { \
    var = atomic_swp(&(list)->next, (void*)-1); \
  } while (var == (void*)-1)

static inline void slist_push(struct SList *head, struct SList *item) {
  item->next = head->next;
  head->next = item;
}

static inline struct SList *slist_pop(struct SList *head) {
  struct SList *item = head->next;
  head->next = (item ? item->next : 0);
  return item;
}

static inline void slist_swap(struct SList *head_a, struct SList *head_b) {
  struct SList tmp = *head_a;
  *head_a = *head_b;
  *head_b = tmp;
}

static inline bool slist_empty(struct SList *head) {
  return atomic_read(&head->next) == 0;
}

static inline struct SList *slist_atomic_pop(struct SList *head) {
  struct SList *item;

  slistGetFirst(item, head);

  atomic_swp(&head->next, (item ? item->next : 0));
  return item;
}

static inline void slist_atomic_push(struct SList *head, struct SList *item) {
  slistGetFirst(item->next, head);
  atomic_swp(&head->next, item);
}

// atomic in FIRST argumet
static inline void slist_atomic_swap(struct SList *head_a, struct SList *head_b) {
  struct SList tmp;

  slistGetFirst(tmp.next, head_a);

  atomic_swp(&head_a->next, head_b->next);
  *head_b = tmp;
}

/****************
 *  Type magic  *
 ****************/

#define slist_empty(h) slist_empty(typeUncast(h))

#define slist_push(h, i) slist_push(typeUncast(h), typeUncast(i))
#define slist_pop(h) ((SListItem(h))slist_pop(typeUncast(h)))
#define slist_swap(h, i) slist_swap(typeUncast(h), typeUncast(i))

#define slist_atomic_push(h, i) slist_atomic_push(typeUncast(h), typeUncast(i))
#define slist_atomic_pop(h) ((SListItem(h)*)slist_atomic_pop(typeUncast(h)))
#define slist_atomic_swap(h, i) slist_atomic_swap(typeUncast(h), typeUncast(i))

#endif


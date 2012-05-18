#ifndef __LIST_H__
#define __LIST_H__

/**
 * @file
 * @brief Implementace dvojitého spojového seznamu.
 *
 * Seznam je vždy kruhový s hlavou.
 *
 * Inspirováno implementací použitou v Linuxovém jádře.
 */

#include "basic_utils.h"

/// Dvojitý kruhový spojový seznam s hlavou.
struct List {
  struct List *next, *prev;
};

/// vrátí adresu struktury podle adresy jejího členu.
#define listGetContainer(ContainerType, ListMember, list) \
  utilContainerOf(list, ContainerType, ListMember)

/// Inicializátor seznamu.
#define ListInit(name) { &name, &name }

/// Funkce pro dynamickou inicializaci seznamu.
/// Je nutné volat pouze na hlavu, ne na prvky.
static inline struct List *list_init_head(struct List* head) {
  head->next = head->prev = head;
  return head;
}

/// Vrátí @c true, je-li seznam prázdný.
static inline bool list_empty(struct List *head) {
  return head->next == head;
}

/// Prohodí dva seznamy.
static inline void list_swap(struct List *a, struct List *b) {
  struct List tmp = *a;
  *a = *b;
  *b = tmp;

  // fix ptrs
  if (a->next != b) {
    a->next->prev = a;
    a->prev->next = a;
  } else {
    a->next = a->prev = a;
  }

  if (b->next != a) {
    b->next->prev = b;
    b->prev->next = b;
  } else {
    b->next = b->prev = b;
  }
}

/// Spojí dva prvky seznamu za sebe.
static inline void list_join_items(struct List *a, struct List *b) {
  a->next = b;
  b->prev = a;
}

/// Přidá obsah seznamu @c b do seznamu @c a. Seznam @c b zůstane prázdný.
static inline void list_join_lists(struct List *head_a, struct List *head_b) {
  if (list_empty(head_b)) return;

  list_join_items(head_a->prev, head_b->next);
  list_join_items(head_b->prev, head_a);

  list_init_head(head_b);
}

/// Přidá prvek za hlavu seznamu (tedy na začátek).
static inline void list_add_begin(struct List *head, struct List *item) {
  list_join_items(item, head->next);
  list_join_items(head, item);
}

/// Přidá prvek před hlavu seznamu (tedy na konec).
static inline void list_add_end(struct List *head, struct List *item) {
  list_join_items(head->prev, item);
  list_join_items(item, head);
}

/// Vyjme prvek ze seznamu.
static inline struct List *list_remove(struct List *item) {
  list_join_items(item->prev, item->next);
  return item;
}

/// Cyklus přes všechny prvky seznamu.
#define list_for_each(var, list) \
  for (struct List *var = list->next; var != list; var = var->next)

/// Cyklus přes všechny prvky seznamu pozpátku.
#define list_for_each_reverse(var, list) \
  for (struct List *var = list->prev; var != list; var = var->prev)


/// Cyklus přes všechny prvky seznamu, jsou-li prvky seznamu
/// typu @c Type a struktura @c List je v nich jako člen jménem @c Memeber.
#define list_for_each_item(var, list, Type, Member) \
  for (Type *var = listGetContainer(Type, Member, (list)->next); \
       &var->Member != (list); \
       var = listGetContainer(Type, Member, var->Member.next))

#endif


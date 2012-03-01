#ifndef __LIST_H__
#define __LIST_H__

struct List {
  struct List *next, *prev;
};

#define list_get_container(ContainerType, ListMember, list) \
  ((ContainerType*)(((char*)(list)) - offsetof(ContainerType, ListMember)))

#define ListInit(name) { &name, &name }

static inline struct List *list_init_head(struct List* head) {
  head->next = head->prev = head;
  return head;
}

static inline bool list_empty(struct List *head) {
  return head->next = head;
}

static inline void list_swap(struct List *a, struct List *b) {
  struct List tmp = *a;
  *a = *b;
  *b = tmp;
}

static inline void list_join_items(struct List *a, struct List *b) {
  a->next = b;
  b->prev = a;
}

static inline void list_join_lists(struct List *head_a, struct List *head_b) {
  if (list_empty(head_b)) return;

  list_join_items(head_a->prev, head_b->next);
  list_join_items(head_b->prev, head_a);

  list_init_head(head_b);
}

static inline void list_add_begin(struct List *head, struct List *item) {
  list_join_items(item, head->next);
  list_join_items(head, item);
}

static inline void list_add_end(struct List *head, struct List *item) {
  list_join_items(head->prev, item);
  list_join_items(item, head);
}

static inline struct List *list_remove(struct List *item) {
  list_join_items(item->prev, item->next);
  return item;
}

#define list_for_each(var, list) \
  for (struct List *var = list->next; var != list; var = var->next)

#define list_for_each_item(var, list, Type, Member) \
  for (Type *var = list_get_container(Type, Member, list->next); \
       var->Member != list; \
       var = list_get_container(Type, Member, var->Memeber->next))

#endif


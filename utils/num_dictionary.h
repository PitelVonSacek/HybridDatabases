#ifndef __NUM_DICTIONARY_H__
#define __NUM_DICTIONARY_H__

#include "basic_utils.h"
#include "slist.h"
#include "../allocators/page_allocator.h"

typedef struct {} IsNumDictionaryDummy;
typedef uint64_t GenericKey;

typedef struct {
  IsNumDictionaryDummy __dummy;
  size_t size, capacity, capacity_index /* index in capacity table */;

  struct NumDictionaryNode {
    union {
      struct NumDictionaryNode *next;
      struct SList slist;
    };
    GenericKey generic_key;
  } **buckets;

  struct SList pages;
  struct SList free_nodes;
} NumDictionary;

#define NumDictionary(Key, Value) NumDictionary_(Key, Value, __COUNTER__)
#define NumDictionary_(K, V, n) NumDictionary__(K, V, n)
#define NumDictionary__(Key, Value, n) \
  struct { \
    IsNumDictionaryDummy __dummy; \
    size_t size, capacity, capacity_index; \
    \
    struct _dictionary_node__##n { \
      union { \
        struct _dictionary_node__##n *next; \
        struct SList slist; \
      }; \
      union { \
        Key key; \
        GenericKey generic_key; \
      }; \
      Value value; \
    } **buckets; \
    \
    struct SList pages; \
    struct SList free_nodes; \
    \
    struct { \
      char key_size_must_be_less_or_equal_to_size_t \
        [(sizeof(Key) <= sizeof(GenericKey)) ? 1 : -1]; \
    } __static_assert[0]; \
  }

#define NumDictionaryInit { {}, 0, 0, 0, 0, { 0 }, { 0 } }

#define _ndict_uncast(dict) \
  static_if( \
    types_equal(typeof((dict)->__dummy), IsNumDictionaryDummy), \
    utilCast(NumDictionary, dict), \
    (void)0 \
  )

#define _ndict_key_uncast(dict, key_) \
  ({ \
    union { \
      typeof((dict)->buckets[0]->key) key; \
      GenericKey generic_key; \
    } __tmp; \
    __tmp.generic_key = 0; \
    __tmp.key = (key_); \
    __tmp.generic_key; \
  })

#define ndict_init(dict) ndict_generic_init(_ndict_uncast(dict))
#define ndict_destroy(dict) ndict_generic_destroy(_ndict_uncast(dict))


#define ndict_contains(dict, key) (!!ndict_get_node(dict, key))

#define ndict_at(dict, key)\
  ({ \
    const typeof((dict)->buckets[0]) __node = \
      ndict_get_node(dict, key); \
    if (!__node) assert(0); \
    __node->value; \
  })

#define ndict_get_node(dict, key) \
  ((typeof((dict)->buckets[0]))ndict_generic_get_node(_ndict_uncast(dict), \
    _ndict_key_uncast(dict, key)))

#define ndict_insert(dict, key_, value_) \
  ({ \
    typeof(&*(dict)) __dict = (dict); \
    typeof((dict)->buckets[0]) __node = slist_pop(&__dict->free_nodes) ?: \
      _ndict_alloc_nodes(_ndict_uncast(__dict), sizeof(*__node)); \
    __node->generic_key = 0; \
    __node->key = (key_); \
    __node->value = (value_); \
    ndict_generic_insert(_ndict_uncast(__dict), \
                         utilCast(struct NumDictionaryNode, __node)); \
  })

#define ndict_remove(dict, key) \
  ndict_generic_remove(_ndict_uncast(dict), \
                       _ndict_key_uncast(dict, key)) \


#define ndictFor(var, dict) ndictFor_(dict, var, __COUNTER__)
#define ndictFor_(...) ndictFor__(__VA_ARGS__)
#define ndictFor__(dict, var, $) do { \
  __label__ _ndict_break_label, _ndict_continue_label; \
  typeof(&*(dict)) _dict##$ = (dict); \
   \
  for (size_t _dict_it = 0; _dict_it < _dict##$->capacity; _dict_it++) \
    for (typeof(*_dict##$->buckets) var = _dict##$->buckets[_dict_it]; \
        var; var = var->next) { \

#define ndictForEnd \
      _ndict_continue_label: __attribute__((unused)); \
    } \
  _ndict_break_label: __attribute__((unused)); \
  } while (0)

#define ndictContinue goto _ndict_continue_label
#define ndictBreak goto _ndict_break_label;

#define Inline static inline

Inline void ndict_generic_init(NumDictionary *dict);
Inline void ndict_generic_destroy(NumDictionary *dict);

Inline struct NumDictionaryNode *ndict_generic_get_node(NumDictionary *dict, 
                                                        GenericKey key);

// fails if key already exists
Inline bool ndict_generic_insert(NumDictionary *dict, struct NumDictionaryNode*);
// fails if key is not in dictionary
Inline bool ndict_generic_remove(NumDictionary *dict, GenericKey key);


// Implementation

static const size_t _ndict_capacities[] = {
  0, 
  29,
  53,
  89,
  137,
  211,
  331,
  499,
  727,
  1061,
  1543,
  2239,
  3229,
  4657,
  6761,
  9767,
  14083,
  20287,
  29243,
  42131,
  60703,
  87433,
  125927,
  181361,
  261223,
  376171,
  541699,
  780119,
  1123391,
  1617739,
  2329559,
  3354581,
  4830619,
  6956203,
  10017011,
  14424539,
  20771371,
  29910821,
  43071601,
  51685939,
  74427803,
  107176057,
  154333591,
  222240413,
  320026249,
  460837813,
  663606499,
  955593439,
  1376054569,
  1981518631,
  2853386881,
  4108877153,
  4294967291,
  // 2^32
  8294967337,
  12294967297,
  16294967303,
  24294967309,
  32294967307,
  48294967321,
  64294967299,
  0
};

#ifndef _num_dict_debug
#define _num_dict_debug(...)
#endif

Inline size_t _ndict_hash(size_t capacity, GenericKey key) {
  return (key * 2971215073) % capacity;
}

static void _ndict_resize(NumDictionary *dict, int cap_index_diff);

Inline void ndict_generic_init(NumDictionary *dict) {
  *dict = (NumDictionary){
    {}, 0, 0, 0, 0, {0}, {0}
  };
}

Inline void ndict_generic_destroy(NumDictionary *dict) {
  struct SList *page;
  while (page = slist_pop(&dict->pages))
    page_free(page);

  free(dict->buckets);
  
  *dict = (NumDictionary){
    {}, 0, 0, 0, 0, {0}, {0}
  };
}

Inline struct NumDictionaryNode *ndict_generic_get_node(NumDictionary *dict, 
                                                        GenericKey key) {
  if (!dict->capacity) return 0;

  struct NumDictionaryNode *node = dict->buckets[_ndict_hash(dict->capacity, key)];

  while (node) {
    if (node->generic_key == key) return node;
    node = node->next;
  }

  return 0;
}

Inline bool ndict_generic_insert(NumDictionary *dict, struct NumDictionaryNode *node) {
  if (ndict_generic_get_node(dict, node->generic_key)) {
    _num_dict_debug("Insert: failed, %li already in dict", node->generic_key);
    slist_push(&dict->free_nodes, &node->slist);
    return false;
  }

  if (dict->capacity < dict->size * 2 + 1) _ndict_resize(dict, 1);
 
  struct NumDictionaryNode **bucket = dict->buckets + 
    _ndict_hash(dict->capacity, node->generic_key);

  node->next = *bucket;
  *bucket = node;

  dict->size++;

  return true;
}

Inline bool ndict_generic_remove(NumDictionary *dict, GenericKey key) {
  if (!dict->capacity) {
    _num_dict_debug("Remove: returning cause of 0 capacity");
    return false;
  }

  struct NumDictionaryNode **node_ptr;
  node_ptr = dict->buckets + _ndict_hash(dict->capacity, key);

  while (*node_ptr) {
    if (node_ptr[0]->generic_key == key) {
      struct NumDictionaryNode *tmp = node_ptr[0];
      node_ptr[0] = node_ptr[0]->next;
      slist_push(&dict->free_nodes, &tmp->slist);
      goto end;
    }
    node_ptr = &node_ptr[0]->next;
  }

  _num_dict_debug("Remove: %li not found", key);
  return false;
  end:

  dict->size--;
  if (dict->capacity > dict->size * 4) _ndict_resize(dict, -1);

  return true;
}

#undef Inline

static void _ndict_resize(NumDictionary *dict, int cap_index_diff) {
  size_t new_cap = _ndict_capacities[dict->capacity_index + cap_index_diff];

  _num_dict_debug("Resize: diff %i, old cap %i\n", cap_index_diff, (int)dict->capacity);

  if (!new_cap) {
    if (cap_index_diff == -1) return;
    // FIXME dirty
    assert(0);
  }

  struct NumDictionaryNode **buckets = xmalloc(sizeof(*buckets) * new_cap);
  memset(buckets, 0, sizeof(*buckets) * new_cap);

  for (size_t bucket = 0; bucket < dict->capacity; bucket++) {
    struct NumDictionaryNode *node, *next = dict->buckets[bucket];
    while (node = next) {
      next = node->next;

      struct NumDictionaryNode **new_bucket = buckets + 
          _ndict_hash(new_cap, node->generic_key);

      node->next = *new_bucket;
      *new_bucket = node;
    }
  }

  free(dict->buckets);
  dict->buckets = buckets;
  dict->capacity_index += cap_index_diff;
  dict->capacity = new_cap;
}

static __attribute__((unused))
void *_ndict_alloc_nodes(NumDictionary *dict, size_t node_size) {
  struct {
    union {
      struct SList slist;
      long long _padding[2];
    };
    char data[0];
  } *page = page_alloc();

  slist_push(&dict->pages, &page->slist);

  const size_t items = (PAGE_ALLOCATOR_PAGE_SIZE - sizeof(*page)) / node_size;

  for (int i = 0; i < items; i++)
    slist_push(&dict->free_nodes, (struct SList*)(page->data + node_size * i));

  return slist_pop(&dict->free_nodes);
}

#undef _num_dict_debug
#endif


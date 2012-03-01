#ifndef __NUM_DICTIONARY_H__
#define __NUM_DICTIONARY_H__

#include "static_if.h"
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>

typedef struct {} IsNumDictionaryDummy;

typedef struct {
  IsNumDictionaryDummy __dummy;
  size_t size, capacity, capacity_index /* index in capacity table */;

  struct NumDictionaryNode {
    struct NumDictionaryNode *next;
    size_t generic_key;
  } **buckets;
} NumDictionary;

#define NumDictionary(Key, Value) NumDictionary_(Key, Value, __COUNTER__)
#define NumDictionary_(K, V, n) NumDictionary__(K, V, n)
#define NumDictionary__(Key, Value, n) \
  struct { \
    IsNumDictionaryDummy __dummy; \
    size_t size, capacity, capacity_index; \
    \
    struct _dictionary_node__##n { \
      struct _dictionary_node__##n *next; \
      union { \
        Key key; \
        size_t generic_key; \
      }; \
      Value value; \
    } **buckets; \
    \
    struct { \
      char key_size_must_be_less_or_equal_to_size_t \
        [(sizeof(Key) <= sizeof(size_t)) ? 1 : -1]; \
    } __static_assert[0]; \
  }

#define NumDictionaryInit { {}, 0, 0, 0, 0 }

#define _ndict_uncast(dict) \
  static_if( \
    types_equal(typeof((dict)->__dummy), IsNumDictionaryDummy), \
    (NumDictionary*)(dict), \
    (void)0 \
  )

#define _ndict_key_uncast(dict, key_) \
  ({ \
    union { \
      typeof((dict)->buckets[0]->key) key; \
      size_t generic_key; \
    } __tmp; \
    __tmp.generic_key = 0; \
    __tmp.key = (key_); \
    __tmp.generic_key; \
  })

#define ndict_init(dict) ndict_generic_init(_ndict_uncast(dict))
#define ndict_destroy(dict) ndict_generic_destroy(_ndict_uncast(dict))


#define ndict_contains(dict, key) (!!ndict_get_node(dict, key))

#define ndict_get_node(dict, key) \
  ((typeof((dict)->buckets[0]))ndict_generic_get_node(_ndict_uncast(dict), \
    _ndict_key_uncast(dict, key)))

#define ndict_insert(dict, key_, value_) \
  ({ \
    typeof((dict)->buckets[0]) __node = malloc(sizeof(*__node)); \
    __node->generic_key = 0; \
    __node->key = (key_); \
    __node->value = (value_); \
    ndict_generic_insert(_ndict_uncast(dict), (struct NumDictionaryNode*)__node); \
  })

#define ndict_remove(dict, key) \
  ndict_generic_remove(_ndict_uncast(dict), \
                       _ndict_key_uncast(dict, key)); \


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
      _ndict_continue_label: ; \
    } \
  _ndict_break_label: ; \
  } while (0);

#define ndictContinue goto _ndict_continue_label
#define ndictBreak goto _ndict_break_label;

#define Inline static inline

Inline void ndict_generic_init(NumDictionary *dict);
Inline void ndict_generic_destroy(NumDictionary *dict);

Inline struct NumDictionaryNode *ndict_generic_get_node(NumDictionary *dict, size_t key);

// fails if key already exists
Inline bool ndict_generic_insert(NumDictionary *dict, struct NumDictionaryNode*);
// fails if key is not in dictionary
Inline bool ndict_generic_remove(NumDictionary *dict, size_t key);


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
  // not primes anymore
  8294967291,
  12294967291,
  16294967291,
  24294967291,
  32294967291,
  48294967291,
  64294967291,
  0
};

Inline size_t _ndict_hash(size_t capacity, size_t key) {
  return (key * 2971215073) % capacity;
}

static void _ndict_resize(NumDictionary *dict, int cap_index_diff);

Inline void ndict_generic_init(NumDictionary *dict) {
  *dict = (NumDictionary){
    {}, 0, 0, 0, 0
  };
}

Inline void ndict_generic_destroy(NumDictionary *dict) {
  for (size_t i = 0; i < dict->capacity; i++) {
    struct NumDictionaryNode *node, *next;
    next = dict->buckets[i];
    while (node = next) {
      next = node->next;
      free(node); 
    }
  } 

  free(dict->buckets);
  
  *dict = (NumDictionary){
    {}, 0, 0, 0, 0
  };
}

Inline struct NumDictionaryNode *ndict_generic_get_node(NumDictionary *dict, size_t key) {
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
    free(node);
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

Inline bool ndict_generic_remove(NumDictionary *dict, size_t key) {
  if (!dict->capacity) return false;

  struct NumDictionaryNode **node_ptr;
  node_ptr = dict->buckets + _ndict_hash(dict->capacity, key);

  while (*node_ptr) {
    if (node_ptr[0]->generic_key == key) {
      struct NumDictionaryNode *tmp = node_ptr[0];
      node_ptr[0] = node_ptr[0]->next;
      free(tmp);
      goto end;
    }
    node_ptr = &node_ptr[0]->next;
  }

  return false;
  end:

  dict->size--;
  if (dict->capacity > dict->size * 4) _ndict_resize(dict, -1);

  return true;
}

#undef Inline

static void _ndict_resize(NumDictionary *dict, int cap_index_diff) {
  size_t new_cap = _ndict_capacities[dict->capacity_index + cap_index_diff];

  if (!new_cap) {
    if (cap_index_diff == -1) return;
    // FIXME dirty
    assert(0);
  }

  struct NumDictionaryNode **buckets = malloc(sizeof(*buckets) * new_cap);
  memset(buckets, 0, sizeof(*buckets) * new_cap);

  for (size_t bucket = 0; bucket < dict->capacity; bucket++) {
    struct NumDictionaryNode *node, *next = dict->buckets[bucket];
    while (node = next) {
      next = node->next;

      struct NumDictionaryNode **new_bucket = buckets + 
          _ndict_hash(dict->capacity, node->generic_key);

      node->next = *new_bucket;
      *new_bucket = node;
    }
  }

  free(dict->buckets);
  dict->buckets = buckets;
  dict->capacity_index += cap_index_diff;
  dict->capacity = new_cap;
}

#endif


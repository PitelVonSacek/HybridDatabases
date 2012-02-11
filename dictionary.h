#ifndef __DICTIONARY_V2_H__
#define __DICTIONARY_V2_H__

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "static_if.h"
#include "exception.h"

EXCEPTION_DECLARE(DictionaryKeyNotFound, NONE);

#define Dictionary_alloc(x) malloc(x)
#define Dictionary_free(x)  free(x)

#define Dictionary(Key, Value, KeyType) Dictionary_(Key, Value, KeyType, __COUNTER__)
#define Dictionary_(K, V, t, n) Dictionary__(K, V, t, n)
#define Dictionary__(Key, Value, key_type_, n) \
  struct { \
    DictionaryKeyType__##key_type_ key_type; \
    size_t size, capacity; \
    \
    struct _dictionary_node__##n { \
      struct _dictionary_node__##n *next; \
      Key key; \
      Value value; \
    } **buckets; \
  }


/**********************
 *  Create & destroy  *
 **********************/

#define DictionaryInit { {}, 0, 0, 0 }

#define dict_alloc() \
  ((void*)dict_init((struct GenericDictionary*)Dictionary_alloc( \
    sizeof(struct GenericDictionary)) \
  ))

#define dict_init(dict) dict_init_(dict, __COUNTER__)
#define dict_init_(...) dict_init__(__VA_ARGS__)
#define dict_init__(dict, $) ({ \
    typeof(&*(dict)) _dict##$ = (dict); \
    _dict##$->size = _dict##$->capacity = 0; \
    _dict##$->buckets = 0; \
    _dict##$; \
  })

#define dict_destroy(dict) dict_destroy_(dict, __COUNTER__)
#define dict_destroy_(...) dict_destroy__(__VA_ARGS__)
#define dict_destroy__(dict, $) ({ \
    typeof(&*(dict)) _dict##$ = (dict); \
    static_if(types_equal(DictionaryKeyType__STRING, typeof(_dict##$->key_type)), \
      ({ DICT_FOR(_dict##$, buck) Dictionary_free(*(void**)&buck->key); DICT_FOR_END }), \
      (void)0); \
    _dict_free_nodes((struct GenericDictionary*)_dict##$); \
    (void)0; \
  })

#define dict_free(dict) dict_free_(dict, __COUNTER__)
#define dict_free_(...) dict_free__(__VA_ARGS__)
#define dict_free__(dict, $) ({ \
    typeof(&*(dict)) _dict##$ = (dict); \
    dict_destroy(_dict##$); \
    Dictionary_free(_dict##$); \
  })


/**********************
 *      Access        *
 **********************/

#define dict_get_node(dict, key) dict_get_node_(dict, key, __COUNTER__)
#define dict_get_node_(...) dict_get_node__(__VA_ARGS__)
#define dict_get_node__(dict, key_, $) ({ \
    typeof(&*(dict)) _dict##$ = (dict); \
    typeof(_dict##$->buckets[0]) _node = 0; \
    if (_dict##$->capacity) { \
      typeof(_dict##$->buckets[0]->key) _key##$ = (key_); \
      _node = *_dict_get_bucket(_dict##$, _key##$); \
      while (_node) { \
        if (_dict_keys_equal(_dict##$, _key##$, _node->key)) break; \
        _node = _node->next; \
      } \
    } \
    _node; \
  })

#define dict_contains(dict, key) (!!(dict_get_node(dict, key)))

#define dict_at(dict, key) \
  ENSURE(dict_get_node(dict, key), DictionaryKeyNotFound, 0)->value

#define dict_insert(dict, key, value) dict_insert_(dict, key, value, __COUNTER__)
#define dict_insert_(...) dict_insert__(__VA_ARGS__)
#define dict_insert__(dict, key_, value_, $) ({ \
    typeof(&*(dict)) _dict##$ = (dict); \
    typeof(_dict##$->buckets[0]->key) _key##$ = (key_); \
    typeof(_dict##$->buckets[0]->value) _value##$ = (value_); \
    typeof(_dict##$->buckets[0]) _node = dict_get_node(_dict##$, _key##$); \
    if (_node) _node->value = (_value##$); \
    else { \
      _dict_expand(_dict##$); \
      typeof(_node) *_bucket = _dict_get_bucket(_dict##$, _key##$); \
      _node = Dictionary_alloc(sizeof(*_node)); \
      _node->next = *_bucket; \
      _node->key = _dict_copy_key(_dict##$, _key##$); \
      _node->value = (_value##$); \
      *_bucket = _node; \
      _dict##$->size++; \
    } \
    _dict##$; \
  })

#define dict_remove(dict, key) dict_remove_(dict, key, __COUNTER__)
#define dict_remove_(...) dict_remove__(__VA_ARGS__)
#define dict_remove__(dict, key_, $) ({ \
    typeof(&*(dict)) _dict##$ = (dict); \
    if (_dict##$->capacity) { \
      typeof(_dict##$->buckets[0]->key) _key##$ = (key_); \
      typeof(_dict##$->buckets) _node_ptr = _dict_get_bucket(_dict##$, _key##$); \
      while (*_node_ptr) { \
        if (_dict_keys_equal(_dict##$, _key##$, _node_ptr[0]->key)) { \
          typeof(*_node_ptr) _tmp = *_node_ptr; \
          *_node_ptr = _tmp->next; \
          _dict_free_key(_dict##$, _tmp->key); \
          Dictionary_free(_tmp); \
          _dict##$->size--; \
          _dict_shrink(_dict##$); \
          break; \
        } \
        _node_ptr = &_node_ptr[0]->next; \
      } \
    } \
    _dict##$; \
  })


#define DICT_FOR(dict, var) DICT_FOR_(dict, var, __COUNTER__)
#define DICT_FOR_(...) DICT_FOR__(__VA_ARGS__)
#define DICT_FOR__(dict, var, $) do { \
  __label__ dict_break_label, dict_continue_label; \
  typeof(&*(dict)) _dict##$ = (dict); \
   \
  for (size_t _dict_it = 0; _dict_it < _dict##$->capacity; _dict_it++) \
    for (typeof(*_dict##$->buckets) var = _dict##$->buckets[_dict_it]; \
        var; var = var->next) { \

#define DICT_FOR_END \
      dict_continue_label: ; \
    } \
  dict_break_label: ; \
  } while (0);


/**********************
 *  Private details   *
 **********************/

#ifndef dict_inline
#define dict_inline static inline
#endif

#include "dictionary_hash_functions.h"

struct GenericDictionary {
  DictionaryKeyType__GENERIC key_type;
  size_t size, capacity;

  struct GenericDictionary_node {
    struct GenericDictionary_node *next;
    unsigned char key[0];
  } **buckets;
};

#define _dict_get_bucket(dict, key) \
  (dict->buckets + (_dict_get_hash(dict, (const void*)&key) % (dict->capacity ?: 1)))

#define _dict_expand(dict) \
  if (dict->capacity < dict->size * 2 + 1) _dict_resize(sizeof(dict->buckets[0]->key), \
                                                    _dict_get_hash_function(dict), \
                                                    (struct GenericDictionary*)dict, \
                                                    (dict->capacity * 83) / 40 + 17)

#define _dict_shrink(dict) \
  if (dict->capacity > dict->size * 8 && dict->capacity > 37) \
    _dict_resize(sizeof(dict->buckets[0]->key), \
                 _dict_get_hash_function(dict), \
                 (struct GenericDictionary*)dict, \
                 (dict->capacity / 2) + 11)

void _dict_resize(size_t key_size,
                  size_t (*hash_fnc)(size_t, const void*),
                  struct GenericDictionary* dict,
                  size_t new_capacity);

void _dict_free_nodes(struct GenericDictionary* dict);

dict_inline char *_dict_strcp(const char* str) {
  size_t l = 0;
  while (str[l++]) ;

  char* ret = Dictionary_alloc(sizeof(char) * l);

  while (l--) ret[l] = str[l];

  return ret;
}

#undef dict_inline

#endif


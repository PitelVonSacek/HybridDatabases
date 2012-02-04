#ifndef __DICTIONARY_H__
#define __DICTIONARY_H__

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/*
 * key_type - type of key, available types:
 *   NUMBER - hash computed from value by mutilipication
 *   POINTER - like NUMBER, but takes care of a few low bites that are zeros
 *   STRING - key should char*
 */

#define Dictionary(Name, Key, Value, key_type_) \
  struct Name##__node { \
    struct Name##__node *next; \
    Key key; \
    Value value; \
  }; \
  typedef struct { \
    DictionaryKeyType__##key_type_ key_type; \
    size_t size, capacity; \
    struct Name##__node **buckets; \
  } Name;

#define DictionaryInit { {}, 0, 0, 0 }


#define _dict_hash_fnc(dict, key)  __builtin_choose_expr( \
  __builtin_types_compatible_p(DictionaryKeyType__NUMBER, typeof(dict->key_type)), \
  _dict_hash_number(key),  __builtin_choose_expr( \
  __builtin_types_compatible_p(DictionaryKeyType__POINTER, typeof(dict->key_type)), \
  _dict_hash_pointer(key),  __builtin_choose_expr( \
  __builtin_types_compatible_p(DictionaryKeyType__STRING, typeof(dict->key_type)), \
  _dict_hash_string(*(char**)&key), 0.0 )))


#define _dict_eq(dict, v1, v2) __builtin_choose_expr( \
  __builtin_types_compatible_p(DictionaryKeyType__STRING, typeof(dict->key_type)), \
  !strcmp((char*)v1, (char*)v2), (v1 == v2))


#define _dict_get_bucket(dict, key_) \
  (dict->buckets + (_dict_hash_fnc(_dict, key_) % (dict->capacity ?: 1)))

#define dict_init(dict) ({ \
    typeof(&*(dict)) _dict = (dict); \
    _dict->size = _dict->capacity = 0; \
    _dict->buckets = 0; \
    _dict; \
  })
#define dict_alloc(dict) \
  ((void*)dict_init((struct GenericDictionary*)malloc(sizeof(struct GenericDictionary))))

#define dict_insert(dict_, key_, value_) ({ \
    typeof(&*(dict_)) _dict = (dict_); \
    if (_dict->size * 2 >= _dict->capacity) _dict_expand(_dict); \
    typeof(_dict->buckets[0]->key) _key = (key_); \
    typeof(_dict->buckets) _bucket = _dict_get_bucket(_dict, _key); \
    typeof(*_bucket) _new_bucket = malloc(sizeof(**_bucket)); \
    _new_bucket->next = *_bucket; \
    if (__builtin_types_compatible_p(DictionaryKeyType__STRING, \
                                     typeof(_dict->key_type))) \
      *(char**)&_new_bucket->key = _dict_str_cp((char*)_key); \
    else _new_bucket->key = _key; \
    _new_bucket->value = (value_); \
    *_bucket = _new_bucket; \
    _dict->size++; \
    _dict; \
  })

#define dict_bucket(dict, key_) ({ \
    typeof(&*(dict)) _dict = (dict); \
    typeof(_dict->buckets[0]->key) _key = (key_); \
    typeof(_dict->buckets[0]) *_bucket_ = _dict_get_bucket(_dict, _key); \
    typeof(_dict->buckets[0]) _bucket = 0; \
    if (_bucket_) { \
      _bucket = *_bucket_; \
      for (; _bucket; _bucket = _bucket->next) \
        if (_dict_eq(_dict, _key, _bucket->key)) break; \
    } \
    _bucket; \
  })


#define dict_contains(dict_, key_) (!!dict_bucket(dict_, key_))

#define dict_at(dict, key_) (dict_bucket(dict, key_)->value)

#define dict_remove(dict, key_) ({ \
    typeof(&*(dict)) _dict = (dict); \
    typeof(_dict->buckets[0]->key) _key = (key_); \
    typeof(_dict->buckets) _bucket_ptr = _dict_get_bucket(_dict, _key); \
    for (; *_bucket_ptr; _bucket_ptr = &_bucket_ptr[0]->next) \
      if (_dict_eq(_dict, _key, _bucket_ptr[0]->key)) { \
        typeof(_dict->buckets[0]) _tmp = *_bucket_ptr; \
        *_bucket_ptr = _bucket_ptr[0]->next; \
        if (__builtin_types_compatible_p(DictionaryKeyType__STRING, \
                                         typeof(_dict->key_type))) \
          free((void*)_tmp->key); \
        free(_tmp); \
        _dict->size--; \
        break; \
      } \
    if (_dict->size * 8 < _dict->capacity && dict->capacity > 1000) \
      _dict_shrink(_dict); \
    (void)0; \
  })

#define dict_size(dict) ((dict)->size)
#define dict_capacity(dict) ((dict)->capacity)

#define dict_destroy(dict) ({ \
    typeof(&*(dict)) _dict = (dict); \
    if (__builtin_types_compatible_p(DictionaryKeyType__STRING, typeof(_dict->key_type))) \
      DICT_FOR(_dict, buck) free((void*)buck->key); DICT_FOR_END \
    _dict_free_buckets((struct GenericDictionary*)_dict); \
    free((_dict)->buckets); \
  })
#define dict_free(dict) ({ \
    typeof(&*(dict)) _dict = (dict); \
    dict_destroy(_dict); \
    free(_dict); \
  })

#define DICT_FOR(dict, var) do { \
  __label__ dict_break_label, dict_continue_label; \
  typeof(&*(dict)) _dict_helper = (dict); { \
  typeof(_dict_helper) _dict = _dict_helper;  \
   \
  for (size_t _dict_it = 0; _dict_it < _dict->capacity; _dict_it++) \
    for (typeof(*_dict->buckets) var = _dict->buckets[_dict_it]; \
        var; var = var->next) { \

#define DICT_FOR_END \
      dict_continue_label: ; \
    } \
  dict_break_label: ; \
  }} while (0);

#define dict_break goto dict_break_label
#define dict_continue goto dict_continue_label


/*******************
 * Private Details *
 *******************/

#define _dict_hash_number(n) (((size_t)n) * 0x01008041) // this sux, needs bigger prime
#define _dict_hash_pointer(p) (((size_t)p) * 0x01008041)
#define _dict_hash_string(s) _dict_hash_fnc_string((char*)s)

static inline size_t _dict_hash_fnc_string(const char* s) {
  size_t r = 1212324345;
  while (*s) r = (r * 0x01008041) ^ *s++;
  return r;
}

static inline char* _dict_str_cp(const char* a) {
  size_t l = strlen(a) + 1;
  char* ret = malloc(l);
  memcpy(ret, a, l);
  return ret;
}

typedef struct {} DictionaryKeyType__NUMBER;
typedef struct {} DictionaryKeyType__POINTER;
typedef struct {} DictionaryKeyType__STRING;

struct GenericDictionary__node {
  struct GenericDictionary__node *next;
};
struct GenericDictionary {
  size_t size, capacity;
  struct GenericDictionary__node **buckets;
};


#define _dict_expand(dict) _dict_resize_(dict, (dict->capacity * 83) / 40 + 17)
#define _dict_shrink(dict) _dict_resize_(dict, (dict->capacity / 2) + 3)

#define _dict_resize_(dict, cap) _dict_resize( \
    sizeof(**dict->buckets), \
    ({ size_t _hash(typeof(dict->buckets[0]) node) { \
      return _dict_hash_fnc(dict, node->key); \
      } \
     (size_t(*)(struct GenericDictionary__node*))_hash; }), \
    (struct GenericDictionary*)dict, \
    cap)

static void _dict_resize(size_t node_size,
                         size_t (*hash_fnc)(struct GenericDictionary__node*),
                         struct GenericDictionary* dict,
                         size_t new_capacity) {

  struct GenericDictionary__node** buckets = malloc(sizeof(*buckets) * new_capacity);

  memset(buckets, 0, sizeof(*buckets) * new_capacity);

  for (size_t it = 0; it < dict->capacity; it++) {
    typeof(*buckets) next_bucket;
    for (typeof(*buckets) bucket = dict->buckets[it]; bucket; bucket = next_bucket) {
      next_bucket = bucket->next;
      size_t buck_nr = hash_fnc(bucket) % new_capacity;

      bucket->next = buckets[buck_nr];
      buckets[buck_nr] = bucket;
    }
  }

  free(dict->buckets);
  dict->buckets = buckets;
  dict->capacity = new_capacity;
}

static void _dict_free_buckets(struct GenericDictionary* dict) {
  for (size_t i = 0; i < dict->capacity; i++) {
    typeof(*dict->buckets) next_bucket;
    for (typeof(*dict->buckets) bucket = dict->buckets[i]; bucket; bucket = next_bucket) {
      next_bucket = bucket->next;
      free(bucket);
    }
  }
}

#endif


#include "dictionary.h"

EXCEPTION_DEFINE(DictionaryKeyNotFound, NONE);

void _dict_resize(size_t key_size,
                         size_t (*hash_fnc)(size_t, const void*),
                         struct GenericDictionary* dict,
                         size_t new_capacity) {

  struct GenericDictionary_node** buckets;
  buckets = Dictionary_alloc(sizeof(*buckets) * new_capacity);

  memset(buckets, 0, sizeof(*buckets) * new_capacity);

  for (size_t it = 0; it < dict->capacity; it++) {
    typeof(*buckets) next_bucket;
    for (typeof(*buckets) bucket = dict->buckets[it]; bucket; bucket = next_bucket) {
      next_bucket = bucket->next;
      size_t buck_nr = hash_fnc(key_size, bucket->key) % new_capacity;
      bucket->next = buckets[buck_nr];
      buckets[buck_nr] = bucket;
    }
  }

  Dictionary_free(dict->buckets);
  dict->buckets = buckets;
  dict->capacity = new_capacity;
}

void _dict_free_nodes(struct GenericDictionary* dict) {
  for (size_t i = 0; i < dict->capacity; i++) {
    typeof(*dict->buckets) next_bucket;
    for (typeof(*dict->buckets) bucket = dict->buckets[i]; bucket; bucket = next_bucket) {
      next_bucket = bucket->next;
      Dictionary_free(bucket);
    }
  }

  Dictionary_free(dict->buckets);
}




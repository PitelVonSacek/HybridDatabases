#include "dictionary.h"
#include <stdio.h>

Dictionary(Dict, const char*, int, STRING);
Dictionary(Dict2, int, int, NUMBER);

void main() {
  Dict dit[1] = { DictionaryInit };

  const char* stringy[] = { "hello", " ", "world", "! ", 0 };
  for (int i = 0; stringy[i]; i++) {
    typeof(&*dit) dict_ = dit;
    typeof(stringy[i]) key_ = stringy[i];
    typeof(i) value_ = i;
    
    ({ 
    typeof(&*(dict_)) _dict = (dict_); 
    if (_dict->size * 2 >= _dict->capacity) _dict_expand(_dict); 
    typeof(_dict->buckets[0]->key) _key = (key_); 
    typeof(_dict->buckets) _bucket = _dict_get_bucket(_dict, _key); 
    typeof(*_bucket) _new_bucket = malloc(sizeof(**_bucket)); 
    _new_bucket->next = *_bucket; 
    if (__builtin_types_compatible_p(DictionaryKeyType__STRING, 
                                     typeof(_dict->key_type))) 
      _new_bucket->key = _dict_str_cp(_key); 
    else _new_bucket->key = _key; 
    _new_bucket->value = (value_); 
    *_bucket = _new_bucket; 
    _dict->size++; 
    _dict; 
    });
  }
  
  for (int i = 0; stringy[i]; i++) dict_insert(dit, stringy[i], i - 1);

  dict_remove(dit, "world");

  printf("%i %i\n", (int)dict_size(dit), (int)dict_capacity(dit));

  DICT_FOR(dit, var)
    printf("'%s' %i\n", var->key, var->value);
  DICT_FOR_END

  dict_destroy(dit);
  
  Dict2 dict2[1] = { DictionaryInit };

  dict_insert(dict2, 0, 0);

  for (int i = 0; i < 1000; i++) dict_insert(dict2, i, i + 1);

  int sum = 0;
  DICT_FOR(dict2, var)
    sum += var->key;
  DICT_FOR_END

  printf("%i\n", sum);

  dict_destroy(dict2);
}


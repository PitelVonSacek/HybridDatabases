#include "dictionary.h"
#include <stdio.h>

typedef Dictionary(const char*, int, STRING) Dict;
typedef Dictionary(int, int, NUMBER) Dict2;

void main() {
  Dict dit[1] = { DictionaryInit };

  const char* stringy[] = { "hello", " ", "world", "! ", 0 };
  for (int i = 0; stringy[i]; i++) dict_insert(dit, stringy[i], i * i);
  
  for (int i = 0; stringy[i]; i++) dict_insert(dit, stringy[i], i - 1);

  dict_remove(dit, "world");

  printf("%i %i\n", (int)dit->size, (int)dit->capacity);

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


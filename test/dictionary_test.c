#include "stdio.h"
#include "../utils/num_dictionary.h"

typedef NumDictionary(int, const char*) Dict1;

char *pole[] = {
  "fvf", "a", "ab", "abc", "abcd", "vgb ", "nf", 0 
};

void main() {
  Dict1 d[1] = { NumDictionaryInit };

  printf("%i %i\n", (int)sizeof(NumDictionary), (int)sizeof(Dict1));

  for (int i = 0; pole[i]; i++) ndict_insert(d, i*i, pole[i]);

  ndictFor(i, d) {
    printf("> %i '%s'\n", i->key, i->value);
  } ndictForEnd;

  ndict_remove(d, 9);

  printf("\n");

  ndictFor(i, d) {
    printf("> %i '%s'\n", i->key, i->value);
  } ndictForEnd;

  for (int i = 0; i < 1000000; i++) ndict_insert(d, i, "");

  ndict_destroy(d);
}


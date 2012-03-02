#include "stdio.h"

#define _num_dict_debug(...) \
  do { printf("%4i: ", __LINE__); printf("" __VA_ARGS__); printf("\n"); } while (0)

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

  const size_t million = 1000000;
  printf("Insert million elements\n");

  for (int i = 0; i < million; i++) if (!ndict_insert(d, i, "")) printf(".");

  printf("Size: %i\n", (int)d->size);
  
  /*ndictFor(i, d) {
    printf("> %i '%s'\n", i->key, i->value);
  } ndictForEnd;*/

  printf("And remove half of them\n");

  for (int i = 1; i < million; i += 2) if (!ndict_remove(d, i)) printf(".");

  printf("Size: %i\n", (int)d->size);

  /*ndictFor(i, d) {
    printf("> %i '%s'\n", i->key, i->value);
  } ndictForEnd;*/

  printf("Done\n");

  int sum = 0;

  ndictFor(i, d) {
    sum++;  
  } ndictForEnd;

  printf("%i\n", sum);

  ndict_destroy(d);
}


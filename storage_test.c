#include "storage.h"
#include "utils.h"

unsigned char *d;
size_t l;
size_t offset = 0;

size_t read(void *unused, size_t _l, unsigned char** out) {
  *out = d + offset;
  if (offset + _l > l) {
    _l = l - offset;
    offset = l;
    return _l;
  }
  offset += _l;
  return _l;
}

void main() {
  Writer* W = writer_create();

  W_ARRAY {
    W_NUMBER(15);
    W_NUMBER(1025);
    memcpy(W_STRING(10), "testicek X", 10);
  } W_ARRAY_END;

  l = w_length(W);
  memcpy(d = malloc(l), w_ptr(W), l);
  writer_free(W);

  printf("Length: %i\nHex:\n", (int)l);
  for (int i = 0; i < l; i++) printf(" %02x", (int)d[i]);

  printf("\n");
  for (int i = 0; i < l; i++) printf("%c", d[i]);
  printf("\n");
  
  Reader* R = reader_create(read, 0, 0);

  BEGIN {
    R_ARRAY 
      printf(".");;
      printf("číslo: %Lu\n", R_NUMBER);
      printf("číslo: %Lu\n", R_NUMBER);
      printf("string (%u): ", (unsigned int)R_LENGTH); printf("'%s'\n", R_STRING(R_LENGTH));
    R_ARRAY_END
    printf(".");
  } CATCH {
    printf("Chybka\n");
  } END_NOTHROW

  //for (int i = 0; i < 20; i++) printf(" %2i: %i\n", i, ulog2(i));
}


/**
 * @file
 * @brief Utilita pro práci s nižší vrstvou diskového formátu.
 *
 * Načítá data ze standardního vstupu a vypisuje je v člověkem čitelné
 * formě na standardní výstup.
 */

#include "storage.h"
#include <stdio.h>

void hex_print(size_t l, const unsigned char *str) {
  for (size_t i = 0; i < l; i++) {
    if (i % 16 == 0) printf("\n %03i:  ", (int)i);
    if (i % 16 == 8) printf("  ");
    printf("%02x ", (int)str[i]);
  }

  printf("\n");
}

void print(size_t l, const unsigned char *str) {
  printf("%i ", (int)l);
  printf("\"");

  for (size_t i = 0; i < l; i++) switch (str[i]) {
  case ' ':
  case '0' ... '9':
  case 'A' ... 'Z':
  case 'a' ... 'z':
  case '_':
  case '+':
  case '-':
  case '*':
  case '/':
  case '\'':
  case '&':
  case '|':
  case '=':
  case '<':
  case '>':
    printf("%c", str[i]);
    break;
  case '%':
    printf("%%");
    break;
  default:
    printf("%%%02x", (int)str[i]);
  }

  printf("\"");

  if (l <= 8) {
    uint64_t num = 0;
    for (int i = l - 1; i >= 0; i--) num = (num << 8) | str[i];
    printf(" as number: %li (0x%lx)", (int64_t)num, num);
  }
}

void ind(int indent) {
  for (int i = 0; i < indent; i++) printf("  ");
}

bool process_element(Reader *R) {
  int indent = 0;

  printf("Element: length %i", (int)(R->end - R->begin));
  hex_print(R->end - R->begin, R->begin);
  printf("\n");

  do {
    switch (rNext) {
      case ST_ARRAY_BEGIN:
        ind(indent++); printf("{\n");
        if (!read_array(R)) goto error;
        break;

      case ST_ARRAY_END:
        ind(--indent); printf("}\n");
        if (!read_array_end(R)) goto error;
        break;
 
      case ST_STRING: {
        size_t length = 0;
        const void *str = 0;
        if (!read_string(R, &str, &length)) goto error;
        ind(indent); print(length, str); printf("\n");
        break;
      }
        
      default:
       goto error;
    }
  } while (indent != 0);

  switch (rNext) {
    case ST_DOT:
      printf("No checksum");
      if (!reader_finish(R, 0)) goto error;
      break;
    case ST_CRC:
      printf("With checksum");
      if (!reader_finish(R, 1)) goto error;
      break;
    default:
      goto error;
  }
  
  printf(" OK\n\n\n");

  return true;

  error:
  printf("\nReading element failed!\nCurrent byte is %i (0x%x)\n", 
         (int)*R->ptr, (int)*R->ptr);
  return false;
}

int main() {
  Reader R[1];
  
  file_reader_init(R, stdin, 0);

  while (rMayBegin) 
    if (!process_element(R)) goto end;
 
  printf("End of file\n");

  if (0) {
    read_failed:
    printf("Error reading next element!\n");
  }

  end:
  reader_destroy(R);

  return 0;
}


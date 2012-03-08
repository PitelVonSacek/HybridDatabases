#include "storage.h"
#include <stdio.h>

bool process_element(Reader *R, Writer *W) {
  int indent = 0;

  do {
    switch (rNext) {
      case ST_ARRAY_BEGIN:
        write_array(W);
        indent++;
        if (!read_array(R)) goto error;
        break;

      case ST_ARRAY_END:
        write_array_end(W);
        indent--;
        if (!read_array_end(R)) goto error;
        break;
 
      case ST_STRING: {
        size_t length = 0;
        const void *str = 0;
        if (!read_string(R, &str, &length)) goto error;
        memcpy(write_string(W, length), str, length);
        break;
      }
        
      default:
       goto error;
    }
  } while (indent != 0);

  switch (rNext) {
    case ST_DOT:
      if (!reader_finish(R, 0)) goto error;
      writer_finish(W, 0);
      break;
    case ST_CRC:
      if (!reader_finish(R, 1)) goto error;
      writer_finish(W, 1);
      break;
    default:
      goto error;
  }

  fprintf(stderr, ".");

  return true;

  error:
  fprintf(stderr, "\nReading element failed!");
  return false;
}

void main() {
  Reader R[1];
  Writer W[1];
  
  file_reader_init(R, stdin, 0);
  writer_init(W);

  while (rMayBegin) {
    if (!process_element(R, W)) goto end;
    fwrite(writer_ptr(W), 1, writer_length(W), stdout);
    writer_discart(W);
  }
 
  fprintf(stderr, "Success\n");

  if (0) {
    read_failed:
    fprintf(stderr, "Error reading next element!\n");
  }

  end:
  writer_destroy(W);
  reader_destroy(R);
}


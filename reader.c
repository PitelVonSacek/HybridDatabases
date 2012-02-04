#include "storage.h"

int _r_fetch(Reader* R) {
  switch (R->read(R->read_context, 10, &R->begin)) {
    case 0:
      R->next = 15;
      return -1; // end of input
    case 10:
      if (R->begin[0] == ST_LEN_BEGIN && R->begin[9] == ST_LEN_END) break;
    default: return 0; // error
  }

  uint64_t length = 0;
  for (int i = 8; i >= 1; i--) length = length * 256 + R->begin[i];

  if (R->read(R->read_context, length, &R->begin) != length)
    return 0;

  R->end = R->begin + length;
  R->ptr = R->begin;

  return 1;
}

int _r_next_(Reader* R) {
  uint64_t x = R->next;
  R->next = 0;

  int i = 7;
  for (; i >= 0 && (x & (1 << i)); i--) {
    if (R->ptr == R->end) return 0;
    x ^= (1 << i);
    R->next = (R->next << 8) | *R->ptr++;
  }
  if (i != -1)
    R->next |= (x << (8 * (7 - i)));
  return 1;
}


Reader* reader_create(size_t (*read)(void*, size_t, unsigned char**),
                      void* read_context,
                      void (*release_context)(void*)) {
  Reader *R = malloc(sizeof(*R));
  R->read = read;
  R->read_context = read_context;
  R->release_context = release_context;

  if (_r_fetch(R) > 0) _r_next(R);
  return R;
}

void reader_free(Reader* R) {
  if (R->release_context) R->release_context(R->read_context);
  free(R);
}

// when at the end of root element, this one checks CRC checksum
// 1 means correct
// 0 is fail
// -1 there's no checksum
int r_checksum(Reader* R) {
  if (R->ptr > R->end) return 0;
  uint32_t crc;
  if (R->ptr[-1] == ST_DOT) {
    if (_r_fetch(R) > 0) _r_next(R);
    return -1;
  }
  if (R->ptr[-1] != ST_CRC || R->ptr[4] != ST_DOT) return 0;

  for (int i = 3; i >= 0; i--) crc = (crc << 8) | R->ptr[i];
  if (crc32_c(R->begin, R->ptr - 1, 0) != crc) return 0;

  if (_r_fetch(R) > 0) _r_next(R);
  return 1;
}

void r_skip(Reader *R) {
  size_t depth = 0;

  while (1) switch (r_next(R)) {
    case ST_ARRAY_BEGIN:
      depth++;
      r_array(R);
      break;
    case ST_ARRAY_END:
      if (depth-- == 0) return;
      r_array_end(R);
      break;
    default:
      if (r_next(R) < ST_STRING) THROW(R_UNEXPECTED, R_EXC_INFO);
      r_string(R);
  }
}


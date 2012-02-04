#include "storage.h"
#include "utils.h"

Writer* writer_create() {
  Writer* W = malloc(sizeof(*W));
  writer_init(W);
  return W;
}

void writer_init(Writer* W) {
  W->depth = 0;
  W->begin = ((char*)malloc(4000 + 10)) + 10; // 4k should be enough for everyone :-)

  W->begin[-10] = ST_LEN_BEGIN;
  W->begin[-1]  = ST_LEN_END;

  W->end = W->begin + 4000;
  W->ptr = W->begin;
}

void writer_destroy(Writer* W) {
  free(W->begin - 10);
}

void writer_free(Writer* W) {
  writer_destroy(W);
  free(W);
}

void _w_length(Writer* W, uint64_t n) {
  int l = ulog2(n);
  if (l < 7) *W->ptr++ = n;
  else if (l >= 56) {
    *W->ptr++ = 0xFF;
    W->ptr += 8;
    for (int i = 1; i <= 8; i++, n /= 256) W->ptr[-i] = n & 0xFF;
  } else {
    l = l / 7;
    *W->ptr++ = ((unsigned char)~(((unsigned)0xFF) >> l)) | (n >> (8*l));

    W->ptr += l;
    for (int i = 1; i <= l; i++, n /= 256) W->ptr[-i] = n & 0xFF;
  }
}

void w_checksum(Writer* W) {
  if (W->end - W->ptr < 5) _w_get_space(W, 5);

  unsigned crc = crc32_c(W->begin, W->ptr, 0);

  *W->ptr++ = ST_CRC;
  for (int i = 0; i < 4; i++, crc /= 256) *W->ptr++ = crc & 0xFF;
}

void w_discart(Writer* W) { W->ptr = W->begin; W->depth = 0; }

void *w_ptr(Writer* W) {
  /* fix hidden length field */
  // // FIXME dirty, works only on LE machines !!
  // *(int64_t*)(W->begin - 9) = W->ptr - W->length;
  if (W->end - W->ptr < 1) _w_get_space(W, 1);
  *W->ptr = ST_DOT;

  uint64_t l = 1 + W->ptr - W->begin;
  unsigned char *ptr = W->begin - 9;
  for (int i = 0; i < 8; i++, l /= 256) *ptr++ = l % 256;

  return W->begin - 10;
}
uint64_t w_length(Writer* W) { return (W->ptr - W->begin) + 11; }

void _w_get_space(Writer *W, size_t space) {
  size_t cur_size = W->end - W->begin;
  space += cur_size;
  // make sure that size increases exponentialy
  if (space < (cur_size * 3) / 2) space = (cur_size * 3) / 2;
  void *data = ((char*)realloc(W->begin - 10, space + 10)) + 10;
  W->end = data + space;
  W->ptr = data + (W->ptr - W->begin);
  W->begin = data;
}

uint64_t w_get_cur_pos(Writer* W) {
  return W->ptr - W->begin;
}
void w_set_cur_pos(Writer* W, uint64_t pos) {
  W->ptr = W->begin + pos;
}





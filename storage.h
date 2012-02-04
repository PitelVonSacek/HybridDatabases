#ifndef __STORAGE_H__
#define __STORAGE_H__

#include <stdint.h>
#include <stdlib.h>
#include "exception.h"

EXCEPTION_DECLARE(R_BAD_INPUT, NONE);
  EXCEPTION_DECLARE(R_UNEXPECTED, R_BAD_INPUT);
  EXCEPTION_DECLARE(R_BAD_FORMAT, R_BAD_INPUT);

enum {
  ST_ARRAY_BEGIN = 0x01,
  ST_ARRAY_END = 0x03,
  ST_DOT = 0x02,
  ST_LEN_BEGIN = 0x07,
  ST_LEN_END = 0x09,
  ST_CRC = 0x04,
  ST_STRING = 16
};

unsigned crc32_c(const unsigned char* begin, const unsigned char* end, unsigned base);

typedef struct {
  size_t (*read)(void*, size_t, unsigned char**);
  void* read_context;
  void (*release_context)(void*);
  int depth;
  uint64_t next;
  unsigned char *begin, *end, *ptr;
} Reader;

int _r_fetch(Reader* R);
int _r_next_(Reader* R);

static inline int _r_next(Reader* R) {
  if (R->ptr == R->end) _r_fetch(R);
  if (R->ptr == R->end) return -1;

  R->next = *R->ptr++;
  if (R->next >= 128) return _r_next_(R);

  return 1;
}

Reader* reader_create(size_t (*read)(void*, size_t, unsigned char**),
                      void* read_context,
                      void (*release_context)(void*));

void reader_free(Reader* R);

/*
 * returns next item in stream
 * 15 means end of input
 * 0 error
 * 1 begin of array
 * 3 end of array
 * >=16 string with length n - 16
 */
static inline uint64_t r_next(Reader* R) { return R->next; }

static inline int r_array(Reader* R) {
  if (R->ptr == R->end || R->next != ST_ARRAY_BEGIN) return 0;

  _r_next(R);
  return 1;
}
static inline int r_array_end(Reader* R) {
  if (R->ptr == R->end || R->next != ST_ARRAY_END) return 0;

  _r_next(R);
  return 1;
}
static inline void* r_string(Reader* R) {
  if (R->ptr == R->end || R->next < 16) return 0;

  void* r = R->ptr;
  R->ptr += R->next - 16;
  _r_next(R);
  return r;
}


// when at the end of root element, this one checks CRC checksum
// 1 means correct
// 0 is fail
// -1 there's no checksum
int r_checksum(Reader* R);


// skips elements up to end of current array
void r_skip(Reader* R);

/* at begining of each root element:
 * '0x07' 'length of next elem without this header but with CRC,
 *  in 8 bytes LE' '0x09'
 */



typedef struct {
  int depth;
  unsigned char *begin, *end, *ptr;
} Writer;

void _w_get_space(Writer *W, size_t space);

Writer* writer_create();
void writer_init(Writer* W);
void writer_destroy(Writer* W);
void writer_free(Writer* W);

static inline void w_array(Writer* W) {
  if (W->ptr == W->end) _w_get_space(W, 1);
  *W->ptr++ = ST_ARRAY_BEGIN;
  W->depth++;
}

static inline int w_array_end(Writer* W) {
  if (W->depth-- < 1) return 0;
  if (W->ptr == W->end) _w_get_space(W, 1);
  *W->ptr++ = ST_ARRAY_END;

  return 1;
}

void _w_length(Writer* W, uint64_t n);

static inline void* w_string(Writer* W, uint64_t length) {
  if (W->end - W->ptr < length + 9) _w_get_space(W, length + 9);

  uint64_t n = length + ST_STRING;
  if (n < 128) *W->ptr++ = n;
  else _w_length(W, n);

  void *p = W->ptr;
  W->ptr += length;
  return p;
}

static inline void w_number(Writer* W, uint64_t num) {
  if (W->end - W->ptr < 9) _w_get_space(W, 9);

  unsigned char *o = ++W->ptr;

  for (; num; num /= 256) *W->ptr++ = num & 0xFF;

  o[-1] = ST_STRING + (W->ptr - o);
}

void w_checksum(Writer*);
void w_discart(Writer*);

void *w_ptr(Writer*);
uint64_t w_length(Writer*);

uint64_t w_get_cur_pos(Writer*);
void w_set_cur_pos(Writer*, uint64_t pos);

#include "storage_macros.h"

#endif


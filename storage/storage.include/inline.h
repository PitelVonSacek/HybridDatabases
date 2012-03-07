#ifdef __STORAGE_INLINE_INCLUDE_H__
#undef __STORAGE_INLINE_INCLUDE_H__

// Reader

static inline size_t reader_next(Reader *R) {
  if (R->ptr >= R->end) return ST_ERROR;
  switch (*R->ptr) {
    case ST_ARRAY_BEGIN:
    case ST_ARRAY_END:
    case ST_CRC:
    case ST_DOT:
      return *R->ptr;
    default:
      return (*R->ptr >= ST_STRING) ? ST_STRING : ST_ERROR;
  }
}

#define _reader_ensure_space(size) \
  do { if (R->ptr + (size) > R->end) return false; } while (0)

static inline bool read_array(Reader *R) {
  _reader_ensure_space(1);
  R->depth++;
  return *R->ptr++ == ST_ARRAY_BEGIN;
}

static inline bool read_array_end(Reader *R) {
  _reader_ensure_space(1);
  assert(R->depth > 0);
  R->depth--;
  return *R->ptr++ == ST_ARRAY_END;
}

static inline bool read_string(Reader *R, const void **ptr, size_t *length) {
  _reader_ensure_space(1);
  unsigned char first = *R->ptr++;
  if (first < ST_STRING) return false;

  uint64_t len = 0;
  if (first < 0x80) {
    len = first;
  } else if (first == 0xFF) {
    _reader_ensure_space(8);
    for (int i = 7; i >= 0; i--) len = (len << 8) | R->ptr[i];
    R->ptr += 8;
  } else {
    int l = 0;
    for (unsigned i = first; i & 0x80; i <<= 1) l++;
    
    len = first & (((uint64_t)0xFF) >> l);
    for (int i = l - 1; i >= 0; i--) len = (len << 8) | R->ptr[i];
    R->ptr += l;
  }
  
  len -= ST_STRING;
  _reader_ensure_space(len);
  *length = len;
  *ptr = R->ptr;
  R->ptr += len;

  return true;
}

static inline bool read_number(Reader *R, uint64_t *value) {
  _reader_ensure_space(1);
  int l = *R->ptr++;
  if (l < ST_STRING || l > ST_STRING + 8) return false;
  l -= ST_STRING;
  
  _reader_ensure_space(l);
  uint64_t ret = 0;
  int i = l;
  while (i--) ret = (ret << 8) | R->ptr[i];
  R->ptr += l;
  *value = ret;

  return true;
}

static inline size_t reader_get_pos(Reader *R) {
  return R->ptr - R->begin;
}

static inline void reader_set_pos(Reader *R, size_t pos) {
  R->ptr = R->begin + pos;
}

// Writer

void _writer_get_space(Writer *W, size_t size);
#define _writer_ensure_space(size) \
  do { \
    if (W->ptr + (size) > W->end) \
      _writer_get_space(W, (W->ptr + (size)) - W->end); \
  } while (0)

void _write_number(Writer *W, uint64_t n);

static inline void write_array(Writer *W) {
  _writer_ensure_space(1);
  *W->ptr++ = ST_ARRAY_BEGIN;
  W->depth++;
}

static inline void write_array_end(Writer *W) {
  _writer_ensure_space(1);
  assert(W->depth > 0);
  W->depth--;
  *W->ptr++ = ST_ARRAY_END;
}

static inline void *write_string(Writer *W, uint64_t length) {
  _writer_ensure_space(length + 9);
  if (length + ST_STRING < 128) *W->ptr++ = length + ST_STRING;
  else _write_number(W, length + ST_STRING);
  
  void *ret = W->ptr;
  W->ptr += length;
  return ret;
}

static inline void write_number(Writer *W, uint64_t number) {
  _writer_ensure_space(9);
  unsigned char *ptr = ++W->ptr;
  for (; number; number >>= 8) *W->ptr++ = number & 0xFF;
  ptr[-1] = ST_STRING + (W->ptr - ptr);
}

#else
# error "Don't include this file directly. Use storage.h instead."
#endif


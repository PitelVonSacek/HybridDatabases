Writer* writer_create() {
  Writer *W = malloc(sizeof(*W));
  writer_init(W);
  return W;
}

void writer_free(Writer *W) {
  writer_destroy(W);
  free(W);
}

void writer_init(Writer* W) {
  const size_t default_size = 4000;
  W->depth = 0;
  W->real_begin = malloc(default_size + 10);
  W->begin = W->real_begin + 10;
  W->ptr = W->begin;
  W->end = W->begin + default_size;
}

void writer_destroy(Writer *W) {
  free(W->real_begin);
}

void _writer_get_space(Writer *W, size_t size) {
  size_t cur_size = W->end - W->real_begin;
  size_t new_size = cur_size + size;
  if (new_size < (cur_size * 3) / 2) new_size = (cur_size * 3) / 2;
  unsigned char *data = realloc(W->real_begin, new_size);
  W->end = data + new_size;
  W->ptr = data + (W->ptr - W->real_begin);
  W->begin = data + (W->begin - W->real_begin);
  W->real_begin = data;
}

void _write_number(Writer *W, uint64_t n) {
  if (!(n & ~(uint64_t)0x7F)) // fits in 1 byte
    *W->ptr++ = n & 0x7F;
  else if (n & (((uint64_t)0xFF) << 56)) { // upper byte is nonempty
    *W->ptr++ = 0xFF;
    for (int i = 0; i < 8; i++, n >>= 8) W->ptr[i] = n & 0xFF;
    W->ptr += 8;
  } else {
    int i = 0;
    unsigned char max = 0x80;
    while (max <= n) {
      W->ptr[++i] = n & 0xFF;
      n >>= 8;
      max >>= 1;
    }
    W->ptr[0] = ((~(((uint64_t)0xFF) >> i)) | n) & 0xFF;
    W->ptr += i + 1;
  }
}

void writer_finish(Writer *W, bool checksum) {
  assert(W->depth == 0);

  _writer_ensure_space(16); // 1 DOT, 5 CRC, 10 next header

  if (checksum) {
    unsigned crc = crc32_c(W->begin, W->ptr, 0);
    storageDebug("CRC: %x\n", crc);
    *W->ptr++ = ST_CRC;
    for (int i = 0; i < 4; i++, crc >>= 8) *W->ptr++ = crc & 0xFF;
  }

  *W->ptr++ = ST_DOT;

  uint64_t length = W->ptr - W->begin;
  W->begin[-10] = ST_LEN_BEGIN;
  for (int i = -9; i < -1; i++, length >>= 8) W->begin[i] = length & 0xFF;
  W->begin[-1] = ST_LEN_END;

  W->ptr += 10;
  W->begin = W->ptr;
}

void writer_discart(Writer *W) {
  W->begin = W->ptr = W->real_begin + 10;
}

void *writer_ptr(Writer *W) {
  assert(W->begin == W->ptr);
  return W->real_begin;
}

size_t writer_length(Writer *W) {
  assert(W->begin == W->ptr);
  return (W->ptr - W->real_begin) - 10;
}


bool read_header(Reader *R, void *header, size_t *length) {
  unsigned char *ptr = header;
  if (ptr[0] != ST_LEN_BEGIN || ptr[9] != ST_LEN_END) return false;

  uint64_t tmp = 0;
  for (int i = 8; i > 0; i--) tmp = (tmp << 8) | ptr[i];
  *length = tmp;

  R->end = ((unsigned char*)0) + tmp;

  return true;
}

bool read_begin(Reader *R, void *data) {
  R->begin = R->ptr = data;
  R->end = R->begin + (R->end - (unsigned char*)0);
  R->depth = 0;
  return true;
}

bool reader_finish(Reader *R, bool checksum) {
  assert(R->depth == 0);

  _reader_ensure_space(1);

  if (*R->ptr == ST_CRC) {
    _reader_ensure_space(6);
    unsigned crc = crc32_c(R->begin, R->ptr, 0);
    unsigned orig_crc = 0;
    for (int i = 4; i > 0; i--) orig_crc = (orig_crc << 8) | R->ptr[i];

    return (R->ptr + 6 == R->end) && 
           (R->ptr[0] == ST_CRC) &&
           (R->ptr[5] == ST_DOT) &&
           crc == orig_crc;
  }

  return (R->ptr + 1 == R->end) && (*R->ptr == ST_DOT) && !checksum;
}


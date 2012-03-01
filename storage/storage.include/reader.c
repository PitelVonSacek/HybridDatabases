struct FileReaderContext {
  FILE *file;
  void *buf;
  size_t buf_size;
  bool close;
};

static struct FileReaderContext *file_reader_context(FILE *f, bool close) {
  struct FileReaderContext *context = malloc(sizeof(*context));

  *context = (struct FileReaderContext){
    .file = f,
    .buf = 0,
    .buf_size = 0,
    .close = close
  };

  return context;
}

static int file_reader_read(void *c, size_t length, void **output) {
  struct FileReaderContext *ctx = c;

  if (ctx->buf_size < length) {
    free(ctx->buf);
    ctx->buf = malloc(length);
  }

  size_t read = fread(ctx->buf, 1, length, ctx->file);

  *output = ctx->buf;

  if (read == length) return ST_READ_SUCCESS;
  if (read == 0) return ST_READ_END_OF_FILE;
  return ST_READ_FAIL;
}

static void file_reader_destroy(void *c) {
  struct FileReaderContext *ctx = c;
  free(ctx->buf);
  if (ctx->close) fclose(ctx->file);
  free(ctx);
}

void file_reader_init(Reader *R, FILE *file, bool close) {
  reader_init(R, &file_reader_read,
              file_reader_context(file, close), 
              &file_reader_destroy);
}


Reader *reader_create(ReadFunction read, void *context, 
                      void (*destroy_context)(void*)) {
  Reader *R = malloc(sizeof(*R));
  reader_init(R, read, context, destroy_context);
  return R;
}

void reader_free(Reader *R) {
  reader_destroy(R);
  free(R);
}

void reader_init(Reader *R, ReadFunction read, void *context, 
                 void (*destroy_context)(void*)) {
  *R = (Reader){
    .context = context,
    .read = read,
    .destroy_context = destroy_context,
    .begin = 0,
    .ptr = 0,
    .end = 0,
    .depth = 0
  };
}

void reader_destroy(Reader *R) {
  if (R->destroy_context)
    R->destroy_context(R->context);
}

static bool read_header(void *header, size_t *length) {
  unsigned char *ptr = header;
  if (ptr[0] != ST_LEN_BEGIN || ptr[9] != ST_LEN_END) return false;

  uint64_t tmp = 0;
  for (int i = 8; i > 0; i--) tmp = (tmp << 8) | ptr[i];
  *length = tmp;

  return true;
}

int reader_begin(Reader *R) {
  void *header = 0;
  size_t length = 0;

  switch (R->read(R->context, 10, &header)) {
    case ST_READ_SUCCESS: 
      break;
    case ST_READ_END_OF_FILE:
      return ST_READ_END_OF_FILE;
    case ST_READ_FAIL:
      return ST_READ_FAIL;
    default:
      assert(0);
  }

  if (!read_header(header, &length)) return ST_READ_FAIL;

  if (R->read(R->context, length, (void**)&R->begin) != ST_READ_SUCCESS)
    return ST_READ_FAIL;

  R->ptr = R->begin;
  R->end = R->begin + length;
  R->depth = 0;

  return ST_READ_SUCCESS;
}

bool reader_finish(Reader *R, bool checksum) {
  assert(R->depth == 0);

  _reader_ensure_space(1);

  if (*R->ptr == ST_CRC) {
    _reader_ensure_space(6);
    unsigned crc = crc32_c(R->begin, R->ptr, 0);
    unsigned orig_crc = 0;
    for (int i = 4; i > 0; i--) orig_crc = (orig_crc << 8) | R->ptr[i];

    storageDebug("Crc: %x  orig_crc: %x\n", crc, orig_crc);

    return (R->ptr + 6 == R->end) && 
           (R->ptr[0] == ST_CRC) &&
           (R->ptr[5] == ST_DOT) &&
           crc == orig_crc;
  }

  return (R->ptr + 1 == R->end) && (*R->ptr == ST_DOT) && !checksum;
}


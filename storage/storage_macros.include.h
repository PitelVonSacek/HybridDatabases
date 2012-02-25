#ifdef __STORAGE_MACROS_INCLUDE_H__
#undef __STORAGE_MACROS_INCLUDE_H__

/*****************
 * Reader macros *
 *****************/

// when read fails, executes goto read_failed;

#define rArray do { if (!read_array(R)) goto read_failed; do {
#define rArrayEnd \
  } while (0); if (!read_array_end(R)) goto read_failed; } while (0)

#define rNumber \
  ({ \
    uint64_t _tmp = 0; \
    if (!read_number(R, &_tmp)) goto read_failed; \
    _tmp; \
  })

#define rCheckString(str) \
  do { \
    const char *_str = (str); \
    size_t _length = 0; \
    char *_string = 0; \
    if (!read_string(R, &_string, &_length) || \
        strlen(str) != _length || \
        strncmp(_string, _str, _length)) \
      goto read_failed; \
  } while (0)

#define rFinish(checksum) \
  do { if (!reader_finish(R, checksum)) goto read_failed; } while (0)


/*****************
 * Writer macros *
 *****************/

#define wArray do { write_array(W); 
#define wArrayEnd write_array_end(W); } while (0)

#define wRawString(length) (write_string(W, (length)))
#define wString(string) \
  do { \
    const char *__string = string; \
    size_t __length = strlen(__string); \
    memcpy(wRawString(__length), __string, __length); \
  } while (0)

#define wNumber(n) (write_number(W, (n)))

#define wFinish(checksum) writer_finish(W, checksum)

#else 
# error "Don't include this file directly. Include storage.h instead."
#endif


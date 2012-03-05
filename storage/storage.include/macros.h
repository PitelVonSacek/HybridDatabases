#ifdef __STORAGE_MACROS_INCLUDE_H__
#undef __STORAGE_MACROS_INCLUDE_H__

/*****************
 * Reader macros *
 *****************/

// when read fails, executes goto read_failed;

#define readFailed goto read_failed

#define rBegin \
  do { if (reader_begin(R) != ST_READ_SUCCESS) readFailed; } while (0)

#define rMayBegin \
  ({ \
    int __ret = reader_begin(R); \
    if (__ret == ST_READ_FAIL) readFailed; \
    __ret == ST_READ_SUCCESS; \
  })

#define rNext reader_next(R)

#define rSkip do { if (!reader_skip(R)) readFailed; } while (0)

#define rArray do { if (!read_array(R)) readFailed; do {
#define rArrayEnd \
  } while (0); if (!read_array_end(R)) readFailed; } while (0)

#define rNumber \
  ({ \
    uint64_t _tmp = 0; \
    if (!read_number(R, &_tmp)) readFailed; \
    _tmp; \
  })

#define rCheckNumber(num) \
  do { \
    if (rNumber != (num)) readFailed; \
  } while (0)

#define rCheckString(str) \
  do { \
    const char *_str = (str); \
    size_t _length = 0; \
    const void *_string = 0; \
    if (!read_string(R, &_string, &_length) || \
        strlen(str) != _length || \
        strncmp(_string, _str, _length)) \
      readFailed; \
  } while (0)

#define rRawString(len) \
  ({ \
    size_t __length = 0; \
    const void *__string = 0; \
    if (!read_string(R, &__string, &__length) || \
        __length != (len)) \
      readFailed; \
    __string; \
  })

#define rFinish(checksum) \
  do { if (!reader_finish(R, checksum)) readFailed; } while (0)


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


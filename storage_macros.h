#ifndef STORAGE_MACROS_H
#define STORAGE_MACROS_H

/*******************
 *  Reader macros  *
 *******************/

// All of them reqire Reader* R

#define R_STRINGIFY_(a) #a
#define R_STRINGIFY(a) R_STRINGIFY_(a)
#define R_EXC_INFO "error at " __FILE__ " line " R_STRINGIFY(__LINE__)

#define R_CHECKSUM(required) do { switch (r_checksum(R)) { \
  case 1: break; \
  case -1: if (!required) break; \
  default: THROW(R_UNEXPECTED, R_EXC_INFO); \
  } } while (0)

#define R_NEXT r_next(R)
#define R_LENGTH ({ if (r_next(R) < 16) THROW(R_UNEXPECTED, R_EXC_INFO); r_next(R) - 16; })
#define R_ARRAY do { if (!r_array(R)) THROW(R_UNEXPECTED, R_EXC_INFO); {
#define R_ARRAY_END } if (!r_array_end(R)) THROW(R_UNEXPECTED, R_EXC_INFO); } while (0);
#define R_STRING(length) ({ if (r_next(R) != length + 16) THROW(R_UNEXPECTED, R_EXC_INFO); r_string(R); })
#define R_NUMBER_(R) ({ \
    if (R->next < 16 || R->next > 16 + 8) THROW(R_UNEXPECTED, R_EXC_INFO); \
    uint64_t ret = 0; \
    for (int i = R->next - 17; i >= 0; i--) ret = (ret << 8) | R->ptr[i]; \
    R->ptr += R->next - 16; \
    _r_next(R); \
    ret; \
  })
#define R_NUMBER R_NUMBER_(R)
#define R_STR(string) do { \
  const char const * _str = (string); \
  size_t _len = strlen(_str); \
  if (strncmp(R_STRING(_len), _str, _len)) THROW(R_UNEXPECTED, R_EXC_INFO); \
  } while (0)

#define R_SKIP r_skip(R)


/*******************
 *  Writer macros  *
 *******************/

// All of them require Writer* W

#define W_ARRAY do { w_array(W); do {
#define W_ARRAY_END } while(0); w_array_end(W); } while (0);
#define W_STRING(l) (w_string(W, (l)))

#define W_STR(string) ({ \
  const char* _string = (string); \
  size_t l = strlen(_string); \
  memcpy(W_STRING(l), _string, l); \
  (void)0; \
})

#define W_NUMBER(n) (w_number(W, (n)))

#define W_CHECKSUM w_checksum(W)

#endif // STORAGE_MACROS_H

#ifndef __TR_EXCEPTION_H__
#define __TR_EXCEPTION_H__

#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct ExceptionHandler_ {
  struct ExceptionHandler_ *outter;
  jmp_buf target;
} ExceptionHandler;

typedef struct Exception_ {
  const struct Exception_ *anc;
  const char *name;
} Exception;

#define EXCEPTION_(e) Exception__##e
#define EXCEPTION(e) EXCEPTION_(e)

#define Exception__NONE 0

#define EXCEPTION_DECLARE(name_, anc_) extern const Exception EXCEPTION(name_)[1]
#define EXCEPTION_DEFINE(name_, anc_) const Exception EXCEPTION(name_)[1] = {{ \
    .name = #name_, \
    .anc = EXCEPTION(anc_) \
  }}


#define THROW(exc, info) ({ \
    exception_exception = EXCEPTION(exc); \
    exception_info = info; \
    THROW__; \
    (void)0; \
  })

#define THROW__ do { \
    if (exception_exception) { \
      if (!exception_handler) { \
        fprintf(stderr, "Uncatched exception '%s' at %s line %i\n", \
                exception_exception->name, __FILE__, __LINE__); \
        exit(1); \
      } \
      longjmp(exception_handler->target, 1); \
    } \
  } while (0)

#define RETHROW ({ exception_exception = exception; goto _exc_section_end; })
#define RETHROW_AS(exc, info) ({ \
    exception; /* to ensure using only in catch */ \
    exception_exception = EXCEPTION(exc); \
    exception_info = info; \
    goto _exc_section_end; \
  })

extern __thread ExceptionHandler *exception_handler;
extern __thread const Exception *exception_exception;
extern __thread void *exception_info;

static inline int exception_compare(const Exception* anc, const Exception* child) {
  do {
    if (anc == child) return 1;
  } while (child = child->anc);
  return 0;
}

#define TRY \
  do { \
    ExceptionHandler _exc_handler = { .outter = exception_handler  }; \
    exception_handler = &_exc_handler; \
    do { if (!setjmp(_exc_handler.target)) { \
      __label__ _exc_section_end;

#define CATCH(exc) \
      _exc_section_end: break; \
    } else if (exception_compare(EXCEPTION(exc), exception_exception)) { \
      __label__ _exc_section_end; \
      const Exception* exception = exception_exception; \
      exception_exception = 0;

#define CATCH_ALL \
      _exc_section_end: break; \
    } else { \
      __label__ _exc_section_end; \
      const Exception* exception = exception_exception; \
      exception_exception = 0;


#define FINALLY \
      _exc_section_end: break; \
    }} while (0); do {{ \
      __label__ _exc_section_end; \

#define END \
      _exc_section_end: break; \
    }} while (0); \
    exception_handler = exception_handler->outter; \
    if (exception_exception) THROW__; \
  } while (0);

#define ALWAYS(var) do { var; TRY
#define ALWAYS_END(cmd) FINALLY { cmd; } END } while(0);

#define ENSURE(expr, exc, info) ({ \
    typeof(expr) _expr = (expr); \
    if (!_expr) THROW(exc, info); \
    _expr; \
  })

#define EXCEPTION_STRINGIFY(a) EXCEPTION_STRINGIFY_(a)
#define EXCEPTION_STRINGIFY_(a) #a

EXCEPTION_DECLARE(OOPS, NONE);
#define THROW_ THROW(OOPS, "Oops at "__FILE__ " line " EXCEPTION_STRINGIFY(__LINE__))
#define ENSURE_(expr) ENSURE(expr, OOPS, "Oops at " __FILE__ " line " EXCEPTION_STRINGIFY(__LINE__))

#endif


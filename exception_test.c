#include "exception.h"

EXCEPTION_DECLARE(TEST, NONE);
EXCEPTION_DECLARE(TEST2, TEST);
EXCEPTION_DECLARE(TEST3, NONE);

EXCEPTION_DEFINE(TEST, NONE);
EXCEPTION_DEFINE(TEST2, TEST);
EXCEPTION_DEFINE(TEST3, NONE);

void f() {
  TRY 
    THROW(TEST, 0);
  CATCH(TEST) 
    printf(".\n");
    RETHROW_AS(TEST3, 0);
  FINALLY
    printf("f1\n");
  END
}

int g() { return 0; }

void main() {
  TRY {
    f();
  
    printf("success\n");
  } CATCH(TEST2) {
    printf("catched test\n");
  } CATCH_ALL {
  } FINALLY {
    printf("f2\n");
  } END

  TRY
    ALWAYS(void *pamet = malloc(sizeof(123)))
      THROW_;
    ALWAYS_END(free(pamet))
  CATCH_ALL
    printf("%s \n", exception_info);
  END

  printf("%i\n", ENSURE(g(), TEST, 0));
}


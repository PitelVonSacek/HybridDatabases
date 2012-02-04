#include "exception.h"

__thread ExceptionHandler *exception_handler = 0;
__thread const Exception *exception_exception = 0;
__thread void *exception_info;

EXCEPTION_DEFINE(OOPS, NONE);


#ifndef __TYPE_MAGIC_H__
#define __TYPE_MAGIC_H__

#include "static_if.h"

typedef struct {} DummyAncestor;

#define typeUncast(object) \
  static_if( \
    types_equal(typeof((object)->__ancestor), DummyAncestor), \
    (object), \
    &((object)->__ancestor) \
  )

#endif


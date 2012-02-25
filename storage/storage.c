#include "storage.h"

static unsigned crc32_c(const unsigned char *b, 
                        const unsigned char *e, 
                        unsigned x);

#include "writer.include.c"
#include "reader.include.c"
#include "crc.include.c"


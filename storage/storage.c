#include "storage.h"

static unsigned crc32_c(const unsigned char *b, 
                        const unsigned char *e, 
                        unsigned x);

#include "storage.include/writer.c"
#include "storage.include/reader.c"
#include "storage.include/crc.c"


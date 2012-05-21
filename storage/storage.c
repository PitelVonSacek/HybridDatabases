#include "storage.h"

/**
 * @file
 * @brief Hlavní zdrojový soubor nižší vrstvy souborového formátu.
 */

/**
 * Spočte kontrolní kód CRC32 Castagnoli z dat [ @a b, @a e ).
 * Jako počáteční hodnotu použije @a x.
 */
static unsigned crc32_c(const unsigned char *b, 
                        const unsigned char *e, 
                        unsigned x);

#include "storage.include/writer.c"
#include "storage.include/reader.c"
#include "storage.include/crc.c"


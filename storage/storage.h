#ifndef __STORAGE_H__
#define __STORAGE_H__

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <stdio.h>

/*
 * File format description:
 * FILE := ROOT_ELEMENT*
 * ROOT_ELEMENT := ST_LEN_BEGIN 'length in 8 bytes LE' ST_LEN_END ELEMENT 
 *                 (ST_CRC 'crc32_c in 4 bytes LE')? ST_DOT
 * ELEMENT := NUMBER 'number - 16 bytes' | ST_ARRAY_BEGIN ELEMENT* ST_ARRAY_END
 * NUMBER := ala utf8 encoded number, LE but top most bits are in 1st byte
 */

#ifndef storageDebug
#define storageDebug(...)
#endif

enum {
  ST_ARRAY_BEGIN = 0x01,
  ST_ARRAY_END = 0x03,
  ST_DOT = 0x02,
  ST_LEN_BEGIN = 0x07,
  ST_LEN_END = 0x09,
  ST_CRC = 0x04,
  ST_STRING = 16,

  ST_ERROR = 15,

  ST_READ_FAIL = 0,
  ST_READ_SUCCESS,
  ST_READ_END_OF_FILE
};


/**************
 *   Reader   *
 **************/

// returns ST_READ_*
typedef int (*ReadFunction)(void *context, size_t length, void **output);

typedef struct {
  void *context;
  ReadFunction read; 
  void (*destroy_context)(void*);

  unsigned char *begin, *ptr, *end;
  int depth;
} Reader;

Reader *reader_create(ReadFunction read, void *context, 
                      void (*destroy_context)(void*));
void reader_free(Reader *R);

void reader_init(Reader *R, ReadFunction read, void *context, 
                 void (*destroy_context)(void*));

void file_reader_init(Reader *R, FILE *file, bool close);

void reader_destroy(Reader *R);

int reader_begin(Reader *R); // returns ST_READ_*
bool reader_finish(Reader *R, bool checksum);

bool reader_skip(Reader *R);

// Defined in storage_inline.include.h:
 
static inline size_t reader_next(Reader *R);

static inline bool read_array(Reader *R);
static inline bool read_array_end(Reader *R);

static inline bool read_string(Reader *R, const void **ptr, size_t *length);
static inline bool read_number(Reader *R, uint64_t *value);

/* Macros (requires Reader* named R):
#define rBegin
#define rMayBegin

#define rNext
#define rSkip

#define rArray
#define rArrayEnd

#define rNumber
#define rRawString

#define rCheckNumber(num)
#define rCheckString(string)

#define rFinish(checksum?)

*/


/**************
 *   Writer   *
 **************/

typedef struct {
  unsigned char *real_begin, *begin, *ptr, *end;
  int depth;
} Writer;

Writer* writer_create();
void writer_free(Writer *W);

void writer_init(Writer* W);
void writer_destroy(Writer *W);

// Defined in storage_inline.include.h:
 
static inline void write_array(Writer *W);
static inline void write_array_end(Writer *W);

static inline void *write_string(Writer *W, uint64_t length);
static inline void write_number(Writer *W, uint64_t number);

/* Macros (requires Writer* named W):
#define wArray
#define wArrayEnd

#define wNumber(number)
#define wRawString(length)  --  returns pointer to begin
#define wString(string)

#define wFinish(checksum?)

*/

void writer_finish(Writer *W, bool checksum);
void writer_discart(Writer *W);

void *writer_ptr(Writer *W);
size_t writer_length(Writer *W);

#define __STORAGE_INLINE_INCLUDE_H__
#include "storage.include/inline.h"

#define __STORAGE_MACROS_INCLUDE_H__
#include "storage.include/macros.h"

#endif


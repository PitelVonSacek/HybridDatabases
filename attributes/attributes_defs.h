#ifndef __ATTRIBUTES_DEFS_H__
#define __ATTRIBUTES_DEFS_H__

#include <stdint.h>

#include "../node.h"

#include "../storage/storage.h"
#include "../generic_allocator.h"

/*
 * Attribute types:
 * UInt8-64, Int8-64 - underlaying type is (u)int*_t
 * Float, Double, LDouble (float, double, long double)
 * String - const char* terminated by '\0̈́'
 * RawString - const char * with length
 * Pointer - pointer to another Node
 */


/*
 * Defined in preprocess.h, implementation generated by macros
 */

typedef struct { 
  size_t length; 
  char data[0]; 
} BinaryString;

#define MAX_ATTR_SIZE sizeof(union UniversalAttribute)

#include "generated_defs.h"

#endif


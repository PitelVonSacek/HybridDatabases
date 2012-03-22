# note: Doxygen docs are generated from attributes.h and attributes.include.h
# because it cannot parse this file

Interface {
/// @file

#include <stdint.h>
#include <stdlib.h>

/*
 * Attribute types:
 * UInt8-64, Int8-64 - underlaying type is (u)int*_t
 * Float, Double, LDouble (float, double, long double)
 * String - const char* terminated by '\0̈́'
 * RawString - const char * with length
 * Pointer - pointer to another Node
 */


/**
 * @brief Struktura obsahující libovolná binární data.
 *
 * Tato struktura funguje jako hlavička pro uskladnění binárních dat
 * v rámci databáze.
 */
struct RawString {
  size_t length; ///< Délka dat.
  char data[0];  ///< Ukazatel na počátek dat.
};


/**
 * @union UniversalAttribute
 * @brief Union obsahující všechny možné atributy.
 *
 * Slouží především k výpočtu maximální velikosti atributu.
 * @see #MAX_ATTR_SIZE
 */

/// Konstanta popisující maximální velikost libovolného atributu
#define MAX_ATTR_SIZE sizeof(union UniversalAttribute)

#include "../database.types/node.h"
#include "../allocators/generic_allocator.h"
#include "../storage/storage.h"

struct Handler_;

/// @returns Velikost daného typu atributu.
static inline size_t attribute_size(int type);

static inline void attribute_init(int type, void *attr);
static inline void attribute_destroy(int type, struct GenericAllocator *allocator,
                                     uint64_t end_time, void *attr);

static inline bool attribute_write(int type, struct Handler_ *H,
                                   void * restrict attr, const void * restrict value);

static inline void attribute_store(int type, Writer *W, const void *value);
static inline bool attribute_load(int type, Reader *R,
                                  struct GenericAllocator *allocator,
                                  void *attr);
}

Implementation {
/// @file

#include "../attributes/attributes.h"
#include "../storage/storage.h"

#include "../database.types/attributes.h"
#include "../database.types/handler.h"
#include "../database.types/node.h"

#include "../database.h"

}

####################
#   Signed types   #
####################

AttributeType Int8 : int8_t {
  Load  { attr->value = *(int8_t*)rRawString(sizeof(int8_t)) }
  Store { *(int8_t*)wRawString(sizeof(int8_t)) = *value }
}

AttributeType Int16 : int16_t {
  Load  { attr->value = *(int16_t*)rRawString(sizeof(int16_t)) }
  Store { *(int16_t*)wRawString(sizeof(int16_t)) = *value }
}

AttributeType Int32 : int32_t {
  Load  { attr->value = *(int32_t*)rRawString(sizeof(int32_t)) }
  Store { *(int32_t*)wRawString(sizeof(int32_t)) = *value }
}

AttributeType Int64 : int64_t {
  Load  { attr->value = *(int64_t*)rRawString(sizeof(int64_t)) }
  Store { *(int64_t*)wRawString(sizeof(int64_t)) = *value }
}


####################
#  Unsigned types  #
####################

AttributeType UInt8 : uint8_t {
  Load  { attr->value = rNumber }
  Store { wNumber(*value) }
}

AttributeType UInt16 : uint16_t {
  Load  { attr->value = rNumber }
  Store { wNumber(*value) }
}

AttributeType UInt32 : uint32_t {
  Load  { attr->value = rNumber }
  Store { wNumber(*value) }
}

AttributeType UInt64 : uint64_t {
  Load  { attr->value = rNumber }
  Store { wNumber(*value) }
}


####################
#  Floating types  #
####################

AttributeType Float : float {
  Load  { attr->value = *(float*)rRawString(sizeof(float)) }
  Store { *(float*)wRawString(sizeof(float)) = *value }
}

AttributeType Double : double {
  Load  { attr->value = *(double*)rRawString(sizeof(double)) }
  Store { *(double*)wRawString(sizeof(int32_t)) = *value }
}

AttributeType LDouble : long double {
  Load  { attr->value = *(long double*)rRawString(sizeof(long double)) }
  Store { *(long double*)wRawString(sizeof(long double)) = *value }
}


###################
#   (Raw)String   #
###################

AttributeType String : const char* {
  Destroy { generic_allocator_free(allocator, (void*)attr->value, end_time) }

  Write {
    if (&attr->value != value) {
      _tr_memory_free(H, (void*)attr->value);
      if (*value) {
        size_t len = strlen(*value) + 1;
        char *tmp = _tr_memory_alloc(H, len);
        memcpy(tmp, *value, len);
        attr->value = tmp;
      } else attr->value = 0;
    }
  }

  Load {
    size_t length;
    const void *ptr;
    if (!read_string(R, &ptr, &length)) return false;
    char *tmp = 0;
    if (length) {
      tmp = generic_allocator_alloc(allocator, length + 1);
      memcpy(tmp, ptr, length);
      tmp[length] = '\0';
    }
    if (attr->value) generic_allocator_free(allocator, (void*)attr->value, 0);
    attr->value = tmp;
  }

  Store { wString(*value ?: "") }
}

AttributeType RawString : const struct RawString* {
  Destroy { generic_allocator_free(allocator, (void*)attr->value, end_time) }

  Write {
    if (&attr->value != value) {
      _tr_memory_free(H, (void*)attr->value);
      if (*value) {
        struct RawString *tmp = _tr_memory_alloc(H,
            sizeof(struct RawString) + value[0]->length);
        memcpy(tmp, value, sizeof(struct RawString) + value[0]->length);
        attr->value = tmp;
      } else attr->value = 0;
    }
  }

  Load {
    size_t length;
    const void *ptr;
    if (!read_string(R, &ptr, &length)) return false;
    if (attr->value) generic_allocator_free(allocator, (void*)attr->value, 0);
    if (length) {
      struct RawString *tmp = generic_allocator_alloc(allocator, sizeof(struct RawString) + length);
      tmp->length = length;
      memcpy(tmp->data, ptr, length);
      attr->value = tmp;
    } else attr->value = 0;
  }

  Store {
    if (value[0]->length) {
      memcpy(wRawString(value[0]->length), value[0]->data, value[0]->length);
    } else wNumber(0);
  }
}


###################
#      Node       #
###################

AttributeType Pointer : union { Node *value; uint64_t id; } {
  Init    { attr->id = 0; attr->value = 0; }
  Destroy { attr->value = 0 }

  Write {
    if (attr->value) {
      if (!_node_ref_count_decrease(H, attr->value)) return false;
    }
    if (value) {
      if (!_node_ref_count_increase(H, *value)) return false;
    }
    attr->value = *value;
  }

  Load { attr->id = rNumber }
  Store { wNumber(*value ? value[0]->id : 0) }
}


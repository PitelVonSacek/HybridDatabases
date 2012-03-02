#ifndef __ATTRIBUTES_H__
#define __ATTRIBUTES_H__

#include "attributes_defs.h"

#include "../handler.h"
#include "../database_types.h"

static inline size_t attribute_size(int type);
static inline bool attribute_is_primitive(int type);

static inline void attribute_read(int type, const void *attr, void *buffer);

static inline void attribute_init(int type, void *attr);
static inline void attribute_destroy(int type, struct GenericAllocatorInfo *allocator,
                                     uint64_t end_time, void *attr);

// FIXME lock node when modifing its ref_count
static inline bool attribute_write(int type, Handler *H, 
                                   uint64_t end_time, void *dest, const void *src);

static inline void attribute_store(int type, Writer *W, void *attr);
static inline bool attribute_load(int type, Reader *R, 
                                  struct GenericAllocatorInfo *allocator, 
                                  void *attr);

#pragma push_macro("readFailed")
#undef readFailed
#define readFailed return false

#include "generated.h"

#pragma pop_macro("readFailed")

#endif


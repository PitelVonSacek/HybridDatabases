
// DefineAttrType(name, Internal_type, is_primitive
//                init, destroy,
//                write,
//                load, store)

#define UnsignedType(name, C_type) \
  DefineAttrType(name, struct { C_type value; }, StaticTrue, \
                 *(C_type*)attr = 0, (void)0, \
                 *(C_type*)dest = *(C_type*)src, \
                 *(C_type*)attr = rNumber, \
                 wNumber(*(C_type*)attr))

// FIXME dirty uses unaligned access
#define SignedType(name, C_type) \
  DefineAttrType(name, struct { C_type value; }, StaticTrue, \
                 *(C_type*)attr = 0, (void)0, \
                 *(C_type*)dest = *(C_type*)src, \
                 *(C_type*)attr = *(C_type*)(rRawString(sizeof(C_type))), \
                 *(C_type*)wRawString(sizeof(C_type)) = *(C_type*)attr)

UnsignedType(UInt8, uint8_t)
UnsignedType(UInt16, uint16_t)
UnsignedType(UInt32, uint32_t)
UnsignedType(UInt64, uint64_t)

SignedType(Int8, int8_t)
SignedType(Int16, int16_t)
SignedType(Int32, int32_t)
SignedType(Int64, int64_t)

SignedType(Float, float)
SignedType(Double, double)
SignedType(LDouble, long double)

DefineAttrType(String, struct { const char *value; }, StaticFalse,
  *(const char**)attr = 0,

  do { // destroy
    void *ptr = *(void**)attr; // no need to be atomic, no one has ptr to us
    *(const char**)attr = 0;
    generic_free(allocator, ptr, end_time);
  } while(0),

  do { // copy
    if (dest == src) break;
    const char* d = *(const char**)dest; // we have write lock
    generic_free(allocator, (void*)d, end_time);
    const char* s = *(const char**)src;
    size_t len = strlen(s) + 1;
    d = generic_alloc(allocator, len);
    memcpy((void*)d, s, len);
    *(const char**)dest = d;
  } while (0),

  do { // load
    size_t length;
    const void *ptr;
    if (!read_string(R, &ptr, &length)) readFailed;
    void *str = generic_alloc(allocator, length + 1);
    memcpy(str, ptr, length);
    ((char*)str)[length] = '\0';
    *(const char**)attr = str;
  } while (0),

  do { // strore
    wString(*(const char**)attr);
  } while (0)
)


DefineAttrType(RawString, struct { BinaryString *value; }, StaticFalse,
  *(RawString_value_t*)attr = 0,  
  
  do { // destroy
    void *ptr = *(void**)attr; // no need to be atomic, no one has ptr to us
    *(BinaryString**)attr = 0;
    generic_free(allocator, ptr, end_time);
  } while(0),

  do { // copy
    if (dest == src) break;
    BinaryString* d = *(BinaryString**)dest; // we have write lock
    generic_free(allocator, (void*)d, end_time);
    BinaryString* s = *(BinaryString**)src;
    d = generic_alloc(allocator, s->length + sizeof(*((BinaryString*)0)));
    memcpy(d, s, s->length + sizeof(*((BinaryString*)0)));
    *(BinaryString**)dest = d;
  } while (0),

  do { // load
    size_t length;
    const void *ptr;
    if (!read_string(R, &ptr, &length)) readFailed;
    BinaryString* str = generic_alloc(allocator, length + sizeof(*((BinaryString*)0)));
    memcpy(str->data, ptr, length);
    str->length = length;
    *(BinaryString**)attr = str;
  } while (0),

  do { // strore
    BinaryString* str = *(BinaryString**)attr;
    memcpy(wRawString(str->length), str->data, str->length);
  } while (0)
)


DefineAttrType(Pointer, union { Node* value; uint64_t id; }, StaticFalse,
  *(uint64_t*)attr = 0,
  (void)0,

  do { // copy
    Node *_src = *(Node**)src;
    if (!_node_ref_count_decrease(*(Node**)dest) ||
        !_node_ref_count_increase(_src)) return false;
    *(Node**)dest = _src;
  } while (0),
  
  *(uint64_t*)attr = rNumber, //load

  do { // store
    Node *n = *(Node**)attr;
    wNumber(n ? n->id : 0);
  } while (0)
)

#undef SignedType
#undef UnsignedType


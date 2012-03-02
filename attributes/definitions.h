
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
  *(String_t*)attr = 0,

  do { // destroy
    void *ptr = *(void**)attr; // no need to be atomic, no one has ptr to us
    *(String_t*)attr = 0;
    generic_free(allocator, ptr, end_time);
  } while(0),

  do { // copy
    if (dest == src) break;
    String_t d = *(String_t*)dest; // we have write lock
    generic_free(allocator, (void*)d, end_time);
    String_t s = *(String_t*)src;
    size_t len = strlen(s) + 1;
    d = generic_alloc(allocator, len);
    memcpy((void*)d, s, len);
    *(String_t*)dest = d;
  } while (0),

  do { // load
    size_t length;
    const void *ptr;
    if (!read_string(R, &ptr, &length)) readFailed;
    void *str = generic_alloc(allocator, length + 1);
    memcpy(str, ptr, length);
    ((char*)str)[length] = '\0';
    *(String_t*)attr = str;
  } while (0),

  do { // strore
    wString(*(String_t*)attr);
  } while (0)
)


DefineAttrType(RawString, struct { BinaryString *value; }, StaticFalse,
  *(RawString_t*)attr = 0,  
  
  do { // destroy
    void *ptr = *(void**)attr; // no need to be atomic, no one has ptr to us
    *(RawString_t*)attr = 0;
    generic_free(allocator, ptr, end_time);
  } while(0),

  do { // copy
    if (dest == src) break;
    RawString_t d = *(RawString_t*)dest; // we have write lock
    generic_free(allocator, (void*)d, end_time);
    RawString_t s = *(RawString_t*)src;
    d = generic_alloc(allocator, s->length + sizeof(*((RawString_t)0)));
    memcpy(d, s, s->length + sizeof(*((RawString_t)0)));
    *(RawString_t*)dest = d;
  } while (0),

  do { // load
    size_t length;
    const void *ptr;
    if (!read_string(R, &ptr, &length)) readFailed;
    RawString_t str = generic_alloc(allocator, length + sizeof(*((RawString_t)0)));
    memcpy(str->data, ptr, length);
    str->length = length;
    *(RawString_t*)attr = str;
  } while (0),

  do { // strore
    RawString_t str = *(RawString_t*)attr;
    memcpy(wRawString(str->length), str->data, str->length);
  } while (0)
)


DefineAttrType(Pointer, union { Node* value; uint64_t id; }, StaticFalse,
  *(Node**)attr = 0,
  (void)0,

  do { // copy
    Node *_src = *(Node**)src;
    if (!_node_ref_count_decrease(H, *(Node**)dest) ||
        !_node_ref_count_increase(H, _src)) return false;
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


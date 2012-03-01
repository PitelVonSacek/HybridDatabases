#define trRead(...) trRead_(H, __VA_ARGS__)
#define trWrite(...) trWrite_(H, __VA_ARGS__)
#define trCheck(...) trCheck_(H, __VA_ARGS__)

#define trNodeCreate(...) trNodeCreate_(H, __VA_ARGS__)
#define trNodeDelete(...) trNodeDelete_(H, __VA_ARGS__)


#define trUnsafeRead_(H, node, AttrName) \
  ((const (typeof((node)->AttrName.value)))((node)->AttrName.value))

#define trUnsafeWrite_(H, node, AttrName, value) \
  ({ \
    typeof(value) __value = (value);
    
  })

#define trRead_(H, node, AttrName) \
  ({ \
    typeof(node) __node = (node);
    bit_array_set(H->read_set.read_set, hash_ptr(__node)); \
    trUnsafeRead_(H, __node, AttrName); \
  })

#define trWrite(H, node, AttrName, value)



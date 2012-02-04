# 1 "attributes_macro_hell_code.h"
# 1 "<built-in>"
# 1 "<command-line>"
# 1 "attributes_macro_hell_code.h"
# 10 "attributes_macro_hell_code.h"
# 1 "attributes_macro_hell_data.h" 1


@define AttrLoad(Attr, ...) __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), Int_t), AttrType__Int__load(ARGS), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), Int8_t), AttrType__Int8__load(ARGS), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), Int16_t), AttrType__Int16__load(ARGS), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), Int64_t), AttrType__Int64__load(ARGS), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), UInt_t), AttrType__UInt__load(ARGS), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), UInt8_t), AttrType__UInt8__load(ARGS), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), UInt16_t), AttrType__UInt16__load(ARGS), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), UInt64_t), AttrType__UInt64__load(ARGS), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), Float_t), AttrType__Float__load(ARGS), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), Double_t), AttrType__Double__load(ARGS), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), LDouble_t), AttrType__LDouble__load(ARGS), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), Pointer_t), AttrType__Pointer__load(ARGS), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), String_t), AttrType__String__load(ARGS), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), BinaryString_t), AttrType__BinaryString__load(ARGS), __builtin_trap() ) ) ) ) ) ) ) ) ) ) ) ) ) )
# 11 "attributes_macro_hell_code.h" 2
# 19 "attributes_macro_hell_code.h"
# 1 "attributes_macro_hell_data.h" 1


@define AttrStore(Attr, W, p) __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), Int_t), AttrType__Int__store(W, (void*)(p)), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), Int8_t), AttrType__Int8__store(W, (void*)(p)), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), Int16_t), AttrType__Int16__store(W, (void*)(p)), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), Int64_t), AttrType__Int64__store(W, (void*)(p)), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), UInt_t), AttrType__UInt__store(W, (void*)(p)), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), UInt8_t), AttrType__UInt8__store(W, (void*)(p)), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), UInt16_t), AttrType__UInt16__store(W, (void*)(p)), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), UInt64_t), AttrType__UInt64__store(W, (void*)(p)), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), Float_t), AttrType__Float__store(W, (void*)(p)), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), Double_t), AttrType__Double__store(W, (void*)(p)), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), LDouble_t), AttrType__LDouble__store(W, (void*)(p)), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), Pointer_t), AttrType__Pointer__store(W, (void*)(p)), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), String_t), AttrType__String__store(W, (void*)(p)), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), BinaryString_t), AttrType__BinaryString__store(W, (void*)(p)), __builtin_trap() ) ) ) ) ) ) ) ) ) ) ) ) ) )
# 20 "attributes_macro_hell_code.h" 2
# 28 "attributes_macro_hell_code.h"
# 1 "attributes_macro_hell_data.h" 1


@define AttrCopy(Attr, tr, dest, src) __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), Int_t), AttrType__Int__copy(tr, (void*)(dest), (const void*)(src)), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), Int8_t), AttrType__Int8__copy(tr, (void*)(dest), (const void*)(src)), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), Int16_t), AttrType__Int16__copy(tr, (void*)(dest), (const void*)(src)), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), Int64_t), AttrType__Int64__copy(tr, (void*)(dest), (const void*)(src)), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), UInt_t), AttrType__UInt__copy(tr, (void*)(dest), (const void*)(src)), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), UInt8_t), AttrType__UInt8__copy(tr, (void*)(dest), (const void*)(src)), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), UInt16_t), AttrType__UInt16__copy(tr, (void*)(dest), (const void*)(src)), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), UInt64_t), AttrType__UInt64__copy(tr, (void*)(dest), (const void*)(src)), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), Float_t), AttrType__Float__copy(tr, (void*)(dest), (const void*)(src)), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), Double_t), AttrType__Double__copy(tr, (void*)(dest), (const void*)(src)), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), LDouble_t), AttrType__LDouble__copy(tr, (void*)(dest), (const void*)(src)), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), Pointer_t), AttrType__Pointer__copy(tr, (void*)(dest), (const void*)(src)), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), String_t), AttrType__String__copy(tr, (void*)(dest), (const void*)(src)), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), BinaryString_t), AttrType__BinaryString__copy(tr, (void*)(dest), (const void*)(src)), __builtin_trap() ) ) ) ) ) ) ) ) ) ) ) ) ) )
# 29 "attributes_macro_hell_code.h" 2
# 37 "attributes_macro_hell_code.h"
# 1 "attributes_macro_hell_data.h" 1


@define AttrInit(Attr, ...) __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), Int_t), AttrType__Int__init(ARGS), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), Int8_t), AttrType__Int8__init(ARGS), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), Int16_t), AttrType__Int16__init(ARGS), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), Int64_t), AttrType__Int64__init(ARGS), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), UInt_t), AttrType__UInt__init(ARGS), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), UInt8_t), AttrType__UInt8__init(ARGS), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), UInt16_t), AttrType__UInt16__init(ARGS), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), UInt64_t), AttrType__UInt64__init(ARGS), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), Float_t), AttrType__Float__init(ARGS), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), Double_t), AttrType__Double__init(ARGS), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), LDouble_t), AttrType__LDouble__init(ARGS), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), Pointer_t), AttrType__Pointer__init(ARGS), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), String_t), AttrType__String__init(ARGS), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), BinaryString_t), AttrType__BinaryString__init(ARGS), __builtin_trap() ) ) ) ) ) ) ) ) ) ) ) ) ) )
# 38 "attributes_macro_hell_code.h" 2
# 46 "attributes_macro_hell_code.h"
# 1 "attributes_macro_hell_data.h" 1


@define AttrDestroy(Attr, ...) __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), Int_t), AttrType__Int__destroy(ARGS), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), Int8_t), AttrType__Int8__destroy(ARGS), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), Int16_t), AttrType__Int16__destroy(ARGS), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), Int64_t), AttrType__Int64__destroy(ARGS), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), UInt_t), AttrType__UInt__destroy(ARGS), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), UInt8_t), AttrType__UInt8__destroy(ARGS), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), UInt16_t), AttrType__UInt16__destroy(ARGS), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), UInt64_t), AttrType__UInt64__destroy(ARGS), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), Float_t), AttrType__Float__destroy(ARGS), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), Double_t), AttrType__Double__destroy(ARGS), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), LDouble_t), AttrType__LDouble__destroy(ARGS), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), Pointer_t), AttrType__Pointer__destroy(ARGS), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), String_t), AttrType__String__destroy(ARGS), __builtin_choose_expr(__builtin_types_compatible_p(typeof(Attr), BinaryString_t), AttrType__BinaryString__destroy(ARGS), __builtin_trap() ) ) ) ) ) ) ) ) ) ) ) ) ) )
# 47 "attributes_macro_hell_code.h" 2
# 59 "attributes_macro_hell_code.h"
# 1 "attributes_macro_hell_data.h" 1


typedef union { Int Int__item; Int8 Int8__item; Int16 Int16__item; Int64 Int64__item; UInt UInt__item; UInt8 UInt8__item; UInt16 UInt16__item; UInt64 UInt64__item; Float Float__item; Double Double__item; LDouble LDouble__item; Pointer Pointer__item; String String__item; BinaryString BinaryString__item; } UniversalAttribute;
# 60 "attributes_macro_hell_code.h" 2







enum {
# 1 "attributes_macro_hell_data.h" 1


AttrType__Int__id, AttrType__Int8__id, AttrType__Int16__id, AttrType__Int64__id, AttrType__UInt__id, AttrType__UInt8__id, AttrType__UInt16__id, AttrType__UInt64__id, AttrType__Float__id, AttrType__Double__id, AttrType__LDouble__id, AttrType__Pointer__id, AttrType__String__id, AttrType__BinaryString__id,
# 69 "attributes_macro_hell_code.h" 2
  AttrType__types_count
};

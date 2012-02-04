// input
// #define NAME pokusny_uzel
// #define ATTRS ATTR(cislo, Int) ATTR(text, String)

#if !defined(NAME) || !defined(ATTRS)
# error "NAME or ATTRS macro not defined !!"
#endif

#define NT__(a, b) NodeType__##a##b
#define NT_(a, b) NT__(a, b)
#define NT(s) NT_(NAME, s)
#define S_(a) #a
#define S(a) S_(a)

#define ATTR(name, type) struct NodeAttribute name;
typedef struct {
  NodeType __type;
  ATTRS
  char __dummy[0];
} NT(__desc_t);
#undef ATTR

extern const NT(__desc_t) NT(__desc)[1];

#define ATTR(name, type) type##_t name;
typedef struct {
  union {
    Node __node;
    const NT(__desc_t) *__type;
  };
  ATTRS
} NT();
#undef ATTR

#ifdef IMPLEMENT_NODES

/*
  Part of NodeType:
void (*load)(Reader*, Node*);
void (*store)(Writer*, Node*);

void (*init_pointers)(IdToNode*, Node*);

// changes all pointers in node to 0, required for deleting node
int (*destroy_pointers)(Transaction*, Node*);

void (*init)(Node*);
void (*destroy)(Node*);
*/


#define ATTR(name, type) NT(__attribute__##name),
enum {
  ATTRS
  NT(__attributes_count)
};
#undef ATTR


#define ATTR(name, type) AttrType__##type##__load(R, &node->name.value);
static void NT(__load) (Reader* R, NT()* node) {
  node->__node._prev = node->__node._next = 0;
  node->__node.ref_count = 0;

  ATTRS
}
#undef ATTR

#define ATTR(name, type) AttrType__##type##__store(W, &node->name.value);
static void NT(__store) (Writer* W, NT()* node) {
  W_NUMBER(node->__node.type->id);
  W_NUMBER(node->__node.id);

  ATTRS
}
#undef ATTR


#define ATTR(name, type) AttrType__##type##__init(&node->name.value);
static void NT(__init) (NT()* node) {
  {
    Node *n = (Node*)node;
    n->_prev = n->_next = 0;
    n->ref_count = 0;
  }

  ATTRS
}
#undef ATTR

#define ATTR(name, type) AttrType__##type##__destroy(&node->name.value);
static void NT(__destroy) (NT()* node) {
  ATTRS
}
#undef ATTR


#define ATTR(name, type) \
  __builtin_choose_expr(__builtin_types_compatible_p(Pointer_t, type##_t), \
  ({ \
    uint64_t *id = (uint64_t*)&(node->name); \
    if (*id) { \
      *(Node**)&(node->name.value) = dict_at(dict, *id); \
      (*(Node**)&(node->name.value))->ref_count++; \
    } else *((void**)&node->name.value) = (void*)0; \
    (void)0; \
  }), (void)0);
static void NT(__init_pointers) (IdToNode* dict, NT()* node) {
  ATTRS
}
#undef ATTR

#define ATTR(name, type) \
  __builtin_choose_expr(__builtin_types_compatible_p(Pointer_t, type##_t), \
  TrWrite_(tr, node, name, 0), 1) &&
static int NT(__destroy_pointers) (Transaction* tr, NT()* node) {
  if (ATTRS 1) return TRERR_SUCCESS;
  return TRERR_COLLISION;
}
#undef ATTR


#define ATTR(name, type) { #name, AttrType__##type, NT(__attribute__##name), __builtin_offsetof(NT(), name) },
const NT(__desc_t) NT(__desc)[1] = {{
  {
    .name = S(NAME),
    .my_size = sizeof(NT(__desc_t)),
    .size = sizeof(NT()),

    .load = (void(*)(Reader*,Node*))&NT(__load),
    .store = (void(*)(Writer*,Node*))&NT(__store),

    .init_pointers = (void(*)(IdToNode*,Node*))&NT(__init_pointers),

    .destroy_pointers = (int(*)(Transaction*,Node*))&NT(__destroy_pointers),

    .init = (void(*)(Node*))&NT(__init),
    .destroy = (void(*)(Node*))&NT(__destroy),

    .attributes_count = NT(__attributes_count)
  },
  ATTRS
  {}
}};
#undef ATTR


#endif

#undef S
#undef S_
#undef NT
#undef NT_
#undef NT__
#undef ATTRS
#undef NAME

# 1 "node_test.h"
# 1 "<built-in>"
# 1 "<command-line>"
# 1 "node_test.h"
# 10 "node_test.h"
# 1 "nodetype_generate.h" 1
# 12 "nodetype_generate.h"
typedef struct {
  NodeType __base;
  struct NodeAttribute textik; struct NodeAttribute cisilko; struct NodeAttribute cosi;
  char __dummy[0];
} NodeType__pokus__desc_t;
extern const NodeType__pokus__desc_t NodeType__pokus__desc[1];




typedef struct {
  Node __base;
  String_t textik; Int_t cisilko; Pointer_t cosi;
} NodeType__pokus;
# 47 "nodetype_generate.h"
enum {
  NodeType__pokus__attribute__textik, NodeType__pokus__attribute__cisilko, NodeType__pokus__attribute__cosi,
  NodeType__pokus__attributes_count
};




static void NodeType__pokus__load (Reader* R, NodeType__pokus* node) {
  node->_prev = node->_next = 0;
  node->ref_count = 0;

  AttrType__String__load(R, &node->textik); AttrType__Int__load(R, &node->cisilko); AttrType__Pointer__load(R, &node->cosi);
}



static void NodeType__pokus__store (Writer* W, NodeType__pokus* node) {
  W_NUMBER(node->__type->id);
  W_NUMBER(node->__base.id);

  AttrType__String__store(W, &node->textik); AttrType__Int__store(W, &node->cisilko); AttrType__Pointer__store(W, &node->cosi);
}




static void NodeType__pokus__init (NodeType__pokus* node) {
  {
    Node *n = (Node*)node;
    n->__prev = n->__next = 0;
    n->ref_count = 0;
  }

  AttrType__String__init(&node->textik); AttrType__Int__init(&node->cisilko); AttrType__Pointer__init(&node->cosi);
}



static void NodeType__pokus__destroy (NodeType__pokus* node) {
  AttrType__String__destroy(&node->textik); AttrType__Int__destroy(&node->cisilko); AttrType__Pointer__destroy(&node->cosi);
}
# 100 "nodetype_generate.h"
static void NodeType__pokus__init_pointers (IdToNode* dict, NodeType__pokus* node) {
  __builtin_choose_expr(__builtin_types_compatible_p(Pointer_t, String_t), ({ if (node->textik.id) { node->textik.value = dict_at(dict, node->textik.id); node->textik.value->ref_count++; } else node->textik.value = 0; }), (void)0); __builtin_choose_expr(__builtin_types_compatible_p(Pointer_t, Int_t), ({ if (node->cisilko.id) { node->cisilko.value = dict_at(dict, node->cisilko.id); node->cisilko.value->ref_count++; } else node->cisilko.value = 0; }), (void)0); __builtin_choose_expr(__builtin_types_compatible_p(Pointer_t, Pointer_t), ({ if (node->cosi.id) { node->cosi.value = dict_at(dict, node->cosi.id); node->cosi.value->ref_count++; } else node->cosi.value = 0; }), (void)0);
}





static int NodeType__pokus__destroy_pointers (Transaction* _transaction, NodeType__pokus* node) {
  if (__builtin_choose_expr(__builtin_types_compatible_p(Pointer_t, String_t), TrWrite(node, textik, 0), 1) && __builtin_choose_expr(__builtin_types_compatible_p(Pointer_t, Int_t), TrWrite(node, cisilko, 0), 1) && __builtin_choose_expr(__builtin_types_compatible_p(Pointer_t, Pointer_t), TrWrite(node, cosi, 0), 1) && 1) return TRERR_SUCCESS;
  return TRERR_COLLISION;
}



/*
const NodeType__pokus__desc_t NodeType__pokus__desc[1] = {{
  {
    .name = "pokus",
    .my_size = sizeof(NodeType__pokus__desc_t),
    .size = sizeof(NodeType__pokus),

    .load = (void(*)(Reader*,Node*))&NodeType__pokus__load,
    .store = (void(*)(Writer*,Node*))&NodeType__pokus__store,

    .init_pointers = (void(*)(IdToNode*,Node*))&NodeType__pokus__init_pointers,

    .destroy_pointers = (int(*)(Transaction*,Node*))&NodeType__pokus__destroy_pointers,

    .init = (void(*)(Node*))&NodeType__pokus__init,
    .destroy = (void(*)(Node*))&NodeType__pokus__destroy,

    .attributes_count = NodeType__pokus__attributes_count
  },
  { "textik", AttrType__String, NodeType__pokus__attribute__textik, __builtin_offsetof(NodeType__pokus, textik) }, { "cisilko", AttrType__Int, NodeType__pokus__attribute__cisilko, __builtin_offsetof(NodeType__pokus, cisilko) }, { "cosi", AttrType__Pointer, NodeType__pokus__attribute__cosi, __builtin_offsetof(NodeType__pokus, cosi) },
  {}
}};
# 10 "node_test.h" 2

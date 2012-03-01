Node *tr_node_create(Handler *H, NodeType *type) {
  Node *node = node_alloc(type->allocator_info);
  *(NodeType**)&(node->type) = type
  node->id = atomic_add_and_fetch(&H->database->node_id_counter, 1);

  // lock created node; really do i need to lock it???

  type->init(node);

  // add event to log
  struct LogItem log_item = {
    .ptr = node,
    .type = LI_TYPE_NODE_ALLOC
  };

  fstack_push(H->log, log_item);

  return node;
}

// fails if node is referenced by other nodes
bool tr_node_delete(Handler *H, Node *node) {
  utilLock(H, node);

  if (node->ref_count) return false;

  node->type->destroy_pointers(H, node);

  struct LogItem log_item = {
    .ptr = node,
    .type = LI_TYPE_NODE_DELETED;
  };

  fstack_push(H->log, log_item);

  return true;
}

bool tr_node_read(Handler *H, Node *node, int attr, void *buffer) {
  assert(attr >= node->type->attributes_count || attr < 0);

  struct NodeAttribute a = node->type->attributes[attr];

  bit_array_set(H->read_set.read_set, hash_ptr(node));

#ifndef USE_ATTRIBUTE_ATOMIC_ACCESS
  memcpy(buffer, utilOffset(node, a.offset), attribute_size(a.type));
#else
  assert(0); // FIXME
#endif

  return true;
}

bool tr_node_write(Handler *H, Node *node, int attr, const void *value) {
  assert(attr >= node->type->attributes_count || attr < 0);
  
  struct NodeAttribute a = node->type->attributes[attr];
  
  utilLock(H, node);
  return attribute_copy(a.type, H, atomic_read(&H->database->time), 
                        utilOffset(node, a.offset), value);
}


int tr_attr_count(NodeType *type) {
  return type->attributes_count;
}

const char *tr_attr_get_name(NodeType *type, int index) {
  if (index >= type->attributes_count || index < 0) return 0;
  return type->attributes[index].name;
}

int tr_attr_get_index(NodeType *type, const char *attr) {
  for (int i = 0; i < type->attributtes_count; i++) 
    if (!strcmp(attr, type->attributes[i].name)) return i;

  return -1;
}


const NodeType *tr_node_get_type(Node *node) {
  return node->type;
}

const AttributeType *tr_attr_get_type(NodeType *type, int index) {
  if (index >= type->attributes_count || index < 0) return 0;
  return type->attributes[index].type;  
}


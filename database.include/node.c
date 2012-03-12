Node *tr_node_create (Handler *H, NodeType *type) {
  NodeType *types = H->database->node_types;
  size_t types_count = H->database->node_types_count;
  ptrdiff_t diff = type - types;
  
  if (diff < 0 || diff >= types_count) {
    // type if not in database->node_types, so we must find equivalent one there
    // FIXME correct but very stupid algorithm
    for (size_t i = 0; i < types_count; i++)
      if (types[i].name == type->name) {
        type = types + i;
        goto found;
      }
  
    return 0;
    found: ;
  }

  Node *node = node_alloc(type->allocator_info);
  *(NodeType**)&(node->type) = type;
  *(uint64_t*)&node->id = atomic_add_and_fetch(&H->database->node_id_counter, 1);
  node->ref_count = 0;
  list_init_head(&node->__list);

  // lock created node; really do i need to lock it???
  if (!utilLock(H, node)) {
    node_free(type->allocator_info, node, atomic_read(&H->database->time));
    return 0;
  }

  type->init(node);

  // add event to log
  struct LogItem log_item = {
    .ptr = node,
    .type = LI_TYPE_NODE_ALLOC
  };

  fstack_push(H->log, log_item);

  if (!node->type->update_indexes(H, CBE_NODE_CREATED, node)) 
    return 0;

  return node;
}

// fails if node is referenced by other nodes
bool tr_node_delete (Handler *H, Node *node) {
  if (!utilLock(H, node)) return false;

  if (node->ref_count) return false;

  if (!node->type->update_indexes(H, CBE_NODE_DELETED, node))
    return false;

  node->type->destroy_pointers(H, node);

  struct LogItem log_item = {
    .ptr = node,
    .type = LI_TYPE_NODE_DELETE
  };

  fstack_push(H->log, log_item);

  return true;
}

bool tr_node_read (Handler *H, Node *node, int attr, void *buffer) {
  assert(attr >= node->type->attributes_count || attr < 0);

  struct NodeAttribute a = node->type->attributes[attr];

  bit_array_set(&H->read_set, hash_ptr(node));

  memcpy(buffer, utilOffset(node, a.offset), attribute_size(a.type));

  return tr_node_check(H, node);
}

bool tr_node_write (Handler *H, Node *node, int attr, const void *value) {
  assert(attr >= node->type->attributes_count || attr < 0);
  
  struct NodeAttribute a = node->type->attributes[attr];
      
  struct LogItem log_item = {
    .ptr = node,
    .type = LI_TYPE_NODE_MODIFY,
    .size = attribute_size(a.type),
    .index = attr,
    .offset = a.offset,
    .attr_type = a.type
  };

  memcpy(log_item.data_old, utilOffset(node, a.offset), attribute_size(a.type));

  if (!utilLock(H, node)) return false;
  if (!attribute_write(a.type, H, utilOffset(node, a.offset), value))
    return false;

  memcpy(log_item.data_new, utilOffset(node, a.offset), attribute_size(a.type));

  fstack_push(H->log, log_item);

  return true;
}


int tr_attr_count(NodeType *type) {
  return type->attributes_count;
}

const char *tr_attr_get_name(NodeType *type, int index) {
  if (index >= type->attributes_count || index < 0) return 0;
  return type->attributes[index].name;
}

int tr_attr_get_index(NodeType *type, const char *attr) {
  for (int i = 0; i < type->attributes_count; i++) 
    if (!strcmp(attr, type->attributes[i].name)) return i;

  return -1;
}


const NodeType *tr_node_get_type (Node *node) {
  return node->type;
}

const int tr_attr_get_type(NodeType *type, int index) {
  if (index >= type->attributes_count || index < 0) return 0;
  return type->attributes[index].type;  
}


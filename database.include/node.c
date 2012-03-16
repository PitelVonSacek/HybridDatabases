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

  Node *node = node_allocator_alloc(type->allocator);

  if (!utilLock(H, node)) {
    node_allocator_free(type->allocator, node, atomic_read(&H->database->time));
    return 0;
  }

  uint64_t id = atomic_add_and_fetch(&H->database->node_id_counter, 1);
  atomic_write((uint64_t*)&node->id, id);
  node->ref_count = 0;
  type->init(node);

  // add event to log
  struct LogItem log_item = {
    .ptr = node,
    .type = LI_TYPE_NODE_ALLOC
  };

  fstack_push(H->log, log_item);

  if (!node_get_type(node)->update_indexes(H, CBE_NODE_CREATED, node)) 
    return 0;

  return node;
}

// fails if node is referenced by other nodes
bool tr_node_delete (Handler *H, Node *node) {
  if (!utilLock(H, node)) return false;

  if (node->ref_count) return false;

  if (!node_get_type(node)->update_indexes(H, CBE_NODE_DELETED, node))
    return false;

  node_get_type(node)->destroy_pointers(H, node);

  struct LogItem log_item = {
    .ptr = node,
    .type = LI_TYPE_NODE_DELETE
  };

  fstack_push(H->log, log_item);

  return true;
}

bool tr_node_read (Handler *H, Node *node, int attr, void *buffer) {
  assert(attr >= node_get_type(node)->attributes_count || attr < 0);

  struct NodeAttribute a = node_get_type(node)->attributes[attr];

  bit_array_set(&H->read_set, hash_ptr(node));

  memcpy(buffer, util_apply_offset(node, a.offset), attribute_size(a.type));

  return tr_node_check(H, node);
}

bool tr_node_write (Handler *H, Node *node, int attr, const void *value) {
  assert(attr >= node_get_type(node)->attributes_count || attr < 0);
  
  struct NodeAttribute a = node_get_type(node)->attributes[attr];
      
  struct LogItem log_item = {
    .ptr = node,
    .type = LI_TYPE_NODE_MODIFY,
    .size = attribute_size(a.type),
    .index = attr,
    .offset = a.offset,
    .attr_type = a.type
  };

  memcpy(log_item.data_old, util_apply_offset(node, a.offset), attribute_size(a.type));

  if (!utilLock(H, node)) return false;
  if (!attribute_write(a.type, H, util_apply_offset(node, a.offset), value))
    return false;

  memcpy(log_item.data_new, util_apply_offset(node, a.offset), attribute_size(a.type));

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
  return node_get_type(node);
}

const int tr_attr_get_type(NodeType *type, int index) {
  if (index >= type->attributes_count || index < 0) return 0;
  return type->attributes[index].type;  
}

static void node_offset_check() {
  STATIC_ASSERT(sizeof(struct SList) == sizeof(((Node*)0)->id));
  STATIC_ASSERT(sizeof(struct NodeAllocatorBlock ) % __alignof__(Node) == 0);
}

static bool _node_get_next_step(NodeType *type, struct NodeAllocatorBlock **block,
                               Node **node) {
  const size_t max_offset =
    ((PAGE_ALLOCATOR_PAGE_SIZE - sizeof(struct NodeAllocatorBlock))
    / type->size) * type->size;

  *node = util_apply_offset((*node), type->size);

  if (util_get_offset(block[0]->data, *node) >= max_offset) {
    pthread_mutex_lock(&type->allocator->mutex);
    *block = utilContainerOf(block[0]->dump_head.next,
                             struct NodeAllocatorBlock, dump_head);
    pthread_mutex_unlock(&type->allocator->mutex);

    if (&block[0]->dump_head == &type->allocator->dump_blocks) return false;
    *node = (void*)block[0]->data;
  }

  return true;
}

static Node *node_get_first(NodeType *type) {
  struct NodeAllocatorBlock *block;

  pthread_mutex_lock(&type->allocator->mutex);
  block = utilContainerOf(type->allocator->dump_blocks.next,
                          struct NodeAllocatorBlock, dump_head);
  pthread_mutex_unlock(&type->allocator->mutex);

  if (&block->dump_head == &type->allocator->dump_blocks) return 0;

  Node *node = (void*)block->data;

  while (!atomic_read(&node->id))
    if (!_node_get_next_step(type, &block, &node)) return 0;

  return node;
}

static Node *node_get_next(NodeType *type, Node *node) {
  struct NodeAllocatorBlock *block = page_allocator_get_page(node);

  do {
    if (!_node_get_next_step(type, &block, &node)) return 0;
  } while (!atomic_read(&node->id));

  return node;
}


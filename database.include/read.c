#pragma push_macro("readFailed")
#undef readFailed

#define readFailed \
  do { \
    dbDebug(DB_INFO); \
    return false; \
  } while (0)

static bool read_schema(Reader *R, Database *D) {
  const DatabaseType *type = D->type;

  rBegin;
  rArray {
    rCheckString("HybridDatabase Database Schema");
    rCheckString(type->name);
    rCheckString(type->version);

    const NodeType *const* n_type = type->node_types;
    for (int i = 0; i < type->node_types_count; i++) rArray {
      rCheckString(n_type[i]->name);

      for (int e = 0; e < n_type[i]->attributes_count; e++) rArray {
        rCheckString(n_type[i]->attributes[e].name);
        rCheckNumber(n_type[i]->attributes[e].type);
      } rArrayEnd;

    } rArrayEnd;
  } rArrayEnd;

  rFinish(true);
  return true;
}

static bool read_file_header(Reader *R, Database *D, uint64_t *magic) {
  rBegin;
  rArray {
    rCheckString("HybridDatabase Data File");
    rCheckString(D->type->name);
    rCheckString(D->type->version);
  } rArrayEnd;
  rFinish(true);
  
  rBegin;
  *magic = rNumber;
  rFinish(false);

  return true;
}

static bool read_file_footer(Reader *R, uint64_t *magic) {
  // rBegin; no rBegin cause look-forward function does it
  rCheckString("END OF FILE");
  rFinish(false);

  rBegin;
  *magic = rNumber;
  rFinish(false);

  return true;
}

static bool read_dump_begin(Reader *R) {
  rCheckString("DUMP BEGIN");
  rFinish(0);
  return true;
}

static bool read_dump_end(Reader *R) {
  rCheckString("DUMP END");
  rFinish(0);
  return true;
}

// FIXME
#define Ensure(expr) do { if (!(expr)) readFailed; } while (0)

static bool read_node_prepare(Reader *R, Database *D, IdToNode *nodes, 
                              NodeType **type, Node **node_) {
  size_t node_type_nr = rNumber;
  Ensure(node_type_nr < D->node_types_count);
  NodeType *node_type = &D->node_types[node_type_nr];
 
  Node *node = node_alloc(node_type->allocator_info);
  list_add_end(&D->node_list, &node->__list);

  *(uint64_t*)&node->id = rNumber;
  *(NodeType**)&node->type = node_type;
  node->ref_count = 0;

  assert(!ndict_get_node(nodes, node->id));
  
  if (node->id > D->node_id_counter) D->node_id_counter = node->id;

  ndict_insert(nodes, node->id, node);

  *type = node_type;
  *node_ = node;

  return true;
}

static bool read_node_load(Reader *R, Database *D, IdToNode *nodes) {
  NodeType *type;
  Node *node;
  return read_node_prepare(R, D, nodes, &type, &node) &&
         (type->init(node), true) &&
         type->load(R, D->tm_allocator, node);
}

static bool read_node_create(Reader *R, Database *D, IdToNode *nodes) {
  NodeType *type;
  Node *node;
  return read_node_prepare(R, D, nodes, &type, &node) && 
         (type->init(node), true);
}

static bool read_node_delete(Reader *R, Database *D, IdToNode *nodes) {
  uint64_t id = rNumber;
  typeof(ndict_get_node(nodes, id)) dict_node = ndict_get_node(nodes, id);

  if (!dict_node) return true;

  Node *node = dict_node->value;

  node->type->destroy(D->tm_allocator, node, 0);
  list_remove(&node->__list);
  node_free(node->type->allocator_info, node, 0);

  ndict_remove(nodes, id);

  return true;
}

static bool read_node_modify(Reader *R, Database *D, IdToNode *nodes) {
  uint64_t id = rNumber;
  int attr_id = rNumber;
  typeof(ndict_get_node(nodes, id)) dict_node = ndict_get_node(nodes, id);

  if (!dict_node) {
    rSkip;
    return true;
  }

  Node *node = dict_node->value;

  Ensure(attr_id < node->type->attributes_count);
  const struct NodeAttribute *attr = &node->type->attributes[attr_id];
  return attribute_load(attr->type, R, D->tm_allocator, utilOffset(node, attr->offset));
}

#undef Ensure

#pragma pop_macro("readFailed")


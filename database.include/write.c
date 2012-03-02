static void write_schema(Writer *W, Database *D) {
  const DatabaseType *type = D->type;

  wArray {
    wString("HybridDatabase Database Schema");
    wString(type->name);
    wString(type->version);

    NodeType *const* n_type = type->node_types;
    for (int i = 0; i < type->node_types_count; i++) wArray {
      wString(n_type[i]->name);

      for (int e = 0; e < n_type[i]->attributes_count; e++) wArray {
        wString(n_type[i]->attributes[i].name);
        wNumber(n_type[i]->attributes[i].type->id);
      } wArrayEnd;

    } wArrayEnd;
  } wArrayEnd;

  wFinish(true);
}


static void write_file_header(Writer *W, Database *D, uint64_t magic) {
  wArray {
    wString("HybridDatabase Data File");
    wString(D->type->name);
    wString(D->type->version);
  } wArrayEnd;
  wFinish(true);
  
  wNumber(magic_nr);
  wFinish(false);
}

static void write_file_footer(Writer *W, Database *D, uint64_t magic) {
  wString("END OF FILE");
  wFinish(false);

  wNumber(magic);
  wFinish(false);
}

static void write_dump_begin(Writer *W) {
  wString("DUMP BEGIN");
  wFinish(0);
}

static void write_dump_end(Writer *W) {
  wString("DUMP END");
  wFinish(0);
}

static void write_node_alloc(Writer *W, NodeType *type, uint64_t id) {
  wArray {
    wNumber(CBE_NODE_CREATED);
    wNumber(type->id);
    wNumber(node->id);
  } wArrayEnd;
}

static void write_node_delete(Writer *W, uint64_t id) {
  wArray {
    wNumber(CBE_NODE_DELETED);
    wNumber(id);
  } wArrayEnd;
}

static void write_node_modify(Writer *W, uint64_t node_id, unsigned attr, 
                              unsigned attr_type, const void *value) {
  wArray {
    wNumber(CBE_NODE_MODIFIED);
    wNumber(node_id);
    wNumber(attr);
    attribute_store(attr_type, W, value);
  } wArrayEnd;
}


static void write_log(Writer *W, TransactionLog *log) {
  wArray {
    fstack_for_each(item, log) {
      Node *node = (Node*)item->ptr;
      switch (item->type) {
        case LI_TYPE_NODE_MODIFY:
        case LI_TYPE_ATOMIC_NODE_MODIFY: 
          write_node_modify(W, node->id, item->index, 
                            item->attr_type, item->data_new);
          break;
        case LI_TYPE_NODE_ALLOC: 
          write_node_alloc(W, node->type, node->id);
          break;
        case LI_TYPE_NODE_DELETE:
          write_node_delete(W, node->id);
      }
    }
  } wArrayEnd;
  
  wFinish(1);
}


/**
 * @file
 * @brief Načítání dat ze souboru.
 *
 *//*
 * Implementované funkce:
 * static bool read_schema(Reader *R, Database *D);
 * static bool read_file_header(Reader *R, Database *D, uint64_t *magic);
 * static bool read_file_footer(Reader *R, uint64_t *magic);
 * static bool read_dump_begin(Reader *R);
 * static bool read_dump_end(Reader *R);
 */


// Předefinujeme makro readFailed, aby lépe vyhovovalo našim potřebám
// (pragma push uchová původní obsah makra, takže ho na konci můžeme obnovit)
#pragma push_macro("readFailed")
#undef readFailed

#define readFailed \
  do { \
    dbDebug(DB_INFO); \
    return false; \
  } while (0)

/// Zkontoluje schéma databáze.
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

/// Načte hlavičku datového souboru.
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

/// Načte značku "konec souboru".
static bool read_file_footer(Reader *R, uint64_t *magic) {
  // rBegin; no rBegin cause look-forward function does it
  rCheckString("END OF FILE");
  rFinish(false);

  rBegin;
  *magic = rNumber;
  rFinish(false);

  return true;
}

/// Načte značku "začátek výpisu databáze".
static bool read_dump_begin(Reader *R) {
  rCheckString("DUMP BEGIN");
  rFinish(0);
  return true;
}

/// Načte značku "konec výpisu databáze".
static bool read_dump_end(Reader *R) {
  rCheckString("DUMP END");
  rFinish(0);
  return true;
}


#define Ensure(expr) do { if (!(expr)) readFailed; } while (0)

/// Společná část funkcí read_node_load() a read_node_create().
static int read_node_prepare(Reader *R, Database *D, IdToNode *nodes,
                             NodeType **type, Node **node_) {
  size_t node_type_nr = rNumber;
  Ensure(node_type_nr < D->node_types_count);
  NodeType *node_type = &D->node_types[node_type_nr];
 
  Node *node = node_allocator_alloc(node_type->allocator);

  *(uint64_t*)&node->id = rNumber;
  node->ref_count = 0;

  if (ndict_get_node(nodes, node->id)) return -1;
  
  if (node->id > D->node_id_counter) D->node_id_counter = node->id;

  ndict_insert(nodes, node->id, node);

  *type = node_type;
  *node_ = node;

  return 1;
}

/// Načte uzel.
static bool read_node_load(Reader *R, Database *D, IdToNode *nodes) {
  NodeType *type;
  Node *node;
  return (read_node_prepare(R, D, nodes, &type, &node) == 1) &&
         (type->init(node), true) &&
         type->load(R, D->tm_allocator, node);
}

/// Načte operaci vytvoř uzel.
static bool read_node_create(Reader *R, Database *D, IdToNode *nodes) {
  NodeType *type;
  Node *node;

  switch (read_node_prepare(R, D, nodes, &type, &node)) {
    case -1:
      dbDebug(I, "Create request but node exits");
      return true;
    case 1:
      type->init(node);
      return true;
    default:
      return false;
  }
}

/// Načte operaci smaž uzel.
static bool read_node_delete(Reader *R, Database *D, IdToNode *nodes) {
  uint64_t id = rNumber;
  typeof(ndict_get_node(nodes, id)) dict_node = ndict_get_node(nodes, id);

  if (!dict_node) return true;

  Node *node = dict_node->value;

  node_get_type(node)->destroy(D->tm_allocator, node, 0);
  *(uint64_t*)&node->id = 0;
  node_allocator_free(node_get_type(node)->allocator, node, 0);

  ndict_remove(nodes, id);

  return true;
}

/// Načte operaci změň atribut uzlu.
static bool read_node_modify(Reader *R, Database *D, IdToNode *nodes) {
  uint64_t id = rNumber;
  int attr_id = rNumber;
  typeof(ndict_get_node(nodes, id)) dict_node = ndict_get_node(nodes, id);

  if (!dict_node) {
    rSkip;
    return true;
  }

  Node *node = dict_node->value;

  Ensure(attr_id < node_get_type(node)->attributes_count);
  const struct NodeAttribute *attr = &node_get_type(node)->attributes[attr_id];
  return attribute_load(attr->type, R, D->tm_allocator, util_apply_offset(node, attr->offset));
}

#undef Ensure

#pragma pop_macro("readFailed")


/**
 * @file
 * @brief Funkce pro zápis na disk.
 *
 * Jediné funkce, která zapisuje na disk a není v tomto souboru, jsou
 * do_dump() v databse.include/thread.c a
 * _database_new_file() v database.include/database.c.
 *
 *//*
 * Implementované funkce:
 * static void write_schema(Writer *W, Database *D);
 * static void write_file_header(Writer *W, Database *D, uint64_t magic);
 * static void write_file_footer(Writer *W, uint64_t magic);
 * static void write_dump_begin(Writer *W);
 * static void write_dump_end(Writer *W);
 * 
 * static void write_node_store(Writer *W, Node* node);
 * static void write_node_alloc(Writer *W, NodeType *type, uint64_t id);
 * static void write_node_delete(Writer *W, uint64_t id);
 * static void write_node_modify(Writer *W, uint64_t node_id, unsigned attr,
 *                               unsigned attr_type, const void *value);
 */

/// Zapíše schéma databáze.
static void write_schema(Writer *W, Database *D) {
  const DatabaseType *type = D->type;

  wArray {
    wString("HybridDatabase Database Schema");
    wString(type->name);
    wString(type->version);

    const NodeType *const* n_type = type->node_types;
    for (int i = 0; i < type->node_types_count; i++) wArray {
      wString(n_type[i]->name);

      for (int e = 0; e < n_type[i]->attributes_count; e++) wArray {
        wString(n_type[i]->attributes[e].name);
        wNumber(n_type[i]->attributes[e].type);
      } wArrayEnd;

    } wArrayEnd;
  } wArrayEnd;

  wFinish(true);
}

/// Zapíše hlavičku datového souboru.
static void write_file_header(Writer *W, Database *D, uint64_t magic_nr) {
  wArray {
    wString("HybridDatabase Data File");
    wString(D->type->name);
    wString(D->type->version);
  } wArrayEnd;
  wFinish(true);
  
  wNumber(magic_nr);
  wFinish(false);
}

static void write_file_footer(Writer *W, uint64_t magic) {
  wString("END OF FILE");
  wFinish(false);

  wNumber(magic);
  wFinish(false);
}

/// Zapíše značku "začátek výpisu databáze".
static void write_dump_begin(Writer *W) {
  wString("DUMP BEGIN");
  wFinish(0);
}

/// Zapíše snačku "konec výpisu databáze".
static void write_dump_end(Writer *W) {
  wString("DUMP END");
  wFinish(0);
}

/// Zapíše obsah uzly -- požívano při výpisu databáze.
static void write_node_store(Writer *W, Node *node) {
  wArray {
    wNumber(node_get_type(node)->id);
    wNumber(node->id);
    node_get_type(node)->store(W, node);
  } wArrayEnd;
}

/// Zapíše vytvoření uzlu.
static void write_node_alloc(Writer *W, NodeType *type, uint64_t id) {
  wArray {
    wNumber(CBE_NODE_CREATED);
    wNumber(type->id);
    wNumber(id);
  } wArrayEnd;
}

/// Zapíše smazání uzlu.
static void write_node_delete(Writer *W, uint64_t id) {
  wArray {
    wNumber(CBE_NODE_DELETED);
    wNumber(id);
  } wArrayEnd;
}

/// Zapíše změnu atributu uzlu.
static void write_node_modify(Writer *W, uint64_t node_id, unsigned attr, 
                              unsigned attr_type, const void *value) {
  wArray {
    wNumber(CBE_NODE_MODIFIED);
    wNumber(node_id);
    wNumber(attr);
    attribute_store(attr_type, W, value);
  } wArrayEnd;
}


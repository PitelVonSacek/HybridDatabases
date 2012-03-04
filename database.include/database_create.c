#include <dirent.h>

static int get_db_files(struct dirent*** files, const char* dir, const char* db_name);
static bool load_file(Database *D, Reader *R);
static bool load_data(Database *D);
static void fix_pointers(Database *D, IdToNode *nodes);
static void fill_indexies(Database *D);

Database *database_create (const DatabaseType *type, const char *file, unsigned flags) {
  Database *D = database_alloc(type);

  D->filename = file;
  D->flags = flags;

  if (!load_data(D)) goto error;

  fix_pointers(D);
  fill_indexies(D);

  if (pthread_create(&D->service_thread, 0, (void*(*)(void*))service_thread, (void*)D))
    goto error;

  return D;

  error:
  dbDebug(E, "Database creation failed");
  return 0;
}


static int get_db_files(struct dirent*** files, const char* dir, const char* db_name) {
  const size_t db_name_len = strlen(db_name);

  /*int db_files_sort(const struct dirent **a, const struct dirent **b) {
    for (int i = 0; db_name[i]; i++) if (db_name[i] != a[0]->d_name[i]) return 0;
    for (int i = 0; db_name[i]; i++) if (db_name[i] != b[0]->d_name[i]) return 0;

    long a_ = atol(a[0]->d_name + db_name_len);
    long b_ = atol(b[0]->d_name + db_name_len);

    if (a_ == b_) return 0;
    if (a_ > b_) return 1;
    return -1;
  }*/

  int db_files_select(const struct dirent *file) {
    int i;
    for (i = 0; db_name[i]; i++) if (db_name[i] != file->d_name[i]) return 0;

    while (1) switch (file->d_name[i]) {
      case '\0': return 1;
      case '0' ... '9': i++;
        continue;
      default: return 0;
    }
  }

  return scandir (dir, files, db_files_select, versionsort);
}


static void fix_pointers(Database *D, IdToNode *nodes) {
  ndictFor(var, nodes) {
    Node *node = var->value;

    node->type->init_pointers(nodes, node);
    list_add_end(&D->node_list, &node->__list);
  } ndictForEnd;
}


static void fill_indexies(Database *D) {
  Handler H[1];
  db_handler_init(D, H);
  tr_begin(H);

  list_for_each_item(node, &D->node_list, Node, __list) {
    if (!node->update_indexies(H, CBE_NODE_LOADED, node)) {
      dbDebug(E, "Filling indexies failed");
      exit(1);
    }

    while (!fstack_empty(H->log)) {
      struct LogItem item = fstack_top(H->log);
      fstack_pop(H->log);

      if (item.type == LI_TYPE_MEMORY_DELETED)
        generic_free(D->tm_allocator, item.ptr, 0);
    }
  }

  _tr_abort_main(H);
  db_handler_destroy(H);
}


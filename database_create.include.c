#include "database.h"
#include <stdio.h>
#include <string.h>
#include "dictionary.h"
#include "exception.h"
#include <dirent.h>

static EXCEPTION_DEFINE(EndOfLogCorrupted, NONE);
static EXCEPTION_DEFINE(EndOfFile, NONE);

enum {
  FIRST_FILE = 1,
  DUMP_ON = 2,
  RECOVER = 4
};

static int database_new_file__(Database* D, int dump_begin, uint64_t magic_nr);

static void check_schema(const DatabaseType* type, Reader* R) {
  R_ARRAY {
    R_STR("HybridDatabase Database Schema");
    R_STR(type->name);
    R_STR(type->version);

    NodeType *const* n_type = type->node_types;
    for (int i = 0; i < type->node_types_count; i++) R_ARRAY {
      R_STR(n_type[i]->name);

      for (int e = 0; e < n_type[i]->attributes_count; e++) R_ARRAY {
        R_STR(n_type[i]->attributes[i].name);
        if (R_NUMBER != n_type[i]->attributes[i].type->id) THROW_;
      } R_ARRAY_END

    } R_ARRAY_END
  } R_ARRAY_END
  R_CHECKSUM(1);
}

static void create_schema(const DatabaseType* type, Writer *W) {
  W_ARRAY {
    W_STR("HybridDatabase Database Schema");
    W_STR(type->name);
    W_STR(type->version);

    NodeType *const* n_type = type->node_types;
    for (int i = 0; i < type->node_types_count; i++) W_ARRAY {
      W_STR(n_type[i]->name);

      for (int e = 0; e < n_type[i]->attributes_count; e++) W_ARRAY {
        W_STR(n_type[i]->attributes[i].name);
        W_NUMBER(n_type[i]->attributes[i].type->id);
      } W_ARRAY_END

    } W_ARRAY_END
  } W_ARRAY_END
  W_CHECKSUM;
}

static void check_data_header(const DatabaseType* type, Reader* R) {
  R_ARRAY {
    R_STR("HybridDatabase Data File");
    R_STR(type->name);
    R_STR(type->version);
  } R_ARRAY_END
  R_CHECKSUM(1);
}

static void node_create(Reader* R, IdToNode *nodes, uint64_t *id_counter, NodeType *const* types) {
  NodeType *node_type = types[R_NUMBER];
  Node *node = tr_alloc(node_type->size);
  *(uint64_t*)&node->id = R_NUMBER;

  if (node->id > *id_counter) *id_counter = node->id;

  dict_insert(nodes, node->id, node);

  *(NodeType**)&node->type = node_type;
  node_type->init(node);
}

static void node_load(Reader* R, IdToNode *nodes, uint64_t *id_counter, NodeType *const* types) {
  NodeType *node_type = types[R_NUMBER];
  Node *node = tr_alloc(node_type->size);
  *(uint64_t*)&node->id = R_NUMBER;
  *(NodeType**)&node->type = node_type;

  if (node->id > *id_counter) *id_counter = node->id;

  dict_insert(nodes, node->id, node);

  node_type->load(R, node);
}

static void node_delete(Reader* R, IdToNode *nodes) {
  uint64_t id = R_NUMBER;

  if (!dict_contains(nodes, id)) return;

  Node *n = dict_at(nodes, id);
  n->type->destroy(n);
  tr_free(n);
  dict_remove(nodes, id);
}

static void node_change(Reader* R, IdToNode *nodes) {
  uint64_t id = R_NUMBER;
  int attr_id = R_NUMBER;

  if (!dict_bucket(nodes, id)) {
    R_SKIP;
    return;
  }

  Node *node = dict_at(nodes, id);

  const struct NodeAttribute *attr = &node->type->attributes[attr_id];
  attr->type->load(R, TR_OFFSET(node, attr->offset));
}

static void read_end_of_file(Reader* R, uint64_t *magic_nr) {
  R_STR("END OF FILE");
  R_CHECKSUM(0);
  *magic_nr = R_NUMBER;
  R_CHECKSUM(0);
}

static void read_end_of_dump(Reader* R, int *flags) {
  R_STR("DUMP END");
  R_CHECKSUM(0);
  if (*flags & DUMP_ON) *flags ^= DUMP_ON;
}

static void load_file(const DatabaseType* type, Reader *R, IdToNode *nodes,
                      int *flags, uint64_t *magic_nr, uint64_t* id_counter) {

  check_data_header(type, R);

  ENSURE_(R_NUMBER == *magic_nr || *magic_nr == 0);
  R_CHECKSUM(0);

  if (*flags & FIRST_FILE) {
    R_STR("DUMP BEGIN");
    R_CHECKSUM(0);
    *flags ^= FIRST_FILE;
  }

  while (1) switch (R_NEXT) {
    case 11 + ST_STRING: read_end_of_file(R, magic_nr);
      return;
    case 8 + ST_STRING:  read_end_of_dump(R, flags);
      break;
    case 10 + ST_STRING:
      R_STR("DUMP BEGIN");
      R_CHECKSUM(0);
      break;
    case 15: THROW(EndOfFile, 0);

    case ST_ARRAY_BEGIN: R_ARRAY {

      if (R_NEXT == ST_STRING) { // dump transaction
        R_STR("");
        if (*flags & DUMP_ON) {
          /*while (R_NEXT == ST_ARRAY_BEGIN) // echa dump "transaction" contains one node
            R_ARRAY {*/ node_load(R, nodes, id_counter, type->node_types); /*} R_ARRAY_END*/
        } else R_SKIP; // ignore more recent dumps

      } else { // normal transaction
        while (R_NEXT == ST_ARRAY_BEGIN) R_ARRAY {
          switch (R_NUMBER) {
            case CBE_NODE_CREATED: node_create(R, nodes, id_counter, type->node_types);
            break;

            case CBE_NODE_DELETED: node_delete(R, nodes);
              break;

            case CBE_NODE_MODIFIED: node_change(R, nodes);
              break;

            default: THROW_;
          }
        } R_ARRAY_END
      }

    } R_ARRAY_END
    R_CHECKSUM(1);
    break;

    default:
    TrDebug("Read: %li", (long)R_NEXT);
    THROW_;
  }
}

static Database* database_alloc(const DatabaseType* type) {
  Database *D = tr_alloc(type->size);
  *(const DatabaseType**)&D->type = type;

  D->time = 1;
  D->node_id_counter = 0;
  for (int i = 0; i < DB_LOCKS; i++) l_init(D->locks + i);

  D->node_list = D->node_list_end = 0;
  D->tr_old = D->tr_new = 0;

  D->filename = 0;
  D->output = 0;
  D->current_file_index = 0;

  pthread_mutex_init(&D->mutex, 0);
  pthread_cond_init(&D->signal, 0);

  pthread_mutex_init(&D->dump_mutex, 0);
  pthread_cond_init(&D->dump_signal, 0);

  D->dump_flags = 0;
  D->flags = 0;

  D->gc_thread = D->io_thread = D->dump_thread = 0;

 //  sem_init(&D->oq_size, 0, 0);
  D->oq_head = D->oq_tail = 0;

  D->tr_new = D->tr_old = tr_alloc(sizeof(*D->tr_new));
  D->tr_new->newer = 0;
  D->tr_new->time = 1;
  D->tr_new->running = 0;
  D->tr_new->node_list = 0;
  D->tr_new->void_list = 0;

  return D;
}

static void fill_indexes(Database* D) {
  // TODO / FIXME / CHECK - transactional magic, ok

  ALWAYS(Transaction* _transaction = tr_main_start(D)) {

    for (Node *node = D->node_list; node; node = node->_next) {
      TrStart {
        _transaction->database->type->indexes_update(_transaction, CBE_NODE_LOADED, node);
      } TrCommit

      // discarts undo log
      _transaction->write_log->ptr = _transaction->write_log->begin;
      stack_erase(_transaction->memory_new);
      while (!stack_empty(_transaction->memory_deleted))
        tr_free(stack_pop(_transaction->memory_deleted));
      // FIXME delete deleted memory, done
    }

  } ALWAYS_END(tr_main_abort(_transaction))

  // release locks :-)
  D->time = 1;
  for (int i = 0; i < DB_LOCKS; i++) l_init(D->locks + i);
}

static int get_db_files(struct dirent*** files, const char* dir, const char* db_name) {
  const size_t db_name_len = strlen(db_name);

  int db_files_sort(const struct dirent **a, const struct dirent **b) {
    for (int i = 0; db_name[i]; i++) if (db_name[i] != a[0]->d_name[i]) return 0;
    for (int i = 0; db_name[i]; i++) if (db_name[i] != b[0]->d_name[i]) return 0;

    long a_ = atol(a[0]->d_name + db_name_len);
    long b_ = atol(b[0]->d_name + db_name_len);

    if (a_ == b_) return 0;
    if (a_ > b_) return 1;
    return -1;
  }

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

  return scandir (dir, files, db_files_select, db_files_sort);
}

static void fix_pointers(Database *D, IdToNode *nodes) {
  DICT_FOR(nodes, var) {
    Node *n = var->value;

    // fix pointers
    n->type->init_pointers(nodes, n);

    // add node to D->node_list
    n->_next = D->node_list;
    n->_prev = 0;

    if (D->node_list) D->node_list->_prev = n;
    else D->node_list_end = n;

    D->node_list = n;
  } DICT_FOR_END;
}

#include "threads.include.c"

// FIXME
Database *database_create(DatabaseType* type, const char* dir, const char* file, unsigned int flags) {
#define bprintf(...) ({ sprintf(buffer, __VA_ARGS__); buffer; })

  TrDebug("Loading database '%s/%s' with flags %04x", dir, file, flags);

  Database *D = database_alloc(type);
  {
    char *x = tr_alloc(strlen(dir) + strlen(file) + 2);
    sprintf(x, "%s/%s", dir, file);
    D->filename = x;
  }

  Reader *R;
  char *buffer = tr_alloc(strlen(dir) + strlen(file) + 30);

  int data_files_count = -1;
  struct dirent** data_files = 0;

  TRY {
    // init indexes
    type->indexes_init(D);

    // TODO check files, examine first_file & last_file
    // TODO create files if they don't exists
    TrDebug_("Getting file list... ");
    ENSURE_((data_files_count = get_db_files(&data_files, dir, bprintf("%s.", file))) >= 0);
    TrDebug_("OK\n");

    TrDebug_("Files found (%i):\n", data_files_count);
    for (int i = 0; i < data_files_count; i++) TrDebug_("  '%s'\n", data_files[i]->d_name);

    if (data_files_count) {
      size_t l = strlen(buffer);
      int first_file = atol(data_files[0]->d_name + l);
      int last_file = atol(data_files[data_files_count - 1]->d_name + l);

      D->current_file_index = last_file;

      // if there are holes in file serie abort
      if (last_file - first_file != data_files_count - 1) {
        TrDebug("Some file is missing, aborting");
        THROW_;
      }
    }

    struct reader_fread_context read_ctx;

    // verify database schema
    TrDebug_("Verifing schema... ");
    ALWAYS(R = reader_fread_create(&read_ctx, bprintf("%s/%s.schema", dir, file)));

    if (!R && data_files_count) {
        TrDebug_("\nFound data files but no schema, aborting\n");
        THROW_;
      }

    if (!data_files_count) {
      if (!R) {
        TrDebug_("\nNo data files and no schema\nTrying to create empty database... ");

        ALWAYS(Writer W[1]; writer_init(W)) {
          create_schema(type, W);

          TrDebug_("\nsf: '%s'\n", buffer);
          ALWAYS(FILE *schema = ENSURE_(fopen(buffer, "wb")))
              fwrite(w_ptr(W), 1, w_length(W), schema);
          ALWAYS_END(fclose(schema))
        } ALWAYS_END(writer_destroy(W));

        TrDebug_("schema written, ");
      } else TrDebug_("\nNo data file found, creating empty... ");

      ENSURE_(database_new_file__(D, 1, 0) == TRERR_SUCCESS);

      {
        Writer W[1];
        writer_init(W);
        struct OutputQueue *Q = tr_alloc(sizeof(*Q));
        W_STR("DUMP END");
        oq_add(Q, D, W, 0, 0);
      }

      TrDebug_("created empty datafile OK\n");

      TrDebug_("Trying open schema again\n");

      if (!R) ENSURE_(R = reader_fread_create(&read_ctx, buffer));
    }

    check_schema(D->type, R);
    ALWAYS_END(reader_free(R))
    TrDebug_("OK\n");

    // load data
    if (data_files_count > 0) ALWAYS(IdToNode nodes[1] = { DictionaryInit }) {
      uint64_t magic_nr = 0;
      int load_flags = FIRST_FILE | DUMP_ON;

      /*for (int f = 0; f < data_files_count; f++) TRY {
        TrDebug_("Loading file '%s'\n", data_files[f]->d_name);
        ALWAYS(R = ENSURE_(reader_fread_create(&read_ctx, bprintf("%s/%s", dir, data_files[f]->d_name))))
          load_file(type, R, nodes, &load_flags, &magic_nr, &D->node_id_counter);
        ALWAYS_END(reader_free(R))
      } CATCH(EndOfFile) {
        if (f < data_files_count - 1 && !(flags & RECOVER)) RETHROW;
      } CATCH_ALL {
        if (!(flags & RECOVER)) RETHROW;
      } END;*/

      int i = 0;
      TRY {
        for (; i < data_files_count; i++)
          ALWAYS(R = ENSURE_(reader_fread_create(&read_ctx, bprintf("%s/%s", dir, data_files[i]->d_name))))
            load_file(type, R, nodes, &load_flags, &magic_nr, &D->node_id_counter);
          ALWAYS_END(reader_free(R))

         // if we're here, the last file ended with END OF FILE, so we start a new one
        database_new_file__(D, 0, magic_nr);
      } CATCH(EndOfFile) {
        if (i != data_files_count - 1) RETHROW;

        // opens file for writing
        ENSURE_(D->output = fopen(bprintf("%s.%i", D->filename, D->current_file_index), "ab"));
      } CATCH_ALL {
        TrDebug_("Error while loading file %s", buffer);
        RETHROW;
      } END;



      fix_pointers(D, nodes);
    } ALWAYS_END(dict_destroy(nodes))

    D->node_id_counter++;

    fill_indexes(D);

    // spawn threads
    if (pthread_create(&D->io_thread, 0, (void*(*)(void*))io_thread, (void*)D))
      THROW_;

    TRY {
      if (pthread_create(&D->gc_thread, 0, (void*(*)(void*))gc_thread, (void*)D))
        THROW_;

      if (pthread_create(&D->dump_thread, 0, (void*(*)(void*))dump_thread, (void*)D))
        THROW_;
    } CATCH_ALL {
      D->flags |= DB_SHUT_DOWN;
      pthread_cond_broadcast(&D->signal);

      pthread_join(D->io_thread, 0);
      if (D->gc_thread) pthread_join(D->gc_thread, 0);
      RETHROW;
    } END;

  } CATCH_ALL {
    if (D->tr_new) tr_free(D->tr_new);

    // destroy nodes
    Node* node_next;
    for (Node* node = D->node_list; node; node = node_next) {
      node_next = node->_next;
      node->type->destroy(node);
      tr_free(node);
    }

    // destroy indexes
    type->indexes_destroy(D);

    tr_free(D->filename);
    tr_free(D);
    D = 0;

    TrDebug_("Exception: %s Info: %s\n", exception->name, (char*)exception_info);
  } FINALLY {
    tr_free(buffer);

    if (data_files_count >= 0) {
      for (int i = 0; i < data_files_count; i++) free(data_files[i]);
      free(data_files);
    }
  } END

  return D;
#undef bprinft
}


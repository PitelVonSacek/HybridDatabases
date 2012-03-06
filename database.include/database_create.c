#include <dirent.h>
#include <libgen.h>

static int get_db_files(struct dirent*** files, const char* dir, const char* db_name);
static int load_file(Database *D, Reader *R, uint64_t *magic_nr, 
                     IdToNode *nodes, bool first_file);
static bool load_data(Database *D, IdToNode *nodes);
static void fix_pointers(Database *D, IdToNode *nodes);
static void fill_indexies(Database *D);

enum {
  LOAD_SUCCESS,
  LOAD_UNFINISHED_FILE,
  LOAD_CORRUPTED_FILE
};

Database *database_create (const DatabaseType *type, const char *file, unsigned flags) {
  Database *D = database_alloc(type);

  D->filename = strdup(file);
  D->flags = flags;

  IdToNode nodes;
  ndict_init(&nodes);

  if (!load_data(D, &nodes)) goto error;

  fix_pointers(D, &nodes);
  ndict_destroy(&nodes);
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

  return scandir (dir, files, db_files_select, db_files_sort /*versionsort*/);
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
    if (!node->type->update_indexes(H, CBE_NODE_LOADED, node)) {
      dbDebug(E, "Filling indexies failed");
      exit(1);
    }

    while (!fstack_empty(H->log)) {
      struct LogItem item = fstack_top(H->log);
      fstack_pop(H->log);

      if (item.type == LI_TYPE_MEMORY_DELETE)
        generic_free(D->tm_allocator, item.ptr, 0);
    }
  }

  _tr_abort_main(H);
  db_handler_destroy(H);
}

#define Ensure(cond, do_always, ...) \
  do { \
    if (!(cond)) { \
      dbDebug(E, __VA_ARGS__); \
      do_always; \
      goto error; \
    } \
    do_always; \
  } while (0)
static bool load_data(Database *D, IdToNode *nodes) {
  if (!D->filename || D->filename[0] == '\0') {
    dbDebug(I, "No input files specified, using /dev/null");
    D->flags |= DB_READ_ONLY;
    if (!(D->output.file = fopen("/dev/null", "wb"))) {
      dbDebug(E, "Failed to open /dev/null");
      return false;
    }
    return true;
  }

  bool ret = true;
  FILE *F;
  Reader R[1];

  char *_dir = strdup(D->filename);
  char *_file = strdup(_dir);
  char *dir = dirname(_dir);
  char *file = basename(_file);
  char *buffer = malloc(strlen(D->filename) + 30);
  struct dirent **files = 0;
  int files_count = 0;

  Ensure(strcmp(file, "/") && strcmp(file, ".") && strcmp(file, ".."),,
        "Bad database name '%s' (complete path was '%s')", file, D->filename);


  // load file list
  sprintf(buffer, "%s.", file);

  dbDebug(I, "Getting file list...");
  files_count = get_db_files(&files, dir, buffer);
  dbDebug(I, "Files found (%i):", files_count);
  for (int i = 0; i < files_count; i++) dbDebug(I, "  '%s'", files[i]->d_name);


  //  verify schema
  sprintf(buffer, "%s/%s.schema", dir, file);
  if (!(F = fopen(buffer, "rb"))) {
    if (!files_count) {
      dbDebug(I, "Failed to open schema but there are no data files too", buffer);
      Ensure(D->flags & DB_CREATE,);

      Ensure(F = fopen(buffer, "wb"),, "Failed to open '%s' for writing", buffer);
      Writer W[1];
      writer_init(W);
      write_schema(W, D);
      Ensure(fwrite(writer_ptr(W), 1, writer_length(W), F) == writer_length(W),
             ({ fclose(F); writer_destroy(W); }), "Failed to write schema");

      Ensure(F = fopen(buffer, "rb"),,);
    } else
      Ensure(0,, "Failed to open schema, but data files exists");
  }
  file_reader_init(R, F, true);
  Ensure(read_schema(R, D), reader_destroy(R), "Database schema corrupted");

  if (files_count) {
    sprintf(buffer, "%s.", file);
    size_t l = strlen(buffer);
    long first = atol(files[0]->d_name + l);
    long last = atol(files[files_count - 1]->d_name + l);

    D->current_file_index = last;

    Ensure(last - first == files_count - 1,, "Some file is missing");
  }

  uint64_t magic_nr = 0;
  bool create_new_file = true;

  for (int i = 0; i < files_count; i++) {
    sprintf(buffer, "%s/%s", dir, files[i]->d_name);
    Ensure(F = fopen(buffer, "rb"),, "Opening file '%s' failed", buffer);
    file_reader_init(R, F, true);

    switch (load_file(D, R, &magic_nr, nodes, i == 0)) {
      case LOAD_CORRUPTED_FILE: 
        dbDebug(E, "File '%s' is corrupted, switching to read-only mode",  buffer);
        D->flags |= DB_READ_ONLY;
        reader_destroy(R);
        goto end;

      case LOAD_UNFINISHED_FILE:
        if (i != files_count - 1) {
          dbDebug(E, "File '%s' is not finished but is not the last file, "
                     "switching to read-only mode", buffer);
          D->flags |= DB_READ_ONLY;
          reader_destroy(R);
          goto end;
        }
        dbDebug(I, "Last file is not finished, appending new data into it");
        create_new_file = false;
        break;

      case LOAD_SUCCESS:
        if (i == files_count - 1) {
          dbDebug(I, "Last file is closed, creating new data file");
        }
        break;

      default: Ensure(0,);
    }

    reader_destroy(R);
  }
  end:

  if (!(D->flags & DB_READ_ONLY)) {
    if (create_new_file) 
      Ensure(_database_new_file(D, !files_count, magic_nr),, 
             "Failed to create new file");
      if (!files_count) {
        Writer W[1];
        writer_init(W);
        write_dump_end(W);      
        Ensure(fwrite(writer_ptr(W), 1, writer_length(W), D->output.file) == 
               writer_length(W), writer_destroy(W), "Failed to write schema");
      }
    else 
      Ensure(D->output.file = fopen(buffer, "ab"),,
             "Failed to open '%s' for append", buffer);
  } else {
    Ensure(D->output.file = fopen("/dev/null", "wb"),, "Failed to open '/dev/null'");
  }

  if (0) {
    error:
    ret = false;
  }
  for (int i = 0; i < files_count; i++) free(files[i]);
  free(files);
  free(_dir);
  free(_file);
  free(buffer);

  return ret;
}
#undef Ensure

#define Ensure(cond, ...) \
  do { \
    if (!(cond)) { \
      dbDebug(E, __VA_ARGS__); \
      goto read_failed; \
    } \
  } while (0)

static int load_file(Database *D, Reader *R, uint64_t *magic_nr, 
                     IdToNode *nodes, bool first_file) {
  uint64_t new_magic_nr = 0;
  read_file_header(R, D, &new_magic_nr);
  bool dump_on = false;
  
  Ensure(!*magic_nr || *magic_nr == new_magic_nr, "File header corrupted");

  if (first_file) {
    Ensure(reader_begin(R) == ST_READ_SUCCESS && read_dump_begin(R),
           "First file must begin with dump");
    dump_on = true;
  }

  while (rMayBegin) switch (rNext) {
    case ST_ARRAY_BEGIN:
      rArray {
        switch (rNext) {

          case ST_ARRAY_BEGIN: // regular transaction
            while (rNext != ST_ARRAY_END) rArray {
              switch (rNumber) {
                case CBE_NODE_MODIFIED:
                  Ensure(read_node_modify(R, D, nodes));
                  break;
                case CBE_NODE_CREATED:
                  Ensure(read_node_create(R, D, nodes));
                  break;
                case CBE_NODE_DELETED:
                  Ensure(read_node_delete(R, D, nodes));
                  break;
                default: Ensure(0);
              }
            } rArrayEnd;
            break;

          case ST_STRING: // dump transaction
            rCheckNumber(0);
            Ensure(dump_on, "Dump transaction outside of dump");
            if (first_file) Ensure(read_node_load(R, D, nodes));
            else rSkip;
            break;

          default: Ensure(0);
        }
      } rArrayEnd;
      rFinish(1);

    case ST_STRING: {
      size_t pos = reader_get_pos(R);
      if (read_dump_begin(R)) {
        Ensure(!dump_on);
        dump_on = true;
        break;
      } 
      reader_set_pos(R, pos);
      if (read_dump_end(R)) {
        Ensure(dump_on);
        dump_on = false;
        break;
      }
      reader_set_pos(R, pos);
      if (read_file_footer(R, magic_nr)) {
        dbDebug(I, "EOF");
        return LOAD_SUCCESS;
      }
      Ensure(0, "Unknown string");
      break;
    }
    
    default: Ensure(0);
  }

  return LOAD_UNFINISHED_FILE;
 
  read_failed:
  return LOAD_CORRUPTED_FILE;
}
#undef Ensure


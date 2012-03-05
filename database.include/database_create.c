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

  fix_pointers(D);
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

  char *_dir = strdup(D->filename);
  char *_file = strdup(_dir);
  char *dir = dirname(_dir);
  char *file = basename(_file);
  char *buffer = malloc(strlen(D->filename) + 30);

  if (!strcmp(file, "/") || !strcmp(file, ".") || !strcmp(file, "..")) {
    trDebug(E, "Bad database name '%s' (complete path was '%s')", file, D->filename);
    goto error;
  }

  // FIXME verify schema

  sptrinf(buffer, "%s.", file);

  dirent **files = 0;
  dbDebug(I, "Getting file list...");
  int files_count = get_db_files(&files, dir, buffer);
  dbDebug(I, "Files found (%i):", files_count);
  for (int i = 0; i < files_count; i++) dbDebug(I, "  '%s'", files[i]->d_name);

  if (files_count) {
    size_t l = strlen(buffer);
    long first = atol(files[0]->d_name + l);
    long last = atol(files[files_count - 1] + l);

    D->current_file_index = last;

    if (last - first != files_count - 1) {
      dbDebug(E, "Some file is missing");
      goto error;
    }
  }

  FILE *F;
  Reader R[1];
  uint64_t magic_nr = 0;
  bool create_new_file = true;

  for (int i = 0; i < files_count; i++) {
    sprintf(buffer, "%s/%s.%i", dir, file, i);
    if (!(F = fopen(buffer, "rb"))) {
      dbDebug(E, "Opening file '%s' failed", buffer);
      goto error;
    }
    file_reader_init(R, F, true);

    switch (load_file(D, R, &magic_nr, nodes, i == 0)) {
      case LOAD_CORRUPTED_FILE: 
        dbDebug(E, "File '%s' is corrupted, switching to read-only mode");
        D->flags |= DB_READ_ONLY;
        reader_destroy(R);
        goto end;

      case LOAD_UNFINISHED_FILE:
        if (i != files_count - 1) {
          dbDebug(E, "File '%s' is not finished but is not the last file, "
                     "switching to read-only mode");
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
    }

    reader_destroy(R);
  }
  end:

  if (!(D->flags & DB_READ_ONLY)) {
    if (create_new_file) {
      if (!_database_new_file(D, 0, &magic_nr)) {
        dbDebug(E, "Failed to create new file");
        goto error;
      }
    } else if (!(D->output.file = fopen(buffer, "ab"))) {
      dbError(E, "Failed to open '%s' for append", buffer);
      goto error;
    } 
  } else {
    if (!(D->output.file = fopen("/dev/null", "wb"))) {
      dbDebug(E, "Failed to open '/dev/null'");
      goto error;
    }
  }

  if (0) {
    error:
    ret = false;
  }
  free(_dir);
  free(_file);
  free(buffer);

  return ret;
}

#define Ensure(cond, ...) \
  do { \
    if (!(cond)) { \
      dbDebug(E, __VA_ARG__); \
      goto read_failed; \
    } \
  } while (0)

static int load_file(Database *D, Reader *R, uint64_t *magic_nr, 
                     IdToNode *nodes, bool first_file) {
  uint64_t new_magic_nr = 0;
  read_file_header(R, D, &new_magic_nr);
  
  Ensure(*magic_nr && *magic_nr != new_magic_nr, "File header corrupted");

  if (first_file) {
    Ensure(read_begin(R) == ST_READ_SUCCESS && read_dump_begin(R),
           "First file must begin with dump");
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
            if (first_file) Ensure(read_node_load(R, D, nodes));
            else rSkip;
            break;

          default: Ensure(0);
        }
      } rArrayEnd;
      rFinish(1);

    case ST_STRING:
      // TODO read special values
      break;
    
  }
 
  read_failed:
  return LOAD_CORRUPTED_FILE;
}
#undef Ensure


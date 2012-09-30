# Hybrid database - simple example

NodeType Name     { Attribute value : String }
NodeType Greeting { Attribute value : String }


# Simple index
# Stores all nodes in array
IndexType Array : struct {
  IndexLock lock;
  size_t size, capacity;
  Node **data;
} {
  Init {
    *context = (typeof(*context)){
      .size = 0,
      .capacity = 0,
      .data = 0
    };
  }

  Destroy { tr_memory_late_free(allocator, context->data); }

  Update {
    switch (event) {
      // Put new node in index
      case CBE_NODE_CREATED:
      case CBE_NODE_LOADED: {
        trIndexLock(context);

        if (context->size == context->capacity) {
          size_t new_cap;
          void *data = tm_array_expand(H, context->data,
            sizeof(void*) * context->capacity, &new_cap);
          trMemoryInternalWrite(&context->data, data);
          trMemoryInternalWrite(&context->capacity, new_cap / sizeof(void*));
        }

        trMemoryInternalWrite(context->data + context->size, node);
        trMemoryInternalWrite(&context->size, context->size + 1);

        break;
      }

      // Remove deleted node from index
      case CBE_NODE_DELETED:
        trIndexLock(context);

        for (int i = 0; i < context->size; i++)
          if (typeUncast(context->data[i]) == node) {
            trMemoryInternalWrite(&context->data[i], context->data[context->size - 1]);
            trMemoryInternalWrite(&context->size, context->size - 1);

            if (context->size * 2 < context->capacity) {
              size_t new_cap;
              void *data = tm_array_shrink(H, context->data, 
                sizeof(void*) * context->capacity, &new_cap);
              trMemoryInternalWrite(&context->data, data);
              trMemoryInternalWrite(&context->capacity, new_cap / sizeof(void*));
            }

            return true;
          }
        break;

      // Just ignore node modification
      case CBE_NODE_MODIFIED: break;
    }
  }  
  
  # Returns # of nodes in array
  Method size : size_t { *value = trMemoryRead(context, ->size); }
  
  # Returns node at index @c index
  Method at(size_t index) : Node* { 
    Node **data = trMemoryRead(context, ->data);
    *value = data[index];
  }

  # Returns random node from array or NULL if array is empty
  Method get_random : Node* {
    size_t size = trMemoryRead(context, ->size);
    Node **data = trMemoryRead(context, ->data);
    
    *value = size ? data[rand() % size] : 0;
  }
}

DatabaseType HelloWorldDatabase (v1_0) {
  NodeType Name
  NodeType Greeting

  Index Names for Name : Array
  Index Greetings for Greeting : Array
}

Interface {
// We have to include database.h because Perl parser doesn't know
// where database.h is
#include "../database.h"
}

Implementation {

#include <stdio.h>

// simulates realloc in transactional memory
static void *tm_array_resize(Handle *H, void *ptr, size_t old_cap, size_t new_cap) {
  void *new_ = tr_memory_alloc(H, new_cap);
  memcpy(new_, ptr, old_cap);
  tr_memory_free(H, ptr);

  return new_;
}

static void *tm_array_expand(Handle *H, void *ptr, size_t size, size_t *new_cap) {
  *new_cap = size * 2 + 30;
  return tm_array_resize(H, ptr, size, *new_cap);
}

static void *tm_array_shrink(Handle *H, void *ptr, size_t size, size_t *new_cap) {
  *new_cap = size / 2;
  if (*new_cap < 30) *new_cap = 30;

  return tm_array_resize(H, ptr, *new_cap, *new_cap);
}


static void help(const char *name) {
  printf("Usage: %s command command_options...\n%s\n", name,
    "  Available commands are:\n"
    "    ag - adds greeting. Takes one argument. Argument should once contain\n" 
    "         %s sequence. It will by replaced by name.\n"
    "    an - adds name. Takes one argument.\n"
    "    p - prints random greeting with random name.\n"
    "    h - shows this help\n"
  );
}

int main(int argc, char **argv) {
  if (argc < 2) { help(argv[0]); return 1; }
  int ret = 0;

  HelloWorldDatabase *D = dbCreate(HelloWorldDatabase, "hello_db", DB_CREATE);
  if (!D) {
    printf("db creation failed\n");
    return 1;
  }

  HelloWorldDatabase_handle_t H[1];
  db_handle_init(D, H);
  
  tr_begin(H);
  switch (argv[1][0]) {
    case 'a':
      if (argc != 3) goto die;
      switch (argv[1][1]) {
        case 'g': {
          Greeting *h = trNodeCreate(Greeting);
          trWrite(h, value, argv[2]);
          break;
        }

        case 'n': {
          Name *n = trNodeCreate(Name);
          trWrite(n, value, argv[2]);
          break;
        }

        default: goto die;
      }
      break;

    case 'p': {
      Name *name = nodeCast(Name, trIndex(Names, get_random));
      Greeting *greeting = nodeCast(Greeting, trIndex(Greetings, get_random));

      // We can do this, because we are the only transaction so we cannot fail
      // The right way to do this would be sprintf to some buffer
      printf(greeting ? trRead(greeting, value) : "Hello %s!",
             name ? trRead(name, value) : "world");
      printf("\n");
      break;
    }

    case 'h':
      help(argv[0]);
      break;

    default:
      printf("Unknown command\n");
      help(argv[0]);
      goto die;
  }
  
  if (0) {
    die:
    tr_failed:
    printf("Error\n");
    ret = 1;
    tr_abort(H);
  } else if (!tr_commit(H, CT_ASYNC))
    printf("Error: transaction failed, but there is no one to colide with!\n");

  db_handle_destroy(H);
  database_close(D);

  return ret;
}

}


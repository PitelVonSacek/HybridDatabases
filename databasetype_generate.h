// input:
// #define NAME PokusnaDatabaze
// #define VERSION "0.1"
// #define NODE_TYPES NODE_TYPE(pokus1) NODE_TYPE(pokus2) NODE_TYPE(testik)
// #define INDEXES INDEX(prvy_index, index1, pokus1)

#if !defined(NAME) || !defined(VERSION) || !defined(NODE_TYPES) || !defined(INDEXES)
# error "NAME, VERSION, NODE_TYPES or INDEXES macro NOT defined!!!"
#endif

#define G_(a, b) a ## b
#define G(a, b) G_(a, b)
#define S(a) S_(a)
#define S_(a) #a

#include <stdlib.h>
#include <time.h>

/*****************************************
 * Subclasses of Database and Trasaction *
 *****************************************/

#define NODE_TYPE(type) const NodeType__##type##__desc_t *type;
typedef struct {
  DatabaseType __type;
  NODE_TYPES
  struct {} __dummy;
} G(DatabaseType__, NAME);
#undef NODE_TYPE

typedef union {
  Transaction __transaction;
  struct NAME *__database;
} G(NAME, __transaction);

#define INDEX(name, type, node_type) Index__##type##__context name;
typedef struct NAME {
  union {
    Database __database;
    G(DatabaseType__, NAME) *__type;
  };
  INDEXES
  G(NAME, __transaction) __dummy_transaction[0];
} NAME;
#undef INDEX



// database type descriptor
extern const G(DatabaseType__, NAME) G(G(DatabaseType__, NAME), __desc)[1];

void G(database_type_init__, NAME)();



#ifdef IMPLEMENT_DATABASE

#define INDEX(name, type, x) Index__##type##__context_init(&db->name);
void G(G(DatabaseType__, NAME), __indexes_init) (NAME* db) {
INDEXES
}
#undef INDEX

#define INDEX(name, type, x) Index__##type##__context_destroy(&db->name);
void G(G(DatabaseType__, NAME), __indexes_destroy) (NAME* db) {
INDEXES
}
#undef INDEX


#define INDEX(name_, type_, x) if (NodeType__##x##__desc->__type.name == node->type->name && \
  Index__##type_##__callback(&_transaction->__database->name_, (Transaction*)_transaction, event, node) != TRERR_SUCCESS) THROW_;

int G(G(DatabaseType__, NAME), __indexes_update)
  (G(NAME, __transaction)* _transaction, enum CallbackEvent event, Node* node) {
  TRY {
INDEXES
  } CATCH_ALL {
    return TRERR_COLLISION;
  } END
  return TRERR_SUCCESS;
}
#undef INDEX

#define NODE_TYPE(...) + 1
enum { G(G(DatabaseType__, NAME), __node_types_count) = 0 NODE_TYPES };
#undef NODE_TYPE

#define NODE_TYPE(name) NodeType__##name##__desc_t G(G(DatabaseType__, NAME), __node_type__##name)[1];
NODE_TYPES
#undef NODE_TYPE

#define NODE_TYPE(type) 0 + G(G(DatabaseType__, NAME), __node_type__##type),
const G(DatabaseType__, NAME) G(G(DatabaseType__, NAME), __desc)[1] = {{
  {
    S(NAME),
    VERSION,
    sizeof(NAME),

    (void(*)(Database*))&G(G(DatabaseType__, NAME), __indexes_init),
    (void(*)(Database*))&G(G(DatabaseType__, NAME), __indexes_destroy),
    (int (*)(Transaction*,enum CallbackEvent,Node*))&G(G(DatabaseType__, NAME), __indexes_update),

    G(G(DatabaseType__, NAME), __node_types_count)
  },
  NODE_TYPES
  {}
}};
#undef NODE_TYPE


//  G(G(DatabaseType__, NAME), __node_type__##type)->__type.update_indexes =
// (void (*)(Transaction*, enum CallbackEvent, Node*))&G(G(DatabaseType__, NAME), __update_indexes__##type);

#define NODE_TYPE(type) \
  G(G(DatabaseType__, NAME), __node_type__##type)[0] = NodeType__##type##__desc[0]; \
  G(G(DatabaseType__, NAME), __node_type__##type)->__type.id = i++;

void G(database_type_init__, NAME)() {
  srand ( time(NULL) );
  int i = 0;
  NODE_TYPES
}
#undef NODE_TYPE




#endif

#undef G
#undef G_

#undef S
#undef S_

#undef NAME
#undef NODE_TYPES
#undef INDEXES


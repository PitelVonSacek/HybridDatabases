
#include "database.h"

#define IMPLEMENT_ATTRIBUTES
#define IMPLEMENT_NODES
#define IMPLEMENT_DATABASE

#define NAME TestNode1
#define ATTRS ATTR(cislo, Int) ATTR(textik, String)
#include "nodetype_generate.h"

#define NAME TestNode2
#define ATTRS ATTR(popis, String) ATTR(ukazatel, Pointer)
#include "nodetype_generate.h"

#include "index.h"

#define NAME TestovaciDatabaze
#define VERSION "bla bla"
#define NODE_TYPES NODE_TYPE(TestNode1) NODE_TYPE(TestNode2)
#define INDEXES INDEX(prvy, Set, TestNode2) INDEX(treti, Set, TestNode1)
#include "databasetype_generate.h"

void main() {
  database_type_init__TestovaciDatabaze();


  TestovaciDatabaze* D = DatabaseCreate(TestovaciDatabaze, "", 0);
}



#include "database.h"

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

#include <pthread.h>


void *vlakno(void *D) {
  TrMainStart((TestovaciDatabaze*)D) {
    if (_tr_run) {
    //  TrDebug("Lock %lx, %lx", ((Database*)D)->locks[0], TrUncast(_transaction)->start_time);
    }
    //TrDebug_("@");
    Node(TestNode1) *n = NodeCast(TestNode1, TrIndex(treti, at, 0));
    //TrDebug_("$");
    TrWrite(n, cislo, TrRead(n, cislo) + 1);
    //TrDebug_("!");
    // TrAbort;

    for (int i = 0; i < 1000; i++) {
      char buffer[128];
      Node(TestNode2) *n1 = NodeCreate(TestNode2);
      sprintf(buffer, "Uzlik %i s adresou %p", i, n1);
      TrWrite(n1, popis, buffer);
      TrWrite(n1, ukazatel, n);
    }

  } TrMainCommit(0);
}

void main() {
  database_type_init__TestovaciDatabaze();

  printf("%i\n", NodeType__TestNode1__desc->cislo.offset);

  TestovaciDatabaze* D = ENSURE_(DatabaseCreate(TestovaciDatabaze, "dtest", "dd" , 0));

  TrMainStart(D) {
    Node(TestNode1) *n1 = NodeCreate(TestNode1);
    Node(TestNode2) *n2 = NodeCreate(TestNode2);
    TrWrite(n2, ukazatel, n1);
    TrWrite(n2, ukazatel, n1);
    TrWrite(n2, ukazatel, 0);

    node_remove(n2);

  } TrMainCommit(0);

  //database_new_file(D);
  //database_dump(D, 1);
TRY
  TrMainStart(D) {
  if (!TrIndex(treti, size)) TrWrite(NodeCreate(TestNode1), textik, (const char*)"vdfd");

    TrStart {
      Node(TestNode2) *n = NodeCreate(TestNode2);
      TrWrite(n, ukazatel, TrIndex(treti, at, 0));
    } TrCommit;

    TrStart {
      Node(TestNode2) *n = NodeCreate(TestNode2);
      TrWrite(n, ukazatel, TrIndex(treti, at, 0));
    } TrCommit;

    TrDebug_(">>>>\n");
    TrDebug_("nodes in treti %i\n", (int)TrIndex(treti, size));

    if (TrIndex(treti, size) < 2) {
      TrDebug_("Creating nodes\n");

     Node(TestNode1) *n = NodeCreate(TestNode1);

    TrWrite(n, textik, (const char*)"bla bla fuj");

    TrStart {
      TrWrite(n, textik, (const char*)"nic tu neni");
      TrAbort;
    } TrCommit;

   TrStart {
      Node(TestNode1) *n = NodeCreate(TestNode1);
      TrWrite(n, textik, (const char*)"hodnota");
      TrWrite(n, cislo, 345);
    } TrCommit;

    TrDebug_(":: %s\n", TrRead(n, textik));

    TrWrite(n, cislo, 255);

    TrDebug_("Items in treti %i\n", (int)TrIndex(treti, size));
    }

    for (int i = 0; i < TrIndex(treti, size); i++) {
      Node(TestNode1) *n = NodeCast(TestNode1, TrIndex(treti, at, i));
      char *t = TrRead(n, textik);
      int c = TrRead(n, cislo);
      // TrDebug(" %p %i: %s %i\n", n, i, t, c);
    }

    //node_dealloc(n);
    // TrAbort;
    TrDebug_(">>>>\n");
  } TrMainCommit(1);

  int vlaken = 7;
  pthread_t *vlakna = malloc(sizeof(pthread_t) * vlaken);
  for (int i = 0; i < vlaken; i++) pthread_create(vlakna + i, 0, vlakno, D);

  TrDebug_("Threads spawned\n");

  for (int i = 0; i < vlaken; i++) pthread_join(vlakna[i], 0);

  TrMainStart(D) {
    //int c = TrRead(NodeCast(TestNode1, TrIndex(treti, at, 0)), cislo);
    TrDebug_("Count is %i (node id %li)\n", (int)TrRead(NodeCast(TestNode1,
      TrIndex(treti, at, 0)), cislo), (long)TrIndex(treti, at, 0)->id);

  } TrMainCommit(0);

  scanf("a");

  TrMainStart(D) {
    TrRead(NodeCast(TestNode2, TrIndex(prvy, at, 0)), ukazatel);

    //TrDebug_("Pointer ukazuje na node %i\n", (int)TrRead(NodeCast(TestNode2, TrIndex(prvy, at, 0)), ukazatel)->id);
    int s = TrIndex(prvy, size);
    TrDebug_("prvku v prvy %i\n", s);
    if (s > 3) {
      //TrDebug("%i %p %i!!\n", (int)TrIndex(prvy, at, s - 1)->id, TrRead(NodeCast(TestNode2, TrIndex(prvy, at, 0)), ukazatel),
       //       TrRead(NodeCast(TestNode2, TrIndex(prvy, at, 0)), ukazatel)->id);
      ENSURE_(node_remove(TrIndex(prvy, at, s - 1)) == TRERR_SUCCESS);
    }
  } TrMainCommit(0);

  TrMainStart(D) {
    TrDebug_("Nemažu %i\n", node_remove(TrIndex(treti, at, 0)));
  } TrMainCommit(0);


  free(vlakna);
  CATCH_ALL TrDebug_("Exception: %s Info: %s\n", exception->name, (char*)exception_info); END

  TrDebug_(">>>>\n");





  ENSURE_(database_close(D) == TRERR_SUCCESS);
}

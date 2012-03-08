# database test 2

Interface @

#include <stdint.h>
#include <stdio.h>
#include "../database.h"

#include <sys/time.h>

static inline int64_t get_timer() {
  struct timeval t;
  gettimeofday(&t, 0);
  return 1000000*t.tv_sec + t.tv_usec;
}

@

NodeType Node1 {
  Attribute cislo : Int32
  Attribute text : String
  Attribute ptr : Pointer
}

IndexType Max : int {
  Init @ *context = 0 @
  Destroy @@
  Update @
    Node1 *n = nodeCast(Node1, node);
    int v = trInternalRead_(H, n, cislo);
    if (v > trMemoryRead(context, [0])) trMemoryWrite(context, [0], v);
  @
}

IndexType Counter : size_t {
  Init @ *context = 0 @
  Destroy @@
  Update @
    switch (event) {
      case CBE_NODE_LOADED:
      case CBE_NODE_CREATED:
        context[0] += 1;
        break;
      case CBE_NODE_DELETED:
        context[0] -= 1;
    }
  @
}

DatabaseType MyDatabase (v00) {
  NodeType Node1
  Index maximum for Node1 : Max
  Index count for all : Counter
}

Implementation @

#undef trFail
#define trFail ({ dbDebug(E,); return; })

void main(int argc, char **argv) {
  MyDatabase *D = dbCreate(MyDatabase, "dtest2/bb", DB_CREATE | DB_READ_ONLY);

  int c = atoi(argv[1]);
  int t = atoi(argv[2]);
  int sync = atoi(argv[3]);

  MyDatabase_handler_t H[1];
  db_handler_init(D, H);

  int64_t t0, t1, t2;

  printf("Creating %i node in %i transaction(s)\n", c, t);
  t0 = get_timer();
  
  for (int i = 0; i < t; i++) trBegin {

    for (int i = 0; i < c; i++) {
      Node1 *node = trNodeCreate(Node1);    

      trWrite(node, cislo, rand() * rand());
      trWrite(node, text, "Hello world");
    }



  } trCommit(sync);
  t1 = get_timer();

  trBegin { } trCommit(CT_SYNC);

  t2 = get_timer();
  printf("node count: %li\n", (long)D->indexes.count.context);

  printf("Time: create nodes %li ms  write them %li ms\n", (t1 - t0)/1000, (t2 - t1)/1000);

  db_handler_destroy(H);

  database_close(D);
}

@


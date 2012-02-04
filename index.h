#include "database_macros.h"

// Velmi triviální index, protě shromažďuje všechny uzly daného typu
// bez ladu a skladu
// funkce:
// size - vrátí počet uzlů v indexu
// at(i) - vrátí uzel na dané pozici
// cointains(uzel) - true obsahuje-li index daný uzel

typedef struct {
  size_t size, cap;
  Node **data;
} Index__Set__context;


static int Index__Set__callback(Index__Set__context* ctx, Transaction* _transaction,
                     enum CallbackEvent event, Node* node) {

  TRY
  switch (event) {
    case CBE_NODE_CREATED:
    case CBE_NODE_LOADED: {
      size_t size = TrBlockRead(&ctx->size);
      size_t cap = TrBlockRead(&ctx->cap);
      Node** data = TrBlockRead(&ctx->data);

      if (size == cap) {
        // enlarge ctx->data
        cap = size * 2 + 10;
        Node** new_data = tr_alloc(sizeof(Node*) * cap);
        TrMemoryAdd(new_data);

        // no need to use TrBlockRead on new_data, cause we're the only
        // transaction that knows about them
        for (size_t i = 0; i < size; i++) new_data[i] = TrBlockRead(data + i);

        TrMemoryDelete(data);
        TrBlockWrite(&ctx->data, new_data);
        TrBlockWrite(&ctx->cap, cap);

        data = new_data;
      }

      TrBlockWrite(data + size, node);
      TrBlockWrite(&ctx->size, size + 1);
    }
    break;
    case CBE_NODE_DELETED: {
      size_t size = TrBlockRead(&ctx->size);
      Node** data = TrBlockRead(&ctx->data);

      for (size_t i = 0; i < size; i++)
        if (TrBlockRead(data + i) == node) {
          if (i < size - 1) TrBlockWrite(data + i, TrBlockRead(data + size - 1));
          TrBlockWrite(&ctx->size, size - 1);
          break;
        }
    }
  }
  CATCH_ALL return TRERR_COLLISION; END

  return TRERR_SUCCESS;
}

static void Index__Set__context_init(Index__Set__context* ctx) {
  ctx->size = 0;
  ctx->cap = 0;
  ctx->data = 0;
}
static void Index__Set__context_destroy(Index__Set__context* ctx) {
  tr_free(ctx->data);
}

static int contains(Index__Set__context* ctx, Transaction* _transaction, Node* node) {
  size_t size = TrBlockRead(&ctx->size);
  Node** data = TrBlockRead(&ctx->data);

  for (size_t i = 0; i < size; i++)
    if (TrBlockRead(data + i) == node) return 1;

  return 0;
}

static size_t size(Index__Set__context* ctx, Transaction* _transaction) {
  return TrBlockRead(&ctx->size);
}

static Node* at(Index__Set__context* ctx, Transaction* _transaction, size_t index) {
  size_t size = TrBlockRead(&ctx->size);
  Node** data = TrBlockRead(&ctx->data);

  if (index >= size) THROW_;
  return TrBlockRead(data + index);
}



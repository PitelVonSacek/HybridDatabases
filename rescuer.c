#include "storage.h"
#include <stdio.h>

struct context {
  size_t size;
  size_t offset;
  unsigned char *data;
};

static void release(struct context *ctx) {
  free(ctx->data);
}
static size_t read(struct context* ctx, size_t l, unsigned char** out) {
  if (l > ctx->size) ctx->data = realloc(ctx->data, l);
  *out = ctx->data;
  return fread(ctx->data, 1, l, stdin);
}

void main() {
  struct context context = { 0, 0, 0 };

  Reader *R = reader_create((size_t (*)(void *, size_t, unsigned char **))read,
                                   &context, (void(*)(void*))release);

  Writer W[1];
  writer_init(W);

  TRY {
    while (1) {
      switch (R_NEXT) {
        case ST_ARRAY_BEGIN:
          R_ARRAY {
            w_array(W);
            int depth = 0;

            while (1) switch (R_NEXT) {
              case ST_ARRAY_BEGIN:
                r_array(R);
                w_array(W);
                depth++;
                break;
              case ST_ARRAY_END:
                if (depth-- > 0) {
                  r_array_end(R);
                  w_array_end(W);
                } else goto loop_end;
                break;
              default:
                if (R_NEXT < 16) THROW_;
                size_t l = R_NEXT - 16;
                memcpy(w_string(W, l), r_string(R), l);
            }

            loop_end:
            w_array_end(W);
          } R_ARRAY_END;
          break;

        case 15: goto end;

        default: {
          size_t l = R_LENGTH;
          memcpy(w_string(W, l), r_string(R), l);
        }
      }

      switch (r_checksum(R)) {
        case 1: W_CHECKSUM;
        case -1: break;
        default: THROW_;
      }

      fwrite(w_ptr(W), 1, w_length(W), stdout);
      w_discart(W);
    }
    end: ;
  } CATCH_ALL { fprintf(stderr, "Error\n"); } END;
}

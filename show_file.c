#include "storage.h"

void print(size_t l, const unsigned char* str) {
  printf("%i ", (int)l);
  printf("\"");

  for (size_t i = 0; i < l; i++) switch (str[i]) {
  case ' ':
  case '0' ... '9':
  case 'A' ... 'Z':
  case 'a' ... 'z':
  case '_':
  case '+':
  case '-':
  case '*':
  case '/':
  case '\'':
  case '&':
  case '|':
  case '=':
  case '<':
  case '>':
    printf("%c", str[i]);
    break;
  case '%':
    printf("%%");
    break;
  default:
    printf("%%%02x", (int)str[i]);
  }

  printf("\"");
}

struct reader_fread_context {
  FILE *f;
  size_t size;
  size_t offset;
  unsigned char *data;
};

static void reader_fread_release_context(struct reader_fread_context *ctx) {
  fclose(ctx->f);
  free(ctx->data);
}
static size_t reader_fread(struct reader_fread_context* ctx, size_t l, unsigned char** out) {
  if (l > ctx->size) ctx->data = realloc(ctx->data, l);
  *out = ctx->data;
  return fread(ctx->data, 1, l, ctx->f);
}

static Reader* reader_fread_create(struct reader_fread_context* context, const char* file) {
  if (!(context->f = fopen(file, "rb"))) return 0;

  context->size = context->offset = 0;
  context->data = 0;

  return reader_create((size_t (*)(void *, size_t,  unsigned char **))reader_fread,
                       context, (void(*)(void*))reader_fread_release_context);
}

void ind(int indent) {
  for (int i = 0; i < indent; i++) printf("  ");
}

void main(int argc, char** argv) {
  struct reader_fread_context ctx;
  int indent = 0;

  ALWAYS(Reader *R = ENSURE_(reader_fread_create(&ctx, argv[1]))) {
    TRY {
    while (1) switch (R_NEXT) {
    case ST_ARRAY_BEGIN:
      ind(indent);
      printf("{\n");
      indent++;

      r_array(R);
      break;
    case ST_ARRAY_END:
      indent--;
      ind(indent);
      printf("}\n");
      r_array_end(R);

      if(indent == 0) printf("DOT(%i)\n", r_checksum(R));
      break;
    case 15: goto end;
    case ST_CRC:
    case ST_DOT:
      printf("DOT(%i)\n", r_checksum(R));
      break;
    default:
      if (r_next(R) < ST_STRING) {
        printf("OOps %%%x\n", (int)R_NEXT);
        THROW(R_UNEXPECTED, R_EXC_INFO);
      }
      ind(indent);
      size_t l = R_NEXT - 16;
      print(l, r_string(R));
      printf("\n");
    }

    end: ;
    } CATCH_ALL {
      printf("Exception: %s Info: %s\n", exception->name, (char*)exception_info);
      printf("Dump: ");
      print(16, R->ptr - 1);
      printf("\n");
    } END;
  } ALWAYS_END(reader_free(R));
}


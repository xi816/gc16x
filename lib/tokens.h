#include <holyc-types.h>
enum TT;
typedef enum TT TT;

struct Token {
  U8 type;
  U16 addr;
  union {
    U8* s;
    U64 i;
  } value;
};

typedef struct Token Token;

U0 printtoks(Token* toks) {
  U16 p = 0;
  printf("%d: %d (%s | %d)\n", p, toks[p].type, toks[p].value.s, toks[p].value.i);
  while (toks[p].addr != p) {
    printf("%d: %d (%s | %d)\n", p, toks[p].type, toks[p].value.s, toks[p].value.i);
    p++;
  }
}


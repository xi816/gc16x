#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <tokens.h>
#include <scuffers.h>
#include <holyc-types.h>

enum TT {
  INT = 0,
  CHAR = 1,
  INST = 2,
  NAME = 3
};

U8* keywords[] = {
  "mov", "jmp"
};
U16 keylen = 2;

Token* lex(U8* buf, U32 srclen, Token* toks) {
  U32 pos = 0;
  U16 tpos = 0;
  Token tok;
  Scuffer temp = {.end = 0};

  while (pos < srclen) {
    // if ((buf[pos] >= '0') && (buf[pos] <= '9')) {
    if (buf[pos] == '$') {
      pos++;
      while ((buf[pos] >= '0') && (buf[pos] <= '9')) {
        scuffer_add(&temp, buf[pos]);
        pos++;
      }
      toks[tpos++] = (Token){.type = INT, .value.i = strtol(temp.cont, NULL, 16)};
    }
    else if ((buf[pos] >= 'a') && (buf[pos] <= 'z')) {
      while ((buf[pos] >= 'a') && (buf[pos] <= 'z')) {
        scuffer_add(&temp, buf[pos]);
        pos++;
      }
      for (U16 i = 0; i < keylen; i++) {
        if (!strcmp(temp.cont, keywords[i])) {
          toks[tpos++] = (Token){.type = INST, .value.s = temp.cont};
          scuffer_clear(&scuffer);
          break;
        }
      }
    }
    else if (buf[pos] <= ' ') {
      pos++;
    }
    else {
      fprintf(stderr, "Unknown character \"%c\"\n", buf[pos]);
      exit(1);
    }
  }

  return toks;
}

U0 showhelp(void) {
  puts("Syntax: kasm <file.asm> <file.bin>");
  exit(0);
}

U0 panic(char* msg) {
  fputs("\033[31mError\033[0m\n  ", stderr);
  fputs(msg, stderr);
  putchar(10);
  exit(1);
}

U32 main(U32 argc, I8** argv) {
  puts("ochkasm 0.1");
  if (argc < 3) showhelp();
  if (argc > 4) showhelp();

  FILE* srcf = fopen(argv[1], "r");
  if (srcf == NULL) panic("Unable to open file");
  fseek(srcf, 0, SEEK_END);
  U32 srclen = ftell(srcf);
  fseek(srcf, 0, SEEK_SET);

  U8 buf[srclen];
  fread(buf, 1, 65536, srcf);

  printf("asm program size: %d\n", srclen);
  Token* toks = malloc(1024*sizeof(Token));
  toks = lex(buf, srclen, toks);
  printtoks(toks);

  return 0;
}


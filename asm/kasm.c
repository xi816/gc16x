#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#define TOKENS_CAPACITY 1024

uint32_t p = 0; // Lexer position
uint16_t line = 1; // 65'536 lines is enough for everybody

typedef enum {
  TK_INST,
  TK_ID,
  TK_INT,
  TK_EOF
} TokenType;

typedef struct {
  TokenType type;
  size_t value; // kto takoi size_t?
  uint16_t line; // 65'536 lines is enough for everybody
} Token;

Token tokens[TOKENS_CAPACITY];
uint32_t tp = 0; // tp points to the last token offset in Token* plus one

char* GC_INSTS[] = {
  "push", "int"
};
uint16_t GC_INSTS_SIZE = sizeof(GC_INSTS) / sizeof(GC_INSTS[0]);

uint8_t is_gcinst(char* buf) {
  for (uint16_t i = 0; i < GC_INSTS_SIZE; i++) {
    if (!strcmp(GC_INSTS[i], buf)) return 1;
  }
  return 0;
}

void piss_off(char* filename) {
  puts("Dear kasm user,");
  puts("WHY THE HELL ARE YOU USING TABS INSTEAD OF SPACES");
  puts("I won't even lex that file until you remove the tabs");
  puts("kasm (c), 2'025");
  exit(1);
}

void kasm_lex_number(char* src, char* filename) {
  int32_t buf = 0;
  while (isdigit(src[p])) {
    buf *= 10;
    buf += src[p++]-48;
  }
  tokens[tp++] = (Token){.type = TK_INT, .value = (size_t)buf};
}

void kasm_lex_hex(char* src, char* filename) {
  puts("kasm: kasm_lex_hex is not implemented yet");
  exit(1);
}

void kasm_lex_directive(char* src, char* filename) {
  puts("kasm: kasm_lex_directive is not implemented yet");
  puts("And won't be in the near future. We can write good code");
  puts("without fancy shmancy assembler features.");
}

void kasm_lex_instruction(char* src, char* filename) {
  char buf[64]; // i don't think we will surpass that limit in any time soon
  uint8_t bufp = 0; // buffer pointer
  while (isalpha(src[p])) {
    buf[bufp++] = src[p++];
  }
  buf[bufp++] = 0;
  if (is_gcinst(buf)) {
    tokens[tp++] = (Token){.type = TK_INST, .value = (size_t)strdup(buf)};
  }
  else {
    tokens[tp++] = (Token){.type = TK_ID, .value = (size_t)strdup(buf)};
  }
}

uint8_t kasm_lex_file(char* src, char* filename) {
  while (src[p]) {
    switch (src[p]) {
      case '0' ... '9':
        kasm_lex_number(src, filename);
        break;
      case 'A' ... 'Z':
        kasm_lex_directive(src, filename);
        break;
      case 'a' ... 'z':
        kasm_lex_instruction(src, filename);
        break;
      case '$':
        kasm_lex_hex(src, filename);
        break;
      case ';':
        while (src[p] != '\n') p++;
        break;
      case ' ':
        p++;
        break;
      case '\t':
        piss_off(filename);
        break;
      case '\n':
        p++;
        line++;
        break;
      default:
        printf("kasm: lexer: unknown character `%c`\n", src[p]);
    }
  }
  tokens[tp++] = (Token){.type=TK_EOF, .value = (size_t)0};
}

void kasm_output_tokens() {
  for (uint16_t i = 0; i < tp; i++) {
    switch (tokens[i].type) {
      case TK_INST:
        printf("type=inst, value=%s\n", (char*)tokens[i].value);
        break;
      case TK_ID:
        printf("type=id, value=%s\n", (char*)tokens[i].value);
        break;
      case TK_INT:
        printf("type=int, value=%d\n", (uint32_t)tokens[i].value);
        break;
      case TK_EOF:
        printf("type=eof\n");
        break;
      default:
        printf("kasm: kasm_output_tokens: unknown token type %d", tokens[i].type);
    }
  }
}

void usage(void) {
  puts("Syntax: kasm <file.asm> <file.bin>");
}

int main(int argc, char** argv) {
  if (argc == 1) {
    puts("kasm: fatal error: too few argumets given");
    usage();
    return 1;
  }
  FILE* fl = fopen(argv[1], "r");
  if (fl == NULL) {
    printf("kasm: fatal error: could not open file `%s`\n", argv[1]);
    return 1;
  }
  fseek(fl, 0, SEEK_END);
  uint32_t srclen = ftell(fl);
  fseek(fl, 0, SEEK_SET);
  uint8_t src[srclen];
  fread(src, 1, srclen, fl);
  fclose(fl);
  kasm_lex_file(src, argv[1]);
  kasm_output_tokens();
}

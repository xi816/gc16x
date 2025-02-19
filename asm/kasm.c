#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#define TOKENS_CAPACITY 1024
#define INSTS_CAPACITY 1024
#define ASSEMBLE_CAPACITY 65536

uint32_t p = 0; // Lexer position
uint16_t line = 1; // 65'536 lines is enough for everybody
uint16_t pc = 0; // Parser PC register for instruction addresses

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

typedef enum {
  INST_PUSH,
  INST_INT,
  PARSER_EOF
} Opcode;

typedef enum {
  OP_NULL = 0,
  OP_IMM16,
  OP_REG16,
  OP_RC16
} OperandType;

typedef struct {
  Opcode mnemonic;
  OperandType optype;
  size_t operand1;
  size_t operand2;
  uint16_t pc;
} Inst;

Token tokens[TOKENS_CAPACITY];
uint32_t tp = 0; // tp points to the last token offset in Token* plus one

Inst insts[INSTS_CAPACITY]; // Parser output
uint32_t ip = 0; // ip points to the last instruction in the AST (Inst*) plus one

uint8_t bc[ASSEMBLE_CAPACITY];
uint32_t bp = 0; // bp points to the last byte assembled to the assembler buffer

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

void put_word(uint16_t word) {
  bc[bp++] = word % 256;
  bc[bp++] = word >> 8;
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
        printf("kasm: lexer: hex code $%02X\n", src[p]);
        exit(1);
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

void kasm_parse_file(char* filename) {
  while (tokens[p].type != TK_EOF) {
    // printf("p: %d\n", p);
    if (tokens[p].type == TK_INST) {
      if (!strcmp((char*)tokens[p].value, "push")) {
        if (tokens[p+1].type == TK_INT) {
           insts[ip++] = (Inst){.mnemonic=INST_PUSH, .optype=OP_IMM16, .operand1=(uint16_t)tokens[p+1].value, .pc=pc};
           pc += 4;
           p += 2;
        }
        else {
           fprintf(stderr, "kasm: parser error: illegal `push` opcode\n");
           exit(1);
        }
      }
      else if (!strcmp((char*)tokens[p].value, "int")) {
        if (tokens[p+1].type == TK_INT) {
          insts[ip++] = (Inst){.mnemonic=INST_INT, .optype=OP_IMM16, .operand1=(uint8_t)tokens[p+1].value, .pc=pc};
          pc += 3;
          p += 2;
        }
        else {
          fprintf(stderr, "kasm: parser error: illegal `int` opcode\n");
          exit(1);
        }
      }
      else {
        fprintf(stderr, "kasm: parser error: unhandled instruction `%s`\n", (char*)tokens[p].value);
        exit(1);
      }
    }
    else {
      fprintf(stderr, "kasm: parser error: expected instruction, found something else\n");
      exit(1);
    }
  }
  insts[ip++] = (Inst){.mnemonic=PARSER_EOF, .optype=OP_NULL, .operand1=(size_t)0, .pc=pc};
}

void kasm_output_insts() {
  for (uint32_t i = 0; insts[i].mnemonic != PARSER_EOF; i++) {
    if ((insts[i].mnemonic == INST_PUSH) && (insts[i].optype == OP_IMM16)) {
      printf("%04X    push $%04X\n", insts[i].pc, (uint16_t)insts[i].operand1);
    }
    else if ((insts[i].mnemonic == INST_INT) && (insts[i].optype == OP_IMM16)) {
      printf("%04X    int $%02X\n", insts[i].pc, (uint8_t)insts[i].operand1);
    }
    else {
      fprintf(stderr, "kasm: parser output: unexpected token type");
      exit(1);
    }
  }
}

void kasm_compile_file(char* filename) { // filename will be used for error output
  while (insts[p].mnemonic != PARSER_EOF) {
    if ((insts[p].mnemonic == INST_PUSH) && (insts[p].optype == OP_IMM16)) { // push imm16
      bc[bp++] = 0x0F;
      bc[bp++] = 0x84;
      put_word((uint16_t)insts[p].operand1);
      p++;
    }
    else if ((insts[p].mnemonic = INST_INT) && (insts[p].optype == OP_IMM16)) { // int imm8
      bc[bp++] = 0x0F;
      bc[bp++] = 0xC2;
      bc[bp++] = (uint8_t)insts[p].operand1;
      p++;
    }
    else {
      fprintf(stderr, "kasm: error: unhandled instruction of type %d\n", insts[p].mnemonic);
      exit(1);
    }
  }
}

void usage(void) {
  puts("Syntax: kasm <file.asm> <file.bin>");
}

int main(int argc, char** argv) {
  if (argc < 3) {
    puts("kasm: fatal error: too few argumets given");
    usage();
    return 1;
  }
  else if (argc > 3) {
    fprintf(stderr, "kasm: fatal error: expected 2 arguments, got %d", argc-1);
    usage();
    return 1;
  }
  FILE* fl = fopen(argv[1], "r");
  if (fl == NULL) {
    printf("kasm: fatal error: could not open file `%s`\n", argv[1]);
    return 1;
  }
  fseek(fl, 0, SEEK_END);
  uint32_t srclen = ftell(fl)+1;
  fseek(fl, 0, SEEK_SET);
  uint8_t src[srclen];
  fread(src, 1, srclen, fl);
  src[srclen-1] = 0;
  fclose(fl);
  kasm_lex_file(src, argv[1]);
  puts("Emitting tokens:");
  kasm_output_tokens();
  p = 0; // Reset p so we can use it for the parser too
  kasm_parse_file(argv[1]);
  puts("\nEmitting AST:");
  kasm_output_insts();
  p = 0; // Reset p so we can use it for the assembler
  kasm_compile_file(argv[1]);
  printf("kasm: %s: assembled %d bytes\n", argv[1], bp);
  for (uint16_t i = 0; i < bp; i++) {
    printf("%02X ", bc[i]);
  }
  puts("\b");
  FILE* output = fopen(argv[2], "wb");
  fwrite(bc, 1, bp, output);
  fclose(output);

  return 0;
}

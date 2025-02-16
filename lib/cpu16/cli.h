// The CLI for using the emulator
#include <cpu16/gpuh.h>
#include <cpu16/cpu16h.h>
#define GC16X_VERSION "1.0"
#define EXEC_START 1

typedef struct GC16X GC;
char* regf[16] = {
  "AX", "BX", "CX", "DX", "SI", "GI", "SP", "BP",
  "EX", "FX", "HX", "LX", "X ", "Y ", "IX", "IY",
};

U8 cli_DisplayReg(GC* gc) {
  for (U8 i = 0; i < 16; i++) {
    if ((i != 0) && !(i%4)) putchar(10);
    printf("%s:\033[93m$%04X\033[0m  ", regf[i], gc->reg[i]);
  }
  printf("\nPS:\033[93m%08b\033[0m  ", gc->PS);
  printf("PC:\033[93m$%04X\033[0m  ", gc->PC);
  printf("\n   -I---ZNC\n");
}

U8 putmc(U8 c) {
  switch (c) {
  case 0x00 ... 0x1F:
    putchar('.');
    break;
  case 0x20 ... 0x7E:
    putchar(c);
    break;
  case 0x80 ... 0xFF:
    putchar('~');
    break;
  }
  return 0;
}

U8 cli_DisplayMem(GC* gc, U8 page) {
  fputs("\033[A", stdout);
  for (U32 i = page*256; i < page*256+256; i++) {
    if (!(i % 16)) {
      printf("\n%04X  ", i);
    }
    printf("%02X ", gc->mem[i]);
  }
  putchar(10);
  return 0;
}

U8 cli_DisplayMemX(GC* gc, U8 page) {
  fputs("\033[A", stdout);
  for (U32 i = page*256; i < page*256+256; i++) {
    if (!(i % 16)) {
      printf("\n%04X  ", i);
    }
    putmc(gc->mem[i]);
  }
  putchar(10);
  return 0;
}

U8 cli_InsertMem(GC* gc, U16 addr, U8 byte) {
  gc->mem[addr] = byte;
  return 0;
}

U8 ExecD(GC* gc, U8 trapped) {
  char* inpt;
  char* tokens[10];
  size_t bufsize = 25;
  uint8_t j = 0;
  char* buf = (char*)malloc(bufsize);

  if (trapped) printf("\n\033[91mtrapped\033[0m at PC$%04X\n", gc->PC);
  else printf("gc16x emu %s\n", GC16X_VERSION);

  execloop:
  fputs(": ", stdout);
  getline(&buf, &bufsize, stdin);
  j = 0;
  tokens[j] = strtok(buf, " ");
  while (tokens[j] != NULL) {
    tokens[++j] = strtok(NULL, " ");
  }
  switch ((*tokens)[0]) {
  case 'q':
    if (trapped) exit(0);
    return 0;
  case 'R':
    return EXEC_START;
  case 'r':
    cli_DisplayReg(gc);
    break;
  case 'c':
    fputs("\033[H\033[2J", stdout);
    break;
  case 'm':
    if (j == 2)
      cli_DisplayMem(gc, strtol(tokens[1], NULL, 16));
    break;
  case 'M':
    if (j == 2)
      cli_DisplayMemX(gc, strtol(tokens[1], NULL, 16));
    break;
  case 'i':
    if (j == 3)
      cli_InsertMem(gc, strtol(tokens[1], NULL, 16), strtol(tokens[2], NULL, 16));
    break;
  case 'h':
    puts("gc16x cli help:");
    puts("  c       Clear the screen");
    puts("  h       Show help");
    puts("  m <00>  Dump memory");
    puts("  r       Dump registers");
    puts("  R       Run the program");
    puts("  q       Quit");
    break;
  default:
    puts("unknown command");
  }
  goto execloop;
  return 0;
}

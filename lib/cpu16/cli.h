// The CLI for using the emulator
#define GC16X_VERSION "1.0"
#define EXEC_START 1

U8 ExecD(GC* gc) {
  system("stty icanon isig iexten echo");
  char* inps;
  printf("gc16x emu %s\n", GC16X_VERSION);
  execloop:
  fputs(": ", stdout);
  scanf("%s", inps);
  if (!strcmp(inps, "q")) {
    return 0;
  }
  else if (!strcmp(inps, "r")) {
    return EXEC_START;
  }
  else if (!strcmp(inps, "m")) {
    fputs("\033[A", stdout);
    for (U16 i = 0; i < 256; i++) {
      if (!(i % 16)) {
        printf("\n%04X  ", i);
      }
      printf("%02X ", gc->mem[i]);
    }
    putchar(10);
  }
  else if (!strcmp(inps, "h")) {
    puts("gc16x cli help:");
    puts("  h       Show help");
    puts("  m       Dump memory");
    puts("  r       Dump registers");
    puts("  q       Quit");
  }
  else {
    puts("unknown command");
  }
  goto execloop;
  return 0;
}

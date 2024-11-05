#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <termios.h>

#include <holyc-types.h>
#include <sterm-control.h>
#include <cpu16/cpu16.h>

U8 main(I32 argc, I8** argv) {
  const U32 memsize = 65536;
  new_st;

  parseArgs:
  if (argc == 1) {
    fprintf(stderr, "No arguments given\n");
    old_st;
    return 1;
  }
  else if (argc > 2) {
    fprintf(stderr, "Expected 1 argument, got %d\n", argc-1);
    old_st;
    return 1;
  }
  I8 fn[strlen(argv[1])];
  strcpy(fn, argv[1]);

  FILE* fl = fopen(fn, "rb");
  fseek(fl, 0, SEEK_END);
  const U32 flsize = ftell(fl);
  fseek(fl, 0, SEEK_SET);

  GC gc;
  fread(gc.mem, 1, flsize, fl);
  fclose(fl);

  Reset(&gc);
  U8 ExecExit = Exec(gc, memsize);
  if (ExecExit) {
    old_st;
    return ExecExit;
  }

  old_st;
  return 0;
}


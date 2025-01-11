#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <termios.h>

#include <holyc-types.h>
#include <gc16x-types.h>
#include <sterm-control.h>
#include <cpu16/cpu16.h>

U8 loadBootSector(U8* drive, U8* mem) {
  U8* odrive = drive;
  while (1) {
    if ((*(drive+0x91EE) == 0xAA) && (*(drive+0x91EF) == 0x55)) break;
    *(mem++) = *(drive+0x91EE);
    drive++;
  }
  return 0;
}

U8 main(I32 argc, I8** argv) {
  new_st;
  I8 dfn[strlen(argv[1])]; // Disk filename buffer
  I8 fn[strlen(argv[1])]; // Memory filename buffer
  U16 driveboot;

  driveboot = 0x0000;
  parseArgs:
  if (argc == 1) {
    fprintf(stderr, "No arguments given\n");
    old_st;
    return 1;
  }
  else if (argc == 3) { // Load disk
    strcpy(dfn, argv[2]);
    driveboot = 0x91EE;
  }
  else if (argc > 3) {
    fprintf(stderr, "Expected 1 argument, got %d\n", argc-1);
    old_st;
    return 1;
  }
  strcpy(fn, argv[1]);

  FILE* fl = fopen(fn, "rb");
  FILE* dfl;
  U32 dflsize = 0;
  if (fl == NULL) {
    fprintf(stderr, "Error while opening %s\n", fn);
    old_st;
    return 1;
  }
  if (driveboot) {
    dfl = fopen(dfn, "rb");
    if (dfl == NULL) {
      fprintf(stderr, "Error while opening %s\n", fn);
      old_st;
      return 1;
    }
    fseek(dfl, 0, SEEK_END);
    dflsize = ftell(dfl);
    fseek(dfl, 0, SEEK_SET);
  }
  fseek(fl, 0, SEEK_END);
  U32 flsize = ftell(fl);
  fseek(fl, 0, SEEK_SET);

  // CPU
  GC gc;
  fread(gc.mem, 1, flsize, fl);
  fread(gc.rom, 1, dflsize, dfl);
  gc.pin |= 0b00000000;
  if (driveboot) {
    gc.pin |= 0b10000000; // 1 - Drive is connected
    loadBootSector(gc.rom, gc.mem);
    fclose(dfl);
  }
  fclose(fl);
  Reset(&gc, driveboot);
  // GPU
  gravno_start;
  gc.renderer = renderer;
  GGinit(&(gc.gg), renderer);

  U8 ExecExit = Exec(gc, MEMSIZE, renderer);
  if (ExecExit) {
    old_st;
    return ExecExit;
  }

  gravno_end;
  old_st;
  return 0;
}


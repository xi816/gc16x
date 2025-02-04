#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <termios.h>

#include <holyc-types.h>
#include <gc16x-types.h>
#include <sterm-control.h>
#include <cpu16/bpf.h>
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
  srand(time(NULL));
  new_st;
  U16 driveboot;

  driveboot = 0x0000;
  parseArgs:
  if (argc == 1) {
    fprintf(stderr, "No arguments given\n");
    old_st;
    return 1;
  }
  else if (argc == 3) {
    // Load from the disk
    if ((!strcmp(argv[1], "disk")) || (!strcmp(argv[1], "-d")) || (!strcmp(argv[1], "--disk"))) {
      driveboot = 0x91EE;
    }
  }
  else if (argc > 3) {
    fprintf(stderr, "Expected 1 argument, got %d\n", argc-1);
    old_st;
    return 1;
  }

  // Create a virtual CPU
  GC gc;
  gc.pin = 0b00000000; // Reset the pin
  Reset(&gc);

  if (!driveboot) { // Load a memory dump
    FILE* fl = fopen(argv[1], "rb");
    if (fl == NULL) {
      fprintf(stderr, "\033[31mError\033[0m while opening %s\n", argv[1]);
      old_st;
      return 1;
    }
    fread(gc.mem, 1, 65536, fl);
    fclose(fl);
    // Disk signaures for GovnFS (without them, fs drivers would not work)
    gc.rom[0x00] = 0x60;
    gc.rom[0x11] = '#';
    gc.rom[0x21] = 0xF7;
    gc.pin &= 0b01111111;
  }
  else { // Load a disk
    FILE* fl = fopen(argv[2], "rb");
    if (fl == NULL) {
      fprintf(stderr, "\033[31mError\033[0m while opening %s\n", argv[2]);
      old_st;
      return 1;
    }
    fread(gc.rom, 1, 65536, fl);
    fclose(fl);
    // Load the boot sector from $91EE into memory
    loadBootSector(gc.rom, gc.mem);
    // Setup the pin bit 7 to 1 (drive)
    gc.pin |= 0b10000000;
  }

  // GPU
  gravno_start;
  gc.renderer = renderer;
  GGinit(&(gc.gg), renderer);

  U8 exec_errno = Exec(&gc, MEMSIZE);
  gravno_end;
  old_st;
  if (driveboot) { // Save the modified disk back
    FILE* fl = fopen(argv[2], "wb");
    if (fl == NULL) {
      fprintf(stderr, "\033[31mError\033[0m while opening %s\n", argv[2]);
      old_st;
      return 1;
    }
    fwrite(gc.rom, 1, 65536, fl);
    fclose(fl);
  }
  return exec_errno;

}

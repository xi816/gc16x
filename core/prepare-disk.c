#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define ptrlen(t) (sizeof(t)/sizeof(t[0]))

int32_t main(int argc, char** argv) {
  if (argc != 2) {
    fprintf(stderr, "Error: expected 1 argument, got %d\n", argc-1);
    exit(1);
  }
  char* color = "\033[32m";
  char* rcolor = "\033[0m";
  char fcom[128];
  if (strcmp(getenv("TERM"), "xterm-256color")) {
    color = "\0";
    rcolor = "\0";
  }
  // Format the disk and create it if it doesn't exist
  printf("Formatting %s%s%s...\n", color, argv[1], rcolor);
  sprintf(fcom, "touch %s", argv[1]); system(fcom);
  sprintf(fcom, "truncate %s -s 64k", argv[1]); system(fcom);
  sprintf(fcom, "./mkfs.govnfs %s", argv[1]); system(fcom);

  // Compile GovnOS
  printf("Compiling GovnOS...\n");
  system("./kasm -export govnos/boot.asm govnos/exports/boot");
  system("./kasm govnos/boot.asm govnos/boot.bin");
  system("./kasm -d -import govnos/exports/boot govnos/kernel.asm govnos/kernel.bin");
  system("./kasm -d -import govnos/exports/boot govnos/info.asm govnos/info.bin");
  system("./kasm -d -import govnos/exports/boot govnos/date.asm govnos/date.bin");
  system("./kasm -d -import govnos/exports/boot govnos/gsfetch.asm govnos/gsfetch.bin");
  system("./kasm -d govnos/scrclr.asm govnos/scrclr.bin");

  // Load GovnOS
  printf("Loading GovnOS into %s%s%s...\n", color, argv[1], rcolor);
  sprintf(fcom, "./gboot %s govnos/boot.bin", argv[1]); system(fcom);
  sprintf(fcom, "./gload %s govnos/kernel.bin \"kernel.bin\"", argv[1]); system(fcom);
  sprintf(fcom, "./gload %s govnos/info.bin \"info\"", argv[1]); system(fcom);
  sprintf(fcom, "./gload %s govnos/date.bin \"date\"", argv[1]); system(fcom);
  sprintf(fcom, "./gload %s govnos/gsfetch.bin \"gsfetch\"", argv[1]); system(fcom);
  sprintf(fcom, "./gload %s govnos/scrclr.bin \"scrclr\"", argv[1]); system(fcom);
  return 0;
}

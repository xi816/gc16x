#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define ptrlen(t) (sizeof(t)/sizeof(t[0]))

int32_t main(void) {
  char* color = "\033[32m";
  char* rcolor = "\033[0m";
  if (strcmp(getenv("TERM"), "xterm-256color")) {
    color = "\0";
    rcolor = "\0";
  }
  char* targets[] = {"prepare-disk", "gc16", "gasman", "gboot", "ugovnfs"};
  char* builddirs[] = {"core/", "./", "core/gasman", "core/gboot", "core/ugovnfs"};
  char* commands[] = {
    "gcc prepare-disk.c -o ../prepare-disk",
    "gcc core/main.c -Ilib -lSDL2 -o gc16",
    "gcc main.c -I../../lib -o ../../gasman",
    "gcc main.c -o ../../gboot",
    "gcc main.c -I../../lib -o ../../ugovnfs"
  };

  char fcom[256];
  printf("rebuilding %sball%s\n", color, rcolor);
  system("gcc core/ball.c -o ball");
  for (uint16_t i = 0; i < ptrlen(targets); i++) {
    printf("building %s%s%s\n", color, targets[i], rcolor);
    sprintf(fcom, "cd %s; %s; cd - > /dev/null", builddirs[i], commands[i]);
    system(fcom);
  }
  return 0;
}

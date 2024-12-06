#include <stdio.h>
#include <holyc-types.h>

U8 listPart(char* disk) {
  if () {}
  return 0;
}

// CLI tool to make GovnFS partitions
U8 main(I32 argc, I8** argv) {
  puts("UGovnFS 1.0");
  if (argc == 1) {
    puts("ugovnfs: no arguments given");
    return 1;
  }
  FILE* fl = fopen(argv[2], "rb");
  fseek(fl, 0, SEEK_END);
  const U32 flsize = ftell(fl);
  char disk[flsize];
  fseek(fl, 0, SEEK_SET);
  fread(disk, 1, flsize, fl);
  fclose(fl);
  if (!strcmp(argv[1], "-l")) {
    return listPart(disk);
  }
  puts("ugovnfs: no arguments given");
  return 0;
}


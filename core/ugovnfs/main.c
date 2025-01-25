#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <holyc-types.h>

U8* get_gfs_label(U8 magicbyte) {
  switch (magicbyte) {
    case 0x89: return "GovnFS 1.3";
    default:   return "Unknown filesystem";
  }
}

// The header is the first 32 bytes of the disk
U8 readHeader(U8* disk) {
  puts("Disk info:");
  printf("  Magic byte: %02X\n", disk[0x00]);
  printf("  Filesystem: %s\n", get_gfs_label(disk[0x00]));
  printf("  Filesystem label: \"");
  fflush(stdout);
  write(1, disk+0x01, 11);
  printf("\"\n  Serial number: %02X%02X%02X%02X\n", disk[0x0C], disk[0x0D], disk[0x0E], disk[0x0F]);
  printf("  Disk size (sectors): %d\n", disk[0x10]);
  return 0;
}

// CLI tool to make GovnFS partitions
U8 main(I32 argc, I8** argv) {
  // puts("UGovnFS 1.0");
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
  if (!strcmp(argv[1], "-i")) {
    return readHeader(disk);
  }
  puts("ugovnfs: no arguments given");
  return 0;
}


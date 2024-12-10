#include <stdio.h>
#include <string.h>

#include <holyc-types.h>

// The header is the first 32 bytes of the disk
U8 readHeader(U8* disk) {
  puts("Disk info:");
  printf("  Magic byte: %02X\n", disk[0x00]);
  printf("  Filesystem: \"%c%c%c%c%c%c%c%c%c%c%c\"\n",
    disk[0x01], disk[0x02], disk[0x03], disk[0x04], disk[0x05], disk[0x06],
    disk[0x07], disk[0x08], disk[0x09], disk[0x0A], disk[0x0B]);
  printf("  Serial number: %02X%02X%02X%02X\n", disk[0x0C], disk[0x0D], disk[0x0E], disk[0x0F]);
  printf("  Disk size (sectors): %02X%02X\n", disk[0x10], disk[0x11]);
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


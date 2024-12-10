#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define U0 void
#define U8 unsigned char
#define U16 unsigned short int
#define U32 unsigned int
#define U64 unsigned long long int

#define I8 char
#define I16 short int
#define I32 int
#define I64 long long int

// Header defining the GovnFS version
U8 GFS_HEADER_FULL[]   = "\x89GOVNFS1.2  \x14\x9E\xA1\x55";
// First disk partition
// Name: /boot/
// ID: 0000h
// Size: 4KB
U8 GFS_SIZE[]          = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\xFF\xFF\xFF\xF9/boot/     \x00\x00";
U8 GFS_BOOT_PART_END[] = "\xFF\xFF\xFF\xF9\xFF\xFF\xFF\xF4";

U0 blocksIn(U16 blocks) {
  printf("%d blocks read\n", blocks);
}

U0 blocksOut(U16 blocks) {
  printf("%d blocks written\n", blocks);
}

U16 writeHeader(U8* buf, U16 startAddr, U32 diskSize) {
  memcpy(buf+startAddr, GFS_HEADER_FULL, sizeof(GFS_HEADER_FULL));
  GFS_SIZE[0] = diskSize / 256;
  GFS_SIZE[1] = diskSize % 256; // 2 bytes for disk size and 14 bytes reserved
  memcpy(buf+startAddr+16, GFS_SIZE, sizeof(GFS_SIZE));
}

U16 writeBootPartitionEnd(U8* buf, U16 startAddr) {
  memcpy(buf+startAddr, GFS_BOOT_PART_END, sizeof(GFS_BOOT_PART_END));
}

I32 main(I32 argc, I8** argv) {
  printf("mkfs.govnfs 1.0\n");
  U16 blocks = 0; // Max drive size is limited to 32MB
  U16 startInd = 0;
  if (argc == 1) {
    fprintf(stderr, "No file given\n");
    blocksIn(blocks);
    return 1;
  }
  FILE* drvfile = fopen(argv[1], "r+b");
  if (!drvfile) {
    fprintf(stderr, "File %s not found\n", argv[1]);
    blocksIn(blocks);
    return 1;
  }
  fseek(drvfile, 0, SEEK_END); // Read the drive
  U32 drvlen = ftell(drvfile);
  U8 drvbuf[drvlen];
  blocks = drvlen/512;
  fseek(drvfile, 0, SEEK_SET);
  fread(drvbuf, 1, drvlen, drvfile);
  blocksIn(blocks);

  writeHeader(drvbuf, startInd, blocks);
  for (U32 bus = 0; bus < 4096; bus++) {
    drvbuf[bus+startInd+49] = 0;
  }
  writeBootPartitionEnd(drvbuf, 4096+startInd+49);

  fseek(drvfile, 0, SEEK_SET);
  U16 blockso = fwrite(drvbuf, 1, sizeof(drvbuf), drvfile)/512;
  blocksOut(blockso);
  fclose(drvfile);

  return 0;
}


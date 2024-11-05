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

I8 GFS_HEADER_FULL[] = "\x89GOVNFS1.0  \x12\x34\xA4\x20\x7F"; // 7F is File Table End

U0 blocksIn(U16 blocks) {
  printf("%d blocks read\n", blocks);
}

U0 blocksOut(U16 blocks) {
  printf("%d blocks written\n", blocks);
}

U16 writeHeader(U8* buf, U16 startAddr) {
  memcpy(buf+startAddr, GFS_HEADER_FULL, sizeof(GFS_HEADER_FULL));
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

  writeHeader(drvbuf, startInd);
  for (U32 bus = startInd+sizeof(GFS_HEADER_FULL); bus < sizeof(drvbuf); bus++) {
    drvbuf[bus] = 0;
  }

  fseek(drvfile, 0, SEEK_SET);
  U16 blockso = fwrite(drvbuf, 1, sizeof(drvbuf), drvfile)/512;
  blocksOut(blockso);
  fclose(drvfile);

  return 0;
}

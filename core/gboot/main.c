#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define U0 void
#define U8 unsigned char
#define U16 unsigned short int
#define U32 unsigned int
#define U64 unsigned long long int

#define I8 char
#define I16 short int
#define I32 int
#define I64 long long int

I8 status(bool stat) {
  if (stat) {
    printf("Bootable binary written succesfully\n");
    return 0;
  }
  else {
    printf("Bootable binary was not written\n");
    return 1;
  }
}

I32 main(I32 argc, I8** argv) {
  printf("GBoot 1.0\n");
  U16 BootSectorStart = 0x8000;
  if (argc == 1) {
    fprintf(stderr, "No files given\n");
    return status(false);
  }
  else if (argc == 2) {
    fprintf(stderr, "No bootable GovnBin code given\n");
    return status(false);
  }
  FILE* drvfile = fopen(argv[1], "r+b");
  FILE* bsfile = fopen(argv[2], "rb");
  if (!drvfile) {
    fprintf(stderr, "Drive %s not found\n", argv[1]);
    return status(false);
  }
  if (!bsfile) {
    fprintf(stderr, "Bootable binary %s not found\n", argv[2]);
    return status(false);
  }
  fseek(drvfile, 0, SEEK_END);
  U32 drvlen = ftell(drvfile);
  U8 drvbuf[drvlen];
  fseek(drvfile, 0, SEEK_SET);
  fread(drvbuf, 1, drvlen, drvfile);

  fseek(bsfile, 0, SEEK_END);
  U32 bslen = ftell(bsfile)+1;
  U8 bsbuf[bslen];
  bsbuf[bslen-1] = 0;
  fseek(bsfile, 0, SEEK_SET);
  fread(bsbuf, 1, bslen, bsfile);

  printf("Writing to disk (%d bytes of disk storage, %d bytes of boot sector)\n", drvlen, bslen);
  memcpy(drvbuf+BootSectorStart, bsbuf, bslen);
  fseek(drvfile, 0, SEEK_SET);
  fwrite(drvbuf, 1, drvlen, drvfile);

  fclose(drvfile);
  fclose(bsfile);

  return status(true);
}

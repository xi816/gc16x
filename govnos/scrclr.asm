; Clear the VGA
scrclr:
  lds $0000
  lda $00
  ldc 64600
.loop:
  int $0C
  inx %si
  loop .loop
  int $11
  ret

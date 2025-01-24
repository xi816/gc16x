start:
  lda $9ABC ; White
  lds $0000 ; Write to the videobuffer from $0000
  jmp start2
start1:
  inx %ax
  jmp end
start2:
  ldc 339
white:
  int $0C ; Write a pixel to the screen
  inx %si
  loop white

  int $11 ; Videobuffer update
end:
  int $01
  pop %bx
  cmp %bx $20
  jme start2
  cmp %bx $61
  jme start1
  push $00
  int $00

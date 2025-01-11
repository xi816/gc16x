start:
  lda $9ABC ; White
  lds $0000 ; Write to the videobuffer from $0000
  jmp start2
start1:
  inx %a
  jmp end
start2:
  ldc #339
white:
  int $0C ; Write a pixel to the screen
  inx %s
  loop white
int $11 ; Videobuffer update
end:
int $01
pop %b
cmp %b $20
jme start2
cmp %b $61
jme start1

push $00
int $00


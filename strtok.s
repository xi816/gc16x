jmp main

strtok:
  ldg $01
  add %s %g
  ldd %s
  lodsb
  ldc %s
  lds %d
  sub %c %b
  inx dynptr
  ldd $00
  loop strtok
  ret

main:
  lds str
  ldb $20
  call strtok

  lda #48
  lds *dynptr
  add %s %a
  push %s
  int $02
  push $00
  int $00

str: bytes "Hi World$^@"
dynptr: bytes $00


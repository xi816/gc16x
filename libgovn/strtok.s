jmp main

strtok:
  add %s $01
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

  lds *dynptr
  add %s #48
  push %s
  int $02
  push $0A
  int $02
  push $00
  int $00

str: bytes "Hi World$^@"
dynptr: bytes $00


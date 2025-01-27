call start
  push $00
  int $00
puthc:
  div %a #16
  ldg hex-numbers
  add %g %a
  lodgb
  push %g
  int $02
  ldg hex-numbers
  add %g %d
  lodgb
  push %g
  int $02
  ret
hex-numbers: bytes "0123456789ABCDEF"

start:
  lds $0000
main:
  ldds
  call puthc

  push $20
  int $02
  inx %s

  ldb %s
  div %s #32
  lds %b
  cmp %d $00
  jme enter

  ; int $01
  ; pop %b
  jmp main

  push $00
  int $00
enter:
  push $0A
  int $02
  jmp main


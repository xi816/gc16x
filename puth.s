jmp main
puth:
  cmp %a $00
  re
  div %a #16
  ldg hex-numbers
  add %g %d
  lodgb
  push %g
  int $02
  jmp puth
hex-numbers: bytes "0123456789ABCDEF"

main:
  lda $4A69
  call puth

  push $0A
  int $00
  push $00
  int $00


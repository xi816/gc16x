main:
  ldd #100
lp:
  push 'A'
  int $02
  int $22
  jmp lp


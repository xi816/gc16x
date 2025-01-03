jmp Rom
puts:
  ldd %s
  lodsb
  push %s
  int $02
  cmp %s $00
  lds %d
  inx %s
  jmne puts
  ret

Rom:
  lds StartMsg
  call puts

  lds KeypressMsg
  call puts
  int $01

  push $00
  int $00

StartMsg: bytes "Simple ROM 0.1$^@"
KeypressMsg: bytes "Press a key to end$^@"


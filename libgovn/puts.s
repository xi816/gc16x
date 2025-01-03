jmp main
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

main:
  lds msg
  call puts

  push $00
  int $00

msg: bytes "If you see this message,$puts is working!! yay!!$^@"


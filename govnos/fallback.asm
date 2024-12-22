jmp fallback

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

fallback:
  lds fallback-msg
  call puts
  hlt

fallback-msg: bytes "The system could not load$"
              bytes "It is running in fallback mode$^@"


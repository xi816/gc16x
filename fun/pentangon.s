  jmp main
puts:
  cmp *%si $00
  re
  push *%si
  int $02
  inx %si
  jmp puts
hack_pentangon:
  push $E2
  int $02
  push $96
  int $02
  push $88
  int $02
  int $22
  loop hack_pentangon
  ret
main:
  lds m0
  call puts
  ldc 20
  ldd 200
  call hack_pentangon
  lds m1
  call puts

  push 0
  int 0
m0: bytes "Hacking Pentangon...$  [^@"
m1: bytes "]$Pentalgin hacked successfully$^@"

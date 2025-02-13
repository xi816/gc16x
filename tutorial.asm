reboot: jmp main

; puts - Print a string to stdout
; Arguments:
; si - String address
puts:
  cmp *%si $00
  re
  push *%si
  int 2
  inx %si
  jmp puts
main:
  lds s0
  call puts
  push 0
  int 0
s0: bytes "Превед, Мир!$^@"

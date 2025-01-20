jmp main

puts:
  cmp *%si $00
  re
  push *%si
  int $02
  inx %si
  jmp puts

; AX = otkuda
; BX = kuda
; konec

; 20 21 22 23 24 25 26
; --------------------
; 01 02 00 01 02 00 00
;       ^^       ^^
;       AX       BX
; si = $00
; gi = $25
;

strcpy:
  lds %ax
  lodsb
  ldg %bx
  cmp %si $00 ; Target has no more bytes to copy
  re
  stgrb %si
  inx %ax
  inx %bx
  jmp strcpy

main:
  lda str_a
  ldb str_b
  call strcpy

  lda str_c
  call strcpy

  push $00
  int $00

str_a: bytes "Hello^@, World!$^@"
str_b: bytes "Bye, World!$^@"


jmp main

; scani - Scan an integer from standard input
; Returns:
; A - number
scani:
  lda $00
scani-lp:
  int $01
  pop %b
  cmp %b $0A ; Check for Enter
  re
  cmp %b $20 ; Check for space
  re
  cmp %b $00 ; Check for NUL ($00)
  re
  mul %a #10

  push %b
  int $02
  sub %b #48
  add %a %b
  jmp scani-lp

main:
  call scani
  push $0A
  int $02

  push %a
  int $02
  push $0A
  int $02

  push $00
  int $00


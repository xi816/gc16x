jmp main

puts:
  ldd %si
  lodsb
  push %si
  int $02
  cmp %si $00
  lds %dx
  inx %si
  jmne puts
  ret

main:
  lds askgod
  call puts
  call input
  jmp answer-lb
  push $0A
  int $02

  push $00
  int $00
input:
  int $01    ; Get character from input
  pop %dx     ; Save to D register
  cmp %dx $7F ; Check for Backspace (1)
  jme input-bs
  cmp %dx $08 ; Check for Backspace (2)
  jme input-bs
  push %dx    ; Output the
  int $02    ; character
  lds question
  ldg *qptr
  add %si %gi
  storb %dx
  inx qptr
  cmp %dx $0A ; Check for Enter
  re
  jmp input
input-bs: ; Handle backspace ($7F or $08)
  ldg *qptr
  cmp %gi $00
  jme input
input-bs-strict:
  dex qptr
  lds bs-seq
  call puts
  jmp input

answer-lb:
  ldc 200
  answer:
    int $21
    push %dx
    int $02
    loop answer
  jmp main

askgod:   bytes "Ask God a question: ^@"
bs-seq:   bytes "^H ^H^@"
question: reserve 64 bytes
qptr:     reserve 1 bytes


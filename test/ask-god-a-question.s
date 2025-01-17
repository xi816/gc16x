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
  pop %d     ; Save to D register
  cmp %d $7F ; Check for Backspace (1)
  jme input-bs
  cmp %d $08 ; Check for Backspace (2)
  jme input-bs
  push %d    ; Output the
  int $02    ; character
  lds question
  ldg *qptr
  add %s %g
  storb %d
  inx qptr
  cmp %d $0A ; Check for Enter
  re
  jmp input
input-bs: ; Handle backspace ($7F or $08)
  ldg *qptr
  cmp %g $00
  jme input
input-bs-strict:
  dex qptr
  lds bs-seq
  call puts
  jmp input

answer-lb:
  ldc #200
  answer:
    int $21
    push %d
    int $02
    loop answer
  jmp main

askgod:   bytes "Ask God a question: ^@"
bs-seq:   bytes "^H ^H^@"
question: reserve #64 bytes
qptr:     reserve #1 bytes


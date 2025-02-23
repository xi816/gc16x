reboot: jmp main
; write - Output the buffer
; Arguments:
; si - buffer address
; cx - number of characters to output
write:
  ldd $00
  sub %cx $01
.loop:
  push *%si
  int $02
  inx %si
  loop .loop
  ret

; inttostr - Convert a 16-bit integer into a string
; Arguments:
; ax - Number
inttostr:
  ldg inttostr_buf
  add %gi $04
.loop:
  div %ax 10 ; Divide and get the remainder into D
  add %dx 48 ; Convert to ASCII
  lds %gi
  storb %dx
  dex %gi
  cmp %ax $00
  jmne .loop
  ret
inttostr_buf: reserve 5 bytes

inttostr_clr:
  ldc $4
  ldd $00
  lds inttostr_buf
.loop:
  storb %dx
  inx %si
  loop .loop
  ret

; puti - Output a 16-bit integer number
; Arguments:
; ax - Number
puti:
  call inttostr
  lds inttostr_buf
  ldc $05
  call write
  call inttostr_clr
  ret

main:
  lda 5
  bytes $74 $00 $04
  call puti ; outputs pow(5, 4)
  push '$'
  int 2

  push 0
  int 0

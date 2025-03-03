; libgovn/std.s
  call main

  lds endmsg
  push %si
  call puts
  pop %ax

  ; exit code left on stack
  int $0
endmsg: bytes "Destroying GovnPy runtime data...$^@"
; GovnPy Standard Library
puti:
  pop %bp
  pop %ax
  cmp %ax 0
  jme .zer
  ldb 10000
  ldg 0
.loop:
  ldc %ax
  div %ax %bx
  div %ax 10
  cmp %dx %gi
  jme .next
  add %dx $30
  ldg %dx ; so that cmp fails
  push %dx
  int 2
.next:
  lda %cx
  cmp %bx 1
  jme .end
  div %bx 10
  jmp .loop
.zer:
  push $30
  int 2
.end:
  push 0
  push %bp
  ret

puts:
  pop %bp
  pop %si
.loop:
  cmp *%si 0
  jme .end
  push *%si
  int 2
  inx %si
  jmp .loop
.end:
  push %si
  push %bp
  ret

gets:
  pop %bp
  pop %si
  ldg %si
.loop:
  int 1
  pop %ax
  cmp %ax $1B
  jme .esc
  cmp %ax 8
  jme .bs
  cmp %ax 127
  jme .bs
  push %ax
  int 2
  cmp %ax 10
  jme .end
  storb %ax
  inx %si
  jmp .loop
.esc:
  lda 94
  push %ax
  int 2
  storb %ax
  inx %si

  lda '['
  push %ax
  int 2
  storb %ax
  inx %si
jmp .loop

.bs:
  cmp %si %gi
  jme .loop
  lda 0
  storb %ax
  dex %si
  ; Check if we're overwriting a non-first byte
  lda *%si
  div %ax 64
  cmp %ax 2
  jme .bs
  push 8
  int $2
  push 32
  int $2
  push 8
  int $2
  jmp .loop
.end:
  lda 0
  storb %ax
  push %si
  push %bp
  ret
exit:
  pop %bp
  int $0
  hlt

memcmp:
  pop %bp
  pop %si
  pop %gi
  pop %cx
  dex %cx
.loop:
  lda *%si
  ldb *%gi
  cmp %ax %bx
  jmne .fail
  inx %si
  inx %gi
  loop .loop
  push 1
  push %bp
  ret
.fail:
  push 0
  push %bp
  ret
scmp:
  pop %bp
  pop %si
  pop %gi
.loop:
  lda *%si
  ldb *%gi
  cmp %ax %bx
  jmne .fail
  cmp %bx 0
  jme .ok
  inx %si
  inx %gi
  jmp .loop
.fail:
  push 0
  push %bp
  ret
.ok:
  push 1
  push %bp
  ret

s2i:
  pop %bp
  pop %si
  lda 0
.loop:
  ldb *%si
  cmp %bx 0
  jme .end
  sub %bx $30
  mul %ax 10
  add %ax %bx
  inx %si
  jmp .loop
.end:
  push %ax
  push %bp
  ret

putc:
  pop %bp
  int 2
  push 0
  push %bp
  ret

getch:
  pop %bp
  int 1
  push %bp
ret
msleep:
  pop %bp
  pop %dx
  int $22
  push 0
  push %bp
  ret

rand:
  pop %bp
  int $21
  push %dx
  push %bp
ret

main:
  mov %bp 0
  add %sp %bp
  mov %bp %sp
  inx %bp
  push %bp
  mov %ax str0
  push %ax
  call puts
  pop %ax
  pop %bp
  push 0
.ret:
  pop %dx
  lds 2
  add %sp %si
  lds 0
  add %si %bp
  lodsw
  push %dx
  push %ax
  ret
str0: bytes "Hello, World!^@"

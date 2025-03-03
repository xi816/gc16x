  call init
  pop %ax
rtloop:
  call frame
  pop %ax
  cmp %ax $0
  jmne .end
  inx cur_frame
  lda *cur_frame
  cmp %ax 30
  jmne .in_second
  lds cur_frame
  lda 0
  storb %ax
.in_second:
  lds magic
  push %si
  call puts
  pop %ax
  ldd 32
  int $22
.iloop:
  int 1
  pop %ax
  cmp %ax $1B
  jme .eloop
  push %ax
  call on_key
  pop %ax
  cmp %ax $0
  jme .iloop
.end:
  push %ax
  int 0
.eloop:
  int 1
  pop %ax
  cmp %ax 'R'
  jmne .eloop
  jmp rtloop

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

rand:
  pop %bp
  int $21
  push %dx
  push %bp
ret

magic: bytes "^[[6n^@"
cur_frame: bytes $00

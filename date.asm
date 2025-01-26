main:
  int 3
  ; DX = date
  lds %dx
  ; Year
  lda %si
  div %ax 371
  add %ax 1970
  ldb 1000
  call puti

  push $2D
  int 2

  ; Month
  lda %si
  div %ax 31
  div %ax 12
  lda %dx
  inx %ax
  ldb 10
  call puti

  push $2D
  int 2

  ; Day
  lda %si
  div %ax 31
  lda %dx
  inx %ax
  ldb 10
  call puti

  push $A
  int 2
push 0
int 0
hlt
puti: ; Use B,C,D,G
; ldb 10000 User sets this
.loop:
  ldc %ax
  div %ax %bx
  div %ax 10
  add %dx $30
  push %dx
  int 2
  lda %cx
  cmp %bx 1
  re
  div %bx 10
  jmp .loop

  mov %ax, $1111
inner_loop:
  mov %si, 0
  mov %cx, 64600
main:
  int $0C
  inx %si
  loop main
  int $11

  add %ax, $1111
  jmp inner_loop

  push 0
  int 0

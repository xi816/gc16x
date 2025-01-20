start:
  /*
  int 1
  pop %ax
  */
  mov %ax, 64
  cmp %ax, 64
  jme .a
  jmp .b
.a:
  push $41
  int 2
  jmp end
.b:
  push $42
  int 2
  jmp end
end:
  push 0
  int 0


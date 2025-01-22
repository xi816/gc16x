// 01  23
   jmp main
//           456789ABCDEF0123
smth: bytes "ABCDEFGHIJKLMNOP"

main:
  mov %si, $0006 // Should load "DC" into %ax
  trap
  bytes $8A
  trap

  push 0
  int 0

retry: jmp main

puts:
  cmp *%si $00
  re
  push *%si
  int 2
  inx %si
  jmp puts

main:
  mov %si, hw_kernel
  call puts
  ret

hw_kernel: bytes "Hello from Game!$THE GOVNGAME!!!$^@"


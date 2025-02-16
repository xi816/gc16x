  lds ruski00
  ldd 600
  jmp main
puts:
  cmp *%si $00
  jme term
  push *%si
  int 2
  inx %si
  cmp *%si $0A
  re
  jmp puts
term:
  push 0
  int 0
main:
  call puts
  int $22
  jmp main
ruski00: bytes "^[[91mЙа руске$"
         bytes "Йа идо да канса$"
         bytes "Йа руске$"
         bytes "Мая кров ат аца$"
         bytes "Йа руске$"
         bytes "И мне пависло$"
         bytes "Йа руске$"
         bytes "Всим лютям на сло$^@"

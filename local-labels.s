main:
  jmp .b
  .a:
    push #52
    int $00
  .b:
    push #1488
    int $00


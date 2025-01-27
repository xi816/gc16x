main:
  mov %ax, $0060
  mov %si, 0
  mov %cx, 64600
  call bg_fill

bg_fill:
  int $0C
  inx %si
  loop bg_fill
  int $11

  call draw_panel
  jmp program_loop

draw_panel:
  mov %ax, *color
  mov %si, 64600
  sub %si, 6800
  mov %cx, 6800
.loop:
  int $0C
  inx %si
  loop .loop
  int $11
  ret

program_loop:
  call draw_panel
  int $01
  pop %ax

  cmp %ax, 'q' ; Quit
  jme term

  ; cmp %ax, '$' ; Draw a square

  cmp %ax, 'w' ; Go up
  jme up
  cmp %ax, 'a' ; Go left
  jme left
  cmp %ax, 's' ; Go down
  jme down
  cmp %ax, 'd' ; Go right
  jme right
  cmp %ax, 'x' ; Change the color to previous
  jme prev_color
  cmp %ax, 'c' ; Change the color to next
  jme next_color

  jmp program_loop

square:
  mov %si, *x
  mov %bx, *y
  mul %bx, 340
  add %si, %bx   ; Address to write
  mov %ax, *color
  int $0C
  inx %si
  int $0C
  add %si, 340
  int $0C
  dex %si
  int $0C
  sub %si, 340

  int $11
  ret
up:
  dex y
  dex y
  call square
  jmp program_loop
down:
  inx y
  inx y
  call square
  jmp program_loop
left:
  dex x
  dex x
  call square
  jmp program_loop
right:
  inx x
  inx x
  call square
  jmp program_loop
prev_color:
  dex color
  mov %bx, *color
  cmp %bx, $FF
  jmne program_loop
  mov %si, color
  mov %ax, $0F
  storb %ax
  jmp program_loop
next_color:
  inx color
  mov %bx, *color
  cmp %bx, $10
  jmne program_loop
  mov %si, color
  mov %ax, $00
  storb %ax
  jmp program_loop
term:
  push 0
  int 0

x:     reserve 1 bytes ; 256x256 drawing square
y:     reserve 1 bytes
color: reserve 1 bytes

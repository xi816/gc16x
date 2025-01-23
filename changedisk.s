mov %cx, 10
mov %si, $0000
aloop:
  bytes $10 $AB $02 // stds cx -- Write to the disk
  inx %si
  loop aloop
push 0
int 0

bytes $AA $55


int 1
pop %ax

cmp %ax, 40
jl shit

push 'V'
int 2

shit:
  push 'Z'
  int 2

push 0
int 0


main:
  lda 69
  trap
  bytes $88 $20
  trap
  bytes $88 $20
  trap

  push 0
  int 0

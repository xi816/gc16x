assert0:
  pop %bp
  pop %ax
  cmp %ax 0
  jme .ok
  lds assert_msg
  push %si
  call puts
  push $0
  int $0
.ok:
  push 0
  push %bp
ret
assert_msg: bytes "^[[31mAssertion failed!^[[0m$^@"

get52:
  pop %bp
  push 52
  push %bp
ret

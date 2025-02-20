jmp get_main
get_main:
  lda cline
  ldd $4000
  mov %fx txt_file_sign
  call gfs_read_file
  cmp %bx $01
  jme get_error
  lds $4000
  call puts
  ret
get_error: bytes "get: no such file or directory$^@"

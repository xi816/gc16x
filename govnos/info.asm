start: jmp infomain
infomain:
  lds OS_RELEASE
  call puts
  ret

OS_RELEASE: bytes "^[[96m"
            extern "govnos/info.ans"
            bytes "^[[0m^@"

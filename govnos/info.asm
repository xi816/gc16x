start: jmp infomain
puts:
  cmp *%si $00
  re
  push *%si
  int $02
  inx %si
  jmp puts
infomain:
  lds OS_RELEASE
  call puts
  ret

OS_RELEASE: bytes "^[[96mGovnOS version 0.0.4 (alpha)$"
            bytes "Release date: 2025-02-02"
            bytes "$(c) Xi816, 2025"
            bytes "^[[0m$^@"

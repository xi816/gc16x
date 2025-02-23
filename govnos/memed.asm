; Memory editor
memed_reboot: jmp memed
; idk how many scans do i need, this is the third one
fscans:
  push %si
.loop:
  int 1
  cop %ax
  int 2
  cmp %ax $00
  jme .end
  cmp %ax $0A
  jme .end
  storb %ax
  inx %si
  inx %cx
  jmp .loop
.end:
  dex %si
  storb %ax
  pop %si
  ret
memed:
  lds enter_m00
  call puts
  call scani
  call fscans
  push '$'
  int 2
  lds alter_m01
  call puts
  ret
enter_m00: bytes "Enter address: ^@"
alter_m01: bytes "Memory has been ^[[33maltered^[[0m$^@"

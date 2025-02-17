reboot_reboot: jmp reboot_main
reboot_main:
  lds m0_reb
  call puts
  ldd 300 ; wait(300) just because i can
  int $22
  lds cls_seq
  call puts
  int 4

m0_reb: bytes "Rebooting now...$^@"

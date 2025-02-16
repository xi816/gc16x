reload_kernel: jmp kshell

; kscans - Kernel func for scanning a string from standard input
; si - buffer address
kscans:
  int $01    ; Get character from input
  pop %dx

  cmp %dx $7F ; Check for Backspace
  jme .backspace
  cmp %dx $08
  jme .backspace

  push %dx    ; Output the character
  int $02

  ldg *qptr  ; Save into memory
  add %si %gi
  storb %dx
  sub %si %gi
  inx qptr

  cmp %dx $0A ; Return if pressed Enter
  re
  jmp kscans
.backspace: ; Handle backspace ($7F or $08)
  ldg *qptr
  cmp %gi $00
  jme kscans
.backspace_strict:
  dex qptr
  push %si
  lds bs_seq
  call puts
  pop %si
  jmp kscans

kshell:
  lds cls_seq
  call puts
  lds welcome_shell
  call puts
.prompt: ; Print the prompt
  lds kenv_PS
  call puts
.input:
  lds cline
  call kscans
  jmp .process
.term:
  push $00
  int $00
.process:
  lds cline
  ldg *qptr
  add %si %gi
  dex %si
  str $00 ; Load $00 (NUL) instead of $0A (Enter)

  ; reboot
  lda cline
  ldb kinst_reboot
  call strcmp
  cmp %ax $00
  jme kexec_reboot

  jmp .prompt
kexec_reboot:
  int 4

kenv_PS:       bytes "69^$ ^@"
welcome_shell: bytes "^[[92mGovnOS 0.0.4^[[0m$GovnOS Shell 1.0$$^@"

kinst_reboot:  bytes "reboot^@"

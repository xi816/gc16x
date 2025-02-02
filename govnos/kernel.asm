retry: jmp shell

puts:
  cmp *%si $00
  re
  push *%si
  int 2
  inx %si
  jmp puts

scans:
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
  jmp scans
.backspace: ; Handle backspace ($7F or $08)
  ldg *qptr
  cmp %gi $00
  jme scans
.backspace_strict:
  dex qptr
  push %si
  lds bs_seq
  call puts
  pop %si
  jmp scans

shell:
  lds cls_seq
  call puts
  lds welcome_shell
  call puts
.prompt: ; Print the prompt
  lds env_PS
  call puts
.input:
  lds cline
  call scans
  jmp .process
.term:
  push $00
  int $00
.process:
  push $0A
  int 2
  jmp .prompt

env_PS:        bytes "@/^$ ^@"

bs_seq:        bytes "^H ^H^@"
cls_seq:       bytes "^[[H^[[2J^@"

welcome_shell: bytes "^[[92mGovnOS 0.0.4^[[0m$GovnOS Shell 1.0$$^@"
cline:         reserve 64 bytes
qptr:          reserve 1 bytes

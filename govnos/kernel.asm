reload_kernel: jmp kshell

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
  call scans
  jmp .process
.term:
  push $00
  int $00
.process:
.aftexec:
  jmp .prompt

kenv_PS:       bytes "69^$ ^@"
welcome_shell: bytes "^[[92mGovnOS 0.0.4^[[0m$GovnOS Shell 1.0$$^@"

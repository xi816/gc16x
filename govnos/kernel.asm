reload_kernel: jmp shell

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
  lda cline
  ldb instFULL_date
  call strcmp
  cmp %ax $00
  jme com_govnosEXEC_date
.aftexec:
  jmp .prompt

welcome_shell: bytes "^[[92mGovnOS 0.0.4^[[0m$GovnOS Shell 1.0$$^@"

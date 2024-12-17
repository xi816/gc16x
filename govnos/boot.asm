jmp boot

; GovnFS signature bytes that are not allowed in code:
; F0, F1, F2, F7
; F0 -- Directory block
; F1 -- File block
; F2 -- Group block
; F7 -- Disk end
;
; Dir: F0 [name] F0
; File: F1 [filename] F2 [dir] F2 [contents] F1

puts:
  lda $00    ; A = $00
  ldd %s     ; G = S
  lodsb      ; S = [S]
  push %s    ; PUSH(S)
  int $02    ; INT(2)
  cmp %s %a  ; SEI = (S == A)
  lds %d     ; S = D
  inx %s     ; S++
  jmne puts  ; IF (!SEI) PC = PUTS
  ret        ; RETURN

boot:
  lds st-msg
  call puts
  ; hlt
boot-start:
  ldd $00
  cpuid
  ; Start the shell
  jmp com-govnos

  jmp fail

fail:
  lds fail-msg
  call puts
  hlt

com-govnos:
  lds welcome-msg
  call puts
  ldc $0A
  jmp com-govnos-prompt
com-govnos-prompt:
  lds env-PS
  call puts
com-govnos-input:
  int $01
  cop %d
  push %d
  int $02
  cmp %d %c
  jmne com-govnos-input
com-govnos-process:
  lds bad-inst-msg
  call puts
  jmp com-govnos-prompt
com-govnos-term:
  int $00

st-msg: bytes "GovnOS$^@"
fail-msg: bytes "Fatal error. Halted$^@"
welcome-msg: bytes "Welcome to GovnOS!$To get help, type `help`$$^@"
env-PS: bytes "^$ ^@^@^@^@^@^@^@^@^@"

full-inst-he: bytes "he^@"
bad-inst-msg: bytes "Bad command.$^@"


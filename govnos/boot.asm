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
  ldd %s     ; G = S
  lodsb      ; S = [S]
  push %s    ; PUSH(S)
  int $02    ; INT(2)
  cmp %s $00 ; SEI = (S == 0)
  lds %d     ; S = D
  inx %s     ; S++
  jmne puts  ; IF (!SEI) PC = PUTS
  ret        ; RETURN

; Arguments:
; A - first string address
; B - second string address
; Returns:
; H - status
strcmp:
  lds %a
  ldg %b
  lodsb
  lodgb
  cmp %s %g
  jmne strcmp-fail
  cmp %s $00
  jme strcmp-eq
  inx %a
  inx %b
  jmp strcmp
strcmp-eq:
  ldh $00
  ret
strcmp-fail:
  ldh $01
  ret

; D - directory
; G - filename
; S - address to store data from a file
gfs-read-file:
  jmp fail

boot:
  lds st-msg
  call puts
  ; hlt
boot-start:
  ; Find the current CPU
  lds procchk-msg
  call puts
  ldd $00
  cpuid
  cmp %d $00
  jme boot-gc16x-cpu
  jmp boot-unk-cpu
boot-gc16x-cpu:
  lds proc-00-msg
  call puts
  jmp boot-shell
boot-unk-cpu:
  lds proc-unk-msg
  call puts
  jmp fail
boot-shell:
  ; Start the shell
  call com-govnos
  call com-exit

fail:
  lds fail-msg
  call puts
  hlt

com-govnos:
  lds welcome-msg
  call puts
  jmp com-govnos-prompt
com-govnos-prompt:
  lds env-PS
  call puts
com-govnos-input:
  int $01    ; Get character from input
  cop %d     ; Save to D register
  push %d    ; Output the
  int $02    ; character
  lds comm
  ldg commi
  lodgb
  add %s %g
  storb %d
  inc commi
  cmp %d $0A ; Check for Enter
  jmne com-govnos-input
com-govnos-process:
  lds bad-inst-msg
  call puts
  lds comm
  call puts
  push $0A
  int $02
  jmp com-govnos-prompt
com-govnos-term:
  ret
com-exit:
  lds exit-msg
  call puts
  push $00
  int $00

; Text
st-msg:       bytes "Loading GovnOS ...$^@"
fail-msg:     bytes "Fatal error. Halted$^@"
exit-msg:     bytes "exit$^@"
welcome-msg:  bytes "Welcome to GovnOS!$To get help, type `help`$$^@"

; CPU types
procchk-msg:  bytes "[0000] Checking CPU$^@"
proc-00-msg:  bytes "[0001] CPU: Govno Core 16X$$^@"
proc-unk-msg: bytes "[0001] CPU: Unknown$$^@"

; Environment variables
env-PS:       bytes "^$ ^@^@^@^@^@^@^@^@^@"

; Commands
full-inst-he: bytes "he^@"
bad-inst-msg: bytes "Bad command.$^@"

comm:         bytes "^@^@^@^@^@^@^@^@^@^@^@^@^@^@^@^@"
commi:        bytes $00


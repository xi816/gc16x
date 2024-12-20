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
  lda $00
  ret
strcmp-fail:
  lda $01
  ret

; D - directory
; G - filename
; S - address to store data from a file
gfs-read-file:
  lds kp-1-0msg
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
  lds kp-0-0msg
  jmp fail
boot-shell:
  ; Start the shell
  call com-govnos

fail:
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
  pop %d     ; Save to D register
  cmp %d $7F ; Check for Backspace (1)
  jme com-govnos-bs
  cmp %d $08 ; Check for Backspace (2)
  jme com-govnos-bs
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
  jmp com-govnos-process
com-govnos-bs:
  ldg commi
  lodgb
  cmp %g $00
  jme com-govnos-input
com-govnos-bs-strict:
  dec commi
  lds bs-seq
  call puts
  jmp com-govnos-input
com-govnos-process:
  ldd $00
  storb %d
  ; lds comm
  ; call puts
  push $0A
  int $02
  lds commi
  ldd $00
  lodgb
  storb %d ; Load $00 (NUL) instead of $0A (Enter)

  ; he
  lda comm
  ldb instFULL-he
  call strcmp ; Assembly with your own library is easy right :D
  cmp %a $00  ; Check for 0 (equal status)
  jme com-govnosEXEC-he

  ; cls
  lda comm
  ldb instFULL-cls
  call strcmp
  cmp %a $00
  jme com-govnosEXEC-cls

  ; exit
  lda comm
  ldb instFULL-exit
  call strcmp
  cmp %a $00
  jme com-govnosEXEC-exit

  ; Otherwise it's a bad instruction
  lds bad-inst-msg
  call puts
com-govnos-aftexec:
  jmp com-govnos-prompt

; Commands
com-govnosEXEC-he:
  lds he-loaded-msg
  call puts
  jmp com-govnos-aftexec ; Go to a new task after execution

com-govnosEXEC-cls:
  lds cls-seq
  call puts
  jmp com-govnos-aftexec

com-govnosEXEC-exit:
  lds exit-term-msg
  call puts
  jmp com-govnos-term

; Shutdown and termination
com-govnos-shutdown:
  lds exit-msg
  call puts
com-govnos-term:
  push $00
  int $00

; Text
st-msg:        bytes "Loading GovnOS ...$^@"
fail-msg:      bytes "Halting execution ...$^@"
exit-msg:      bytes "$Shutting down ...$^@"
exit-term-msg: bytes "exit$^@"
welcome-msg:   bytes "Welcome to GovnOS!$To get help, type `help`$To shutdown, press Ctrl-D$$^@"
bschk:         bytes "Backspace$^@"
he-loaded-msg: bytes "`he` is running$^@"

; Kernel panic
; 0 - Processor error
; 1 - Filesystem error
; 41 - Unknown error
kp-0-0msg:     bytes "Kernel panic: Unable to find processor type(0,0)$^@"
kp-1-0msg:     bytes "Kernel panic: Unknown filesystem(1,0)$^@"
kp-41-0msg:    bytes "Kernel panic: Kernel error(41,0)$^@"

; CPU types
procchk-msg:   bytes "[0000] Checking CPU$^@"
proc-00-msg:   bytes "[0001] CPU: Govno Core 16X$$^@"
proc-unk-msg:  bytes "[0001] CPU: Unknown$$^@"

; Environment variables
env-PS:        bytes "^$ ^@^@^@^@^@^@^@^@^@"

; Control sequences
bs-seq:        bytes ^08 ^20 ^08 "^@"
cls-seq:       bytes ^1B ^5B ^48 ^1B ^5B ^32 ^4A "^@"

; Commands
instFULL-he:   bytes "he^@"
instFULL-cls:  bytes "cls^@"
instFULL-exit: bytes "exit^@"
bad-inst-msg:  bytes "Bad command.$^@"

; Buffers
comm:          bytes "^@^@^@^@^@^@^@^@^@^@^@^@^@^@^@^@"
               bytes "^@^@^@^@^@^@^@^@^@^@^@^@^@^@^@^@"
               bytes "^@^@^@^@^@^@^@^@^@^@^@^@^@^@^@^@"
               bytes "^@^@^@^@^@^@^@^@^@^@^@^@^@^@^@^@"
commi:         bytes ^00


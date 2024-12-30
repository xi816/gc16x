jmp boot

puts:
  ldd %s
  lodsb
  push %s
  int $02
  cmp %s $00
  lds %d
  inx %s
  jmne puts
  ret

boot:
  lds st-msg
  call puts
boot-start:
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
  lds st-shl-msg
  call puts
  call com-govnos
  hlt
com-govnos:
  lds welcome-msg
  call puts
  jmp com-govnos-prompt
com-govnos-prompt: ; Print the prompt
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
  inx commi
  cmp %d $0A ; Check for Enter
  jmne com-govnos-input
  hlt
  ; jmp com-govnos-process
com-govnos-bs: ; Handle backspace ($7F or $08)
  ldg commi
  lodgb
  cmp %g $00
  jme com-govnos-input
com-govnos-bs-strict:
  dex commi
  lds bs-seq
  call puts
  jmp com-govnos-input

fail:
  call puts
  hlt

st-msg:        bytes "Loading GovnOS ...$^@"
st-shl-msg:    bytes "Loading Shell ...$^@"
procchk-msg:   bytes "[0000] Checking CPU ...$^@"
welcome-msg:   bytes "Welcome to GovnOS!$To get help, type `help`$$^@"

; Kernel panic
kp-0-0msg:     bytes "Kernel panic: Unable to find processor type(0,0)$^@"

; CPU types
procchk-msg:   bytes "[0000] Checking CPU$^@"
proc-00-msg:   bytes "[0001] CPU: Govno Core 16X$$^@"
proc-unk-msg:  bytes "[0001] CPU: Unknown$$^@"

; Environment variables
; 11 bytes
env-PS:        bytes "A/^$ ^@^@^@^@^@^@^@"

; Control sequences
bs-seq:        bytes $08 $20 $08 "^@"

; Buffers
comm:          bytes "^@^@^@^@^@^@^@^@^@^@^@^@^@^@^@^@"
               bytes "^@^@^@^@^@^@^@^@^@^@^@^@^@^@^@^@"
               bytes "^@^@^@^@^@^@^@^@^@^@^@^@^@^@^@^@"
               bytes "^@^@^@^@^@^@^@^@^@^@^@^@^@^@^@^@"
commi:         bytes "^@"


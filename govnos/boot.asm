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

; write - Output the buffer
; Arguments:
; S - buffer address
; C - number of characters to output
write:
  ldd $00
  ldg %s
  lodgb
  push %g
  int $02
  inx %s
  loop write
  ret

; puts - Output string until NUL ($00)
; Arguments:
; S - string address
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

; strtok - Progress the string pointer until character
; Arguments:
; S - string address
; B - end character
; (also affects dynptr)
strtok:
  add %s $01
  ldd %s
  lodsb
  ldc %s
  lds %d
  sub %c %b
  inx dynptr
  ldd $00
  loop strtok
  ret

; inttostr - Convert a 16-bit integer into a string
; Arguments:
; A - Number
; inttostr:
;   ldg inttostr-buf
;   add %g $04
; inttostr-lp:
;   div %a #10 ; Divide and get the remainder into D
;   add %d #48 ; Convert to ASCII
;   lds %g
;   storb %d
;   dex %g
;   cmp %a $00
;   jmne inttostr-lp
;   ret
; inttostr-buf: bytes "^@^@^@^@^@"

; puti - Output a 16-bit integer number
; Arguments:
; puti:
;   call inttostr
;   lds inttostr-buf
;   ldc $05
;   call write
;   ret

; strcmp - Check if two strings are equal
; Arguments:
; A - first string address
; B - second string address
; Returns:
; A - status
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

; pstrcmp - Check if two strings are equal ending in some character
; Arguments:
; A - first string address
; B - second string address
; C - character
; Returns:
; A - status
pstrcmp:
  lds %a
  ldg %b
  lodsb
  lodgb
  cmp %s %g
  jmne pstrcmp-fail
  cmp %s %c
  jme pstrcmp-eq
  inx %a
  inx %b
  jmp pstrcmp
pstrcmp-eq:
  lda $00
  ret
pstrcmp-fail:
  lda $01
  ret

; strnul - Check if string is empty
; Arguments:
; A - string address
; Returns:
; A - status
strnul:
  lds %a
  lodsb
  cmp %s $00
  jme strnul-nul
  lda $01
  ret
strnul-nul:
  lda $00
  ret

; memcpy - Copy memory location into an other area
; Arguments:
; A - Target
; B - Destination
; C - Number of bytes to copy
memcpy:
  ldd $00
  lds %a
  lodsb
  ldg %b
  stgrb %s
  inx %a
  inx %b
  loop memcpy
  ret

; gfs-read-file - Read the file in the drive (GovnFS filesystem) and
; copy the file contents into an address
; D - directory
; G - filename
; S - address to store data from a file
gfs-read-file:
  lds kp-1-1msg
  jmp fail

; Boot GovnOS
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

; com-govnos - GovnOS Shell
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
  jme com-govnos-process
  jmp com-govnos-input
com-govnos-term:
  push $00
  int $00
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
com-govnos-process: ; Process the command
  ldd $00
  storb %d
  lds commi
  ldd $00
  lodgb
  storb %d ; Load $00 (NUL) instead of $0A (Enter)

  ; Empty command
  lda comm
  call strnul
  cmp %a $00
  jme com-govnos-aftexec

  ; dir
  lda comm
  ldb instFULL-dir
  call strcmp ; Assembly with your own library is easy right :D
  cmp %a $00  ; Check for 0 (equal status)
  jme com-govnosEXEC-dir

  ; cls
  lda comm
  ldb instFULL-cls
  call strcmp
  cmp %a $00
  jme com-govnosEXEC-cls

  ; help
  lda comm
  ldb instFULL-help
  call strcmp
  cmp %a $00
  jme com-govnosEXEC-help

  ; hlt
  lda comm
  ldb instFULL-hlt
  call strcmp
  cmp %a $00
  jme com-govnosEXEC-hlt

  ; exit
  lda comm
  ldb instFULL-exit
  call strcmp
  cmp %a $00
  jme com-govnosEXEC-exit

  ; exit
  lda comm
  ldb instFULL-echo
  ldc $20 ; compare until space
  call pstrcmp
  cmp %a $00
  jme com-govnosEXEC-echo

  ; retr
  lda comm
  ldb instFULL-retr
  call strcmp
  cmp %a $00
  jme com-govnos

  ; drive
  lda comm
  ldb instFULL-drve
  call strcmp
  cmp %a $00
  jme com-govnosEXEC-drive

  ; gsfetch
  lda comm
  ldb instFULL-gsfc
  call strcmp
  cmp %a $00
  jme com-govnosEXEC-gsfetch

  ; Otherwise it's a bad instruction
  lds bad-inst-msg
  call puts
com-govnos-aftexec:
  jmp com-govnos-prompt

; Commands
com-govnosEXEC-dir:
  lds dir-00msg
  call puts
  lds kp-1-1msg
  call fail
  jmp com-govnos-aftexec ; Go to a new task after execution

com-govnosEXEC-cls:
  lds cls-seq
  call puts
  jmp com-govnos-aftexec

com-govnosEXEC-help:
  lds help-msg
  call puts
  jmp com-govnos-aftexec

com-govnosEXEC-hlt:
  lds kp-6-0msg
  call puts
  hlt

com-govnosEXEC-drive:
  ldd $01
  cpuid
  cmp %d $00
  jme com-govnosEXEC-driveDSC
com-govnosEXEC-driveCNN:
  lds drvCNN-msg
  call puts
  jmp com-govnos-aftexec
com-govnosEXEC-driveDSC:
  lds drvDSC-msg
  call puts
  jmp com-govnos-aftexec

com-govnosEXEC-gsfetch:
  lds gsfc-stM
  call puts

  lds gsfc-hostM ; Host
  call puts
  lds env-PCNAME
  call puts

  lds gsfc-osM ; OS
  call puts
  lds envc-OSNAME
  call puts

  lds gsfc-cpuM ; CPU
  call puts
  ldd $00
  cpuid
  cmp %d $00
  jme com-govnosEXEC-gsfetch-gc16x

  lds proc-unkM
  call puts
  jmp com-govnosEXEC-gsfetch-end
com-govnosEXEC-gsfetch-gc16x:
  lds proc-00M
  call puts
com-govnosEXEC-gsfetch-end:
  push $0A
  int $02
  jmp com-govnos-aftexec

com-govnosEXEC-echo:
  ; Progress to space
  lds comm
  ldb $20
  call strtok
  inx %s
  call puts
  push $0A
  int $02

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
welcome-msg:   bytes "Welcome to GovnOS!$To get help, type `help`$$^@"
bschk:         bytes "Backspace$^@"
dir-00msg:     bytes "dir is not implemented$^@"
help-msg:      bytes "GovnOS Help manual page 1/1$"
               bytes "  cls       Clear the screen$"
               bytes "  dir       List directories$"
               bytes "  drive     Show if any drive is connected$"
               bytes "  echo      Write text to the screen$"
               bytes "  exit      Exit$"
               bytes "  gsfetch   Show system info$"
               bytes "  help      Show help$"
               bytes "  hlt       Halt the system (Kernel panic 6,0)$"
               bytes "  retr      Restart the shell$^@"
exec-statusM:  bytes "Executing command ...$^@"
; GSFETCH
gsfc-stM:      bytes     $1B "[97mgsfetch$" $1B "[0m---------$^@"
gsfc-hostM:    bytes     $1B "[97mHost: " $1B "[0m^@"
gsfc-osM:      bytes $0A $1B "[97mOS: " $1B "[0m^@"
gsfc-cpuM:     bytes $0A $1B "[97mCPU: " $1B "[0m^@"

; Kernel panic
; 0 - Processor error
; 1 - Disk/Filesystem error
; 41 - Unknown error
kp-0-0msg:     bytes "Kernel panic: Unable to find processor type(0,0)$^@"
kp-1-0msg:     bytes "Kernel panic: Unknown filesystem(1,0)$^@"
kp-1-1msg:     bytes "Kernel panic: Could not read disk(1,1)$^@"
kp-6-0msg:     bytes "Kernel panic: Triggered halt(6,0)$^@"
kp-41-0msg:    bytes "Kernel panic: Kernel error(41,0)$^@"

; CPU types
procchk-msg:   bytes "[0000] Checking CPU$^@"
proc-00-msg:   bytes "[0001] CPU: Govno Core 16X$$^@"
proc-unk-msg:  bytes "[0001] CPU: Unknown$$^@"
proc-00M:      bytes "Govno Core 16X$^@"
proc-unkM:     bytes "Unknown :($^@"

drvCNN-msg:    bytes "Disk connected as A/$^@"
drvDSC-msg:    bytes "Disk disconnected, A/ is an empty byte stream.$"
               bytes "Loading without a disk can have serious issues for commands that use GovnFS filesystem$^@"

; Environment variables
; 11 bytes
env-PS:        bytes "A/^$ ^@^@^@^@^@^@^@"
env-PCNAME:    bytes "GovnPC Ultra^@^@^@^@"
; Constant environment variables
envc-OSNAME:   bytes "GovnOS 0.0.1^@^@^@^@"

; Data
dynptr:        bytes $00

; Control sequences
bs-seq:        bytes $08 $20 $08 "^@"
cls-seq:       bytes $1B $5B $48 $1B $5B $32 $4A "^@"

; Commands
instFULL-dir:  bytes "dir^@"
instFULL-cls:  bytes "cls^@"
instFULL-help: bytes "help^@"
instFULL-hlt:  bytes "hlt^@"
instFULL-exit: bytes "exit^@"
instFULL-echo: bytes "echo "
instFULL-retr: bytes "retr^@"
instFULL-drve: bytes "drive^@"
instFULL-gsfc: bytes "gsfetch^@"
bad-inst-msg:  bytes "Bad command.$^@"

; Buffers
comm:          bytes "^@^@^@^@^@^@^@^@^@^@^@^@^@^@^@^@"
               bytes "^@^@^@^@^@^@^@^@^@^@^@^@^@^@^@^@"
               bytes "^@^@^@^@^@^@^@^@^@^@^@^@^@^@^@^@"
               bytes "^@^@^@^@^@^@^@^@^@^@^@^@^@^@^@^@"
commi:         bytes "^@"
bootsecend:    bytes $AA $55 "^@" ; End the boot sector


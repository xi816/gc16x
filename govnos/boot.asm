reboot: jmp boot

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
  sub %c $01
write_lp:
  ldg %s
  lodgb
  push %g
  int $02
  inx %s
  loop write_lp
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
  ldd $00
  loop strtok
  ret

; inttostr - Convert a 16_bit integer into a string
; Arguments:
; A - Number
inttostr:
  ldg inttostr_buf
  add %g $04
inttostr_lp:
  div %a #10 ; Divide and get the remainder into D
  add %d #48 ; Convert to ASCII
  lds %g
  storb %d
  dex %g
  cmp %a $00
  jmne inttostr_lp
  ret
inttostr_buf: reserve #5 bytes

inttostr_clr:
  ldc $4
  ldd $00
  lds inttostr_buf
inttostr_clr_lp:
  storb %d
  inx %s
  loop inttostr_clr_lp
  ret

; inttostr - Convert a 16-bit integer into a string with delimiters
; Arguments:
; A - Number
; B - Locale delimitor symbol
inttostrl:
  ldg inttostrl_buf
  ldc $00
  add %g $05
inttostrl_lp:
  inx %c
  div %a #10 ; Divide and get the remainder into D
  add %d #48 ; Convert to ASCII
  lds %g
  storb %d
  dex %g
  push %c
  div %c $03
  pop %c
  cmp %d $00
  jme inttostrl_pd
  cmp %a $00
  jmne inttostrl_lp
  ret
inttostrl_pd: ; Put the delimiter
  lds %g
  storb %b
  dex %g
  cmp %a $00
  jmne inttostrl_lp
  ret
inttostrl_buf: reserve #6 bytes ; the maximum value is 65,536 (6 bytes)

inttostrl_clr:
  ldc $5
  ldd $00
  lds inttostrl_buf
inttostrl_clr_lp:
  storb %d
  inx %s
  loop inttostrl_clr_lp
  ret

; puti - Output a 16-bit integer number
; Arguments:
; A - Number
puti:
  call inttostr
  lds inttostr_buf
  ldc $05
  call write
  call inttostr_clr
  ret

; putid - Output a 16-bit integer number with delimiters
; Arguments:
; A - Number
; B - Locale delimiter
putid:
  call inttostrl
  lds inttostrl_buf
  ldc $06
  call write
  call inttostrl_clr
  ret

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
  jmne strcmp_fail
  cmp %s $00
  jme strcmp_eq
  inx %a
  inx %b
  jmp strcmp
strcmp_eq:
  lda $00
  ret
strcmp_fail:
  lda $01
  ret

; pstrcmp - Check if two strings are equal ending in a predicate
; Arguments:
; A - first string address
; B - second string address
; C - predicate character
; Returns:
; A - status
pstrcmp:
  lds %a
  ldg %b
  lodsb
  lodgb
  cmp %s %g
  jmne pstrcmp_fail
  cmp %s %c
  jme pstrcmp_eq
  inx %a
  inx %b
  jmp pstrcmp
pstrcmp_eq:
  lda $00
  ret
pstrcmp_fail:
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
  jme strnul_nul
  lda $01
  ret
strnul_nul:
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

; strcpy - Copy one string into another location
; Arguments:
; A - Target
; B - Destination
strcpy:
  ldd $00
  lds %a
  lodsb
  ldg %b
  stgrb %s
  inx %a
  inx %b
  cmp %a $00 ; Target has no more bytes to copy
  jmne strcpy
  ret

; scani - Scan an integer from standard input
; Returns:
; A - number
scani:
  lda $00
scani_lp:
  int $01
  pop %b
  cmp %b $0A ; Check for Enter
  re
  cmp %b $20 ; Check for space
  re
  cmp %b $00 ; Check for NUL ($00)
  re
  mul %a #10
  push %b
  int $02
  sub %b #48
  add %a %b
  jmp scani_lp

; scans - Scan a string from standard input
; S - buffer address 0040
scans:
  int $01    ; Get character from input
  pop %d

  cmp %d $7F ; Check for Backspace
  jme scans_bs
  cmp %d $08
  jme scans_bs

  push %d    ; Output the character
  int $02

  ldg *qptr  ; Save into memory
  add %s %g
  storb %d
  sub %s %g
  inx qptr

  cmp %d $0A ; Return if pressed Enter
  re
  jmp scans
scans_bs: ; Handle backspace ($7F or $08)
  ldg *qptr
  cmp %g $00
  jme scans
scans_bs_strict:
  dex qptr
  push %s
  lds bs_seq
  call puts
  pop %s
  jmp scans

; gfs_read_signature _ Read the signature of the
; drive (GovnFS filesystem)
; Returns:
; magic_byte: magic byte
; drive_letter: drive letter
; disk_size: disk size
; gfs_sign_sernum: serial number
gfs_read_signature:
  lds $0000 ; magic byte address (disk)
  ldds
  lds magic_byte
  storb %a

  lds $0011 ; drive letter address (disk)
  ldds
  lds drive_letter
  storb %a

  lds $0010 ; disk size (in sectors) address (disk)
  ldds
  lds disk_size
  storb %a

  lds $000C
  ldds
  lds gfs_sign_sernum
  storb %a

  lds $000D
  ldds
  lds gfs_sign_sernum
  inx %s
  storb %a

  lds $000E
  ldds
  lds gfs_sign_sernum
  add %s $02
  storb %a

  lds $000F
  ldds
  lds gfs_sign_sernum
  add %s $03
  storb %a

  lda %c
  ret
gfs_sign_sernum: reserve #4 bytes
magic_byte:      reserve #1 bytes
disk_size:       reserve #1 bytes
drive_letter:    reserve #1 bytes

; gfs_read_file - Read the file in the drive (GovnFS filesystem) and
; copy the file contents into an address
; D - directory
; G - filename
; S - address to store data from a file
gfs_read_file:
  lds com_file_full
  str $F0
  lda %s
  ldb cline
  call strcpy
  ldb com_file_sign
  call strcpy
  lda $0020
  ldb com_file_full
  call dstrsubset
  lds com_file_full
  call strlen
  add %s %b              ; S = file content start
  ldg $3000              ; load the file into $3000

  ; call flcpy (s -> src{disk}, g -> dst{mem})
  ret

; flcpy - Copy the file contents into memory (assuming
;   %s is already loaded with the disk address to the file)
; Arguments:
; S - file contents disk address
; G
flcpy:
  lds kp_1_1msg
  jmp fail

; Boot GovnOS
boot:
  lds st_msg
  call puts
  ; hlt
boot_start:
  ; Find the current CPU
  lds procchk_msg
  call puts
  ldd $00
  cpuid
  cmp %d $00
  jme boot_gc16x_cpu
  jmp boot_unk_cpu
boot_gc16x_cpu:
  lds proc_00_msg
  call puts
  jmp boot_shell
boot_unk_cpu:
  lds proc_unk_msg
  call puts
  lds kp_0_0msg
  jmp fail
boot_shell:
  ; Show memory usage
  call fre
  ; Start the shell
  call com_govnos

fail:
  call puts
  hlt

; com_govnos _ GovnOS Shell
com_govnos:
  lds welcome_msg
  call puts

  ; Read the disk signature
  call gfs_read_signature

  ; Change the PS1 (shell prompt) to show
  ; the actual current drive
  lds env_PS
  ldb *drive_letter
  storb %b

  jmp com_govnos_prompt
com_govnos_prompt: ; Print the prompt
  lds env_PS
  call puts
com_govnos_input:
  lds cline
  call scans
  jmp com_govnos_process
com_govnos_term:
  push $00
  int $00
com_govnos_process: ; Process the command
  ; ldd $00
  ; storb %d
  lds cline
  ldg *qptr
  add %s %g
  dex %s
  ldd $00
  storb %d ; Load $00 (NUL) instead of $0A (Enter)

  lds qptr
  storb %d

  ; Empty command
  lda cline
  call strnul
  cmp %a $00
  jme com_govnos_aftexec

  ; dir
  lda cline
  ldb instFULL_dir
  call strcmp ; Assembly with your own library is easy right :D
  cmp %a $00  ; Check for 0 (equal status)
  jme com_govnosEXEC_dir

  ; cls
  lda cline
  ldb instFULL_cls
  call strcmp
  cmp %a $00
  jme com_govnosEXEC_cls

  ; color
  lda cline
  ldb instFULL_colr
  call strcmp
  cmp %a $00
  jme com_govnosEXEC_color

  ; help
  lda cline
  ldb instFULL_help
  call strcmp
  cmp %a $00
  jme com_govnosEXEC_help

  ; hlt
  lda cline
  ldb instFULL_hlt
  call strcmp
  cmp %a $00
  jme com_govnosEXEC_hlt

  ; exit
  lda cline
  ldb instFULL_exit
  call strcmp
  cmp %a $00
  jme com_govnosEXEC_exit

  ; echo
  lda cline
  ldb instFULL_echo
  ldc $20 ; compare until space
  call pstrcmp
  cmp %a $00
  jme com_govnosEXEC_echo

  ; retr
  lda cline
  ldb instFULL_retr
  call strcmp
  cmp %a $00
  jme com_govnos

  ; reboot
  lda cline
  ldb instFULL_rebt
  call strcmp
  cmp %a $00
  jme com_govnosEXEC_reboot

  ; info
  lda cline
  ldb instFULL_info
  call strcmp
  cmp %a $00
  jme com_govnosEXEC_info

  ; drive
  lda cline
  ldb instFULL_drve
  call strcmp
  cmp %a $00
  jme com_govnosEXEC_drive

  ; gsfetch
  lda cline
  ldb instFULL_gsfc
  call strcmp
  cmp %a $00
  jme com_govnosEXEC_gsfetch

  ; Otherwise it's a bad instruction
  lds bad_inst_msg
  call puts
com_govnos_aftexec:
  jmp com_govnos_prompt

; Commands
com_govnosEXEC_dir:
  call gfs_read_signature
  lds dir00_msg
  call puts
  ldb *drive_letter
  push %b
  int $02
  lds dir01_msg
  call puts

  lds dir_00msg
  call puts
  ; lds kp_1_1msg
  ; call fail
  jmp com_govnos_aftexec ; Go to a new task after execution

com_govnosEXEC_cls:
  lds cls_seq
  call puts
  jmp com_govnos_aftexec

com_govnosEXEC_color:
  lds color00_msg
  call puts
  call scani

  push '^[' int $02
  push '['  int $02
  push '3'  int $02
  call puti
  push 'm'  int $02
  push '$'  int $02

  jmp com_govnos_aftexec

com_govnosEXEC_help:
  lds help_msg
  call puts
  jmp com_govnos_aftexec

com_govnosEXEC_hlt:
  lds kp_6_0msg
  call puts
  hlt

com_govnosEXEC_drive:
  ldd $01
  cpuid
  cmp %d $00
  jme com_govnosEXEC_driveDSC
com_govnosEXEC_driveCNN:
  lds drvCNN_msg00
  call puts
  lds *drive_letter
  push %s
  int $02
  lds drvCNN_msg01
  call puts

  jmp com_govnos_aftexec
com_govnosEXEC_driveDSC:
  lds drvDSC_msg
  call puts
  jmp com_govnos_aftexec

com_govnosEXEC_gsfetch:
  lds gsfc_stM
  call puts

  lds gsfc_hostM ; Host
  call puts
  lds env_PCNAME
  call puts

  lds gsfc_osM ; OS
  call puts
  lds envc_OSNAME
  call puts

  lds gsfc_cpuM ; CPU
  call puts
  ldd $00
  cpuid
  cmp %d $00
  jme com_govnosEXEC_gsfetch_gc16x
  lds proc_unkM
  call puts
  jmp com_govnosEXEC_gsfetch_end

com_govnosEXEC_gsfetch_gc16x:
  lds proc_00M
  call puts
com_govnosEXEC_gsfetch_end:
  lds gsfc_memM
  call puts

  ldd $0000
  ldb bootsecend
  sub %d %b
  add %d $02
  lda %d
  div %a #1024
  inx %a ; maybe
  ldb #64
  sub %b %a
  lda %b
  call puti

  lds gsfc_memN
  call puts

  ldd $02
  cpuid ; Get memory size
  lda %d
  dex %a ; in case of 65,536 being 0
  div %a #1024
  inx %a ; maybe
  call puti

  lds gsfc_memO
  call puts

  lds gsfc_backM ; Logo
  call puts

  push $0A
  int $02
  jmp com_govnos_aftexec

com_govnosEXEC_echo:
  ; Progress to space
  lds cline
  ldb $20
  call strtok
  inx %s
  call puts
  push $0A
  int $02
  jmp com_govnos_aftexec

com_govnosEXEC_info:
  lds OS_RELEASE
  call puts
  jmp com_govnos_aftexec

com_govnosEXEC_exit:
  lds exit_term_msg
  call puts
  jmp com_govnos_term

com_govnosEXEC_reboot:
  lds cls_seq
  call puts
  jmp reboot

; Shutdown and termination
fre:
  ldd $0000
  ldb bootsecend
  sub %d %b
  add %d $02
  lda %d
  ldb *locale_delim
  call putid ; Output the number

  lds fre00_msg
  call puts
  ret

com_govnos_shutdown:
  lds exit_msg
  call puts
com_govnos_term:
  push $00
  int $00

; Text
st_msg:        bytes "Loading GovnOS ...$^@"
fail_msg:      bytes "Halting execution ...$^@"
exit_msg:      bytes "$Shutting down ...$^@"
exit_term_msg: bytes "exit$^@"
welcome_msg:   bytes "Welcome to GovnOS!$To get help, type `help`$$^@"
bschk:         bytes "Backspace$^@"
dir_00msg:     bytes "^[[91mdir is not fully implemented^[[0m$^@"
help_msg:      bytes "GovnOS Help manual page 1/1$"
               bytes "  cls       Clear the screen$"
               bytes "  color     Change the text color$"
               bytes "  dir       List directories$"
               bytes "  drive     Show if any drive is connected$"
               bytes "  echo      Write text to the screen$"
               bytes "  exit      Exit$"
               bytes "  gsfetch   Show system info$"
               bytes "  help      Show help$"
               bytes "  hlt       Halt the system (Kernel panic 6,0)$"
               bytes "  info      Show OS release info$"
               bytes "  reboot    Reboot GovnOS$"
               bytes "  retr      Restart the shell$^@"
exec_statusM:  bytes "Executing command ...$^@"
fre00_msg:     bytes " bytes free$^@"
dir00_msg:     bytes "Drive ^@"
dir01_msg:     bytes "$Contents of the drive:$  no shit make the driver first$^@"
color00_msg:   bytes "Enter the color number (0-7): ^@"
; GSFETCH
gsfc_stM:      bytes "             ^[[97mgsfetch$^[[0m             ---------$^@"
gsfc_hostM:    bytes "             ^[[97mHost: ^[[0m^@"
gsfc_osM:      bytes "$             ^[[97mOS: ^[[0m^@"
gsfc_cpuM:     bytes "$             ^[[97mCPU: ^[[0m^@"
gsfc_memM:     bytes "             ^[[97mMemory: ^[[0m^@"
gsfc_memN:     bytes "KB/^@"
gsfc_memO:     bytes "KB$^@"
gsfc_backM:    bytes "^[[6A^[[33m  .     . .$"
               bytes            "     A     .$"
               bytes            "    (=) .$"
               bytes            "  (=====)$"
               bytes            " (========)^[[0m$$^@"

; Kernel panic
; 0 _ Processor error
; 1 _ Disk/Filesystem error
; 41 _ Unknown error
kp_0_0msg:     bytes "Kernel panic: Unable to find processor type(0,0)$^@"
kp_1_0msg:     bytes "Kernel panic: Unknown filesystem(1,0)$^@"
kp_1_1msg:     bytes "Kernel panic: Could not read disk(1,1)$^@"
kp_6_0msg:     bytes "Kernel panic: Triggered halt(6,0)$^@"
kp_41_0msg:    bytes "Kernel panic: Kernel error(41,0)$^@"

; CPU types
procchk_msg:   bytes "Checking CPU ...$^@"
proc_00_msg:   bytes "CPU: Govno Core 16X$$^@"
proc_unk_msg:  bytes "CPU: Unknown$$^@"
proc_00M:      bytes "Govno Core 16X$^@"
proc_unkM:     bytes "Unknown :($^@"

drvCNN_msg00:  bytes "Disk connected as ^@"
drvCNN_msg01:  bytes "/$^@"
drvDSC_msg:    bytes "Disk disconnected, A/ is an empty byte stream.$"
               bytes "Loading without a disk can have serious issues for commands that use GovnFS filesystem$^@"

; Environment variables
; 11 bytes
env_PS:        bytes " /^$ ^@^@^@^@^@^@^@"
env_PCNAME:    bytes "GovnPC Ultra^@^@^@^@"
; Constant environment variables
envc_OSNAME:   bytes "GovnOS 0.0.2^@^@^@^@"

; Info
OS_RELEASE:    bytes "^[[96mGovnOS version 0.0.2 (alpha)$"
               bytes "Release date: 01/12/2025$"
               bytes "$(c) Xi816, 2025"
               bytes "^[[0m$^@"

; Data
errno:         reserve #1 bytes
dynptr:        reserve #1 bytes

locale_delim:  bytes ","

; Control sequences
bs_seq:        bytes "^H ^H^@"
cls_seq:       bytes "^[[H^[[2J^@"

; Commands
instFULL_dir:  bytes "dir^@"
instFULL_cls:  bytes "cls^@"
instFULL_colr: bytes "color^@"
instFULL_help: bytes "help^@"
instFULL_hlt:  bytes "hlt^@"
instFULL_exit: bytes "exit^@"
instFULL_rebt: bytes "reboot^@"
instFULL_retr: bytes "retr^@"
instFULL_info: bytes "info^@"
instFULL_drve: bytes "drive^@"
instFULL_gsfc: bytes "gsfetch^@"

instFULL_echo: bytes "echo "
bad_inst_msg:  bytes "Bad command.$^@"

; Buffers
cline:          reserve #64 bytes
qptr:           reserve #1 bytes

bootsecend:    bytes $AA $55


reboot: jmp boot

; GovnFS signature bytes that are not allowed in code:
; F0, F1, F2, F7
; F1 -- File block
; F2 -- Group block
; F7 -- Disk end
;
; File: F1 [filename] F2 [tag] F2 [contents] F1

; write - Output the buffer
; Arguments:
; si - buffer address
; cx - number of characters to output
write:
  ldd $00
  sub %cx $01
.loop:
  push *%si
  int $02
  inx %si
  loop .loop
  ret

; puts - Output string until NUL ($00)
; Arguments:
; si - string address
puts:
  cmp *%si $00
  re
  push *%si
  int $02
  inx %si
  jmp puts

; dpputs - Output string from disk until NUL ($00) or predicate
; Arguments:
; bx - predicate character
; si - string address
dpputs:
  ldds
  cmp %ax $00
  re
  cmp %ax %bx
  re
  push %ax
  int $02
  inx %si
  jmp dpputs

; strtok - Progress the string pointer until character
; Arguments:
; si - string address
; bx - end character
; (also affects dynptr)
strtok:
  inx %si
  ldc *%si
  sub %cx %bx
  loop strtok
  ret

; dstrtok - Progress the string pointer{disk} until character
; Arguments:
; si - string address
; bx - end character
; (also affects dynptr)
dstrtok:
  ldds
  cmp %ax $F7 ; End of the disk
  jme .error
  cmp %ax %bx
  jme .done
  inx %si
  jmp dstrtok
.error:
  ldb $01
  ret
.done:
  ldb $00
  ret

; dbstrtok - Progress back the string pointer{disk} until character
; Arguments:
; si - string address
; bx - end character
; (also affects dynptr)
dbstrtok:
  ldds
  cmp %ax $F7 ; End of the disk
  jme .error
  cmp %ax %bx
  jme .done
  dex %si
  jmp dstrtok
.error:
  ldb $01
  ret
.done:
  ldb $00
  ret

; inttostr - Convert a 16-bit integer into a string
; Arguments:
; ax - Number
inttostr:
  ldg inttostr_buf
  add %gi $04
.loop:
  div %ax 10 ; Divide and get the remainder into D
  add %dx 48 ; Convert to ASCII
  lds %gi
  storb %dx
  dex %gi
  cmp %ax $00
  jmne .loop
  ret
inttostr_buf: reserve 5 bytes

inttostr_clr:
  ldc $4
  ldd $00
  lds inttostr_buf
.loop:
  storb %dx
  inx %si
  loop .loop
  ret

; inttostr - Convert a 16-bit integer into a string with delimiters
; Arguments:
; ax - Number
; bx - Locale delimitor symbol
inttostrl:
  ldg inttostrl_buf
  ldc $00
  add %gi $05
.loop:
  inx %cx
  div %ax 10 ; Divide and get the remainder into D
  add %dx 48 ; Convert to ASCII
  lds %gi
  storb %dx
  dex %gi
  push %cx
  div %cx $03
  pop %cx
  cmp %dx $00
  jme .putdelim
  cmp %ax $00
  jmne .loop
  ret
.putdelim:    ; Put the delimiter
  cmp %ax $00
  re
  lds %gi
  storb %bx
  dex %gi
  cmp %ax $00
  jmne .loop
  ret
inttostrl_buf: reserve 6 bytes ; the maximum value is 65,536 (6 bytes)

inttostrl_clr:
  ldc $5
  ldd $00
  lds inttostrl_buf
.loop:
  storb %dx
  inx %si
  loop .loop
  ret

; puti - Output a 16-bit integer number
; Arguments:
; ax - Number
puti:
  call inttostr
  lds inttostr_buf
  ldc $05
  call write
  call inttostr_clr
  ret

; putid - Output a 16-bit integer number with delimiters
; Arguments:
; ax - Number
; bx - Locale delimiter
putid:
  call inttostrl
  lds inttostrl_buf
  ldc $06
  call write
  call inttostrl_clr
  ret

zputi: ; Using bx,cx,dx,gi
  ldc %ax
  div %ax %bx
  div %ax 10
  add %dx $30
  push %dx
  int 2
  lda %cx
  cmp %bx 1
  re
  div %bx 10
  jmp zputi

; strcmp - Check if two strings are equal
; Arguments:
; ax - first string address
; bx - second string address
; Returns:
; ax - status
strcmp:
  lds *%ax
  ldg *%bx
  cmp %si %gi
  jmne .fail
  cmp %si $00
  jme .eq
  inx %ax
  inx %bx
  jmp strcmp
.eq:
  lda $00
  ret
.fail:
  lda $01
  ret

; pstrcmp - Check if two strings are equal ending in a predicate
; Arguments:
; ax - first string address
; bx - second string address
; cx - predicate character
; Returns:
; ax - status
pstrcmp:
  lds *%ax
  ldg *%bx
  cmp %si %gi
  jmne .fail
  cmp %si %cx
  jme .eq
  inx %ax
  inx %bx
  jmp pstrcmp
.eq:
  lda $00
  ret
.fail:
  lda $01
  ret

; dstrcmp - Check if two strings are equal (first pointer being on the disk)
; Arguments:
; si - first string address{disk}
; gi - second string address
; Returns:
; ax - status
dstrcmp:
  ldds
  ldb *%gi

  inx %si
  inx %gi

  cmp %ax %bx
  jmne .fail
  cmp %bx $00
  jme .eq
  jmp dstrcmp
.eq:
  lda $00
  ret
.fail:
  lda $01
  ret

; strnul - Check if string is empty
; Arguments:
; ax - string address
; Returns:
; ax - status
strnul:
  cmp *%ax $00
  jme .nul
  lda $01
  ret
.nul:
  lda $00
  ret

; memcpy - Copy memory location into an other area
; Arguments:
; ax - Target
; bx - Destination
; cx - Number of bytes to copy
memcpy:
  lds *%ax
  ldg %bx
  stgrb %si
  inx %ax
  inx %bx
  loop memcpy
  ret

; strcpy - Copy one string into another location
; Arguments:
; ax - Target
; bx - Destination
strcpy:
  lds *%ax
  ldg %bx
  cmp %si $00 ; Target has no more bytes to copy
  re
  stgrb %si
  inx %ax
  inx %bx
  jmp strcpy

; dstrsubset - Find a specific string in the disk
; Arguments:
; si - address{disk}
; gi - address to the string{mem}
; At the end, si should point to the end of that string on the disk
dstrsubset:
  ldb *%gi

  call dstrtok ; Load si with the address to the first character (stored in ax)
    cmp %bx $01
    jme .fnf
  push %gi
  call dstrcmp ; Compare and store the status into ax
    cmp %ax $00 ; We found the substring (address in si)
  jme .end
  pop %gi
  dex %si

  jmp dstrsubset
.end: ; File found
  pop %x
  ldb $00 ; Success status for an outer function
  ret
.fnf: ; File not found
  lds fnf_msg
  call puts
  ldb $01 ; Error status for an outer function
  ret

; scani - Scan an integer from standard input
; Returns:
; ax - number
scani:
  lda $00
.loop:
  int $01
  pop %bx

  cmp %bx $0A ; Check for Enter
  re
  cmp %bx $20 ; Check for space
  re
  cmp %bx $00 ; Check for NUL ($00)
  re
  cmp %bx $30 ; Check if less than '0'
  jl .loop
  cmp %bx $3A ; Check if greater than '9'+1
  jg .loop
  cmp %bx $7F ; Check for Backspace
  jme .back

  mul %ax 10
  push %bx
  int $02
  sub %bx 48
  add %ax %bx
  jmp .loop
.back: ; Backspace handler
  div %ax 10
  jmp .loop

; scans - Scan a string from standard input
; si - buffer address
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

; gfs_read_signature - Read the signature of the
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
  storb %ax

  lds $0011 ; drive letter address (disk)
  ldds
  lds drive_letter
  storb %ax

  lds $0010 ; disk size (in sectors) address (disk)
  ldds
  lds disk_size
  storb %ax

  lds $000C
  ldds
  lds gfs_sign_sernum
  storb %ax

  lds $000D
  ldds
  lds gfs_sign_sernum
  inx %si
  storb %ax

  lds $000E
  ldds
  lds gfs_sign_sernum
  add %si $02
  storb %ax

  lds $000F
  ldds
  lds gfs_sign_sernum
  add %si $03
  storb %ax

  lda %cx
  ret
gfs_sign_sernum: reserve 4 bytes
magic_byte:      reserve 1 bytes
disk_size:       reserve 1 bytes
drive_letter:    reserve 1 bytes

; gfs_read_file - Read the file in the drive (GovnFS filesystem) and
; copy the file contents into an address
; dx - tag (UPDATE LATER)
; gi - filename
; si - address to store data from a file
gfs_read_file:
  lds com_file_full
  str $F1 ; Load $F1 into com_file_full[0]
  ldb %si
  call strcpy ; Load filename into com_file_full
  lda com_file_sign ; Load file signature into com_file_full
  call strcpy
  lds %bx
  str $00 ; Load $00 into com_file_full so it doesn't include the
          ; past query

  lds $001F
  ldg com_file_full
  call dstrsubset
    cmp %bx $01 ; Check for disk end
    re

  ldg %dx                ; load the file into *%dx
  call flcpy
  ldb $00
  ret
gfs_disk_space:
  lds $0020
  ldb $F7
  call dstrtok
  lda %si
  inx %ax
  ret

; flcpy - Copy the file contents into memory (assuming
; %si is already loaded with the disk address to the file)
; Arguments:
; si - file contents address{disk}
; gi - address where the file will be loaded{mem}

flcpy:
  ldds
  cmp %ax $F1
  re
  cmp %ax $E0
  jme .strange
.normie:
  stgrb %ax
  jmp .next
.strange:
  inx %si
  ldds
  add %ax $E0
  stgrb %ax
  jmp .next
.next:
  inx %si
  inx %gi
  jmp flcpy

lcinit:
  mb $F000 ','

; Boot GovnOS
boot:
  lds st_msg
  call puts
  ; hlt
boot_start:
  ; Find the current CPU
  ldd $00
  cpuid
  cmp %dx $00
  jme .gc16x_cpu
  jmp .unk_cpu
.gc16x_cpu:
  lds proc_00_msg
  call puts
  ldd $01
  cpuid
  cmp %dx $01
  jmne .livecd_warn
  jmp .shell
.unk_cpu:
  lds proc_unk_msg
  call puts
  lds kp_0_0msg
  jmp fail
.livecd_warn:
  lds livecd_msg
  call puts
.shell:
  ; Show memory usage
  call fre
  ; Start the shell
  call com_govnos

fail:
  call puts
  hlt

; com_govnos - GovnOS Shell
com_govnos:
  lds welcome_msg
  call puts

  ; Read the disk signature
  call gfs_read_signature

  ; Change the PS1 (shell prompt) to show
  ; the actual current drive
  lds env_PS
  add %si 5
  ldb *drive_letter
  storb %bx

  jmp .prompt
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
.process: ; Process the command
  ; ldd $00
  ; storb %dx
  lds cline
  ldg *qptr
  add %si %gi
  dex %si
  ldd $00
  storb %dx ; Load $00 (NUL) instead of $0A (Enter)
  lds qptr
  storb %dx

  ; Empty command
  lda cline
  call strnul
  cmp %ax $00
  jme com_govnos.aftexec

  ; dir
  lda cline
  ldb instFULL_dir
  call strcmp ; Assembly with your own library is easy right :D
  cmp %ax $00  ; Check for 0 (equal status)
  jme com_govnosEXEC_dir

  ; cls
  lda cline
  ldb instFULL_cls
  call strcmp
  cmp %ax $00
  jme com_govnosEXEC_cls

  ; help
  lda cline
  ldb instFULL_help
  call strcmp
  cmp %ax $00
  jme com_govnosEXEC_help

  ; hlt
  lda cline
  ldb instFULL_hlt
  call strcmp
  cmp %ax $00
  jme com_govnosEXEC_hlt

  ; exit
  lda cline
  ldb instFULL_exit
  call strcmp
  cmp %ax $00
  jme com_govnosEXEC_exit

  ; echo
  lda cline
  ldb instFULL_echo
  ldc $20 ; compare until space
  call pstrcmp
  cmp %ax $00
  jme com_govnosEXEC_echo

  ; retr
  lda cline
  ldb instFULL_retr
  call strcmp
  cmp %ax $00
  jme com_govnos

  ; drive
  lda cline
  ldb instFULL_drve
  call strcmp
  cmp %ax $00
  jme com_govnosEXEC_drive

  ; Load the file from the disk
  lda cline
  ldd $3000
  mov %fx com_file_sign
  call gfs_read_file
    cmp %bx $01
    jme .aftexec
  call $3000

  ; Otherwise it's a bad instruction
  ; lds bad_inst_msg
  ; call puts
.aftexec:
  jmp .prompt

; Commands
com_govnosEXEC_dir:
  lds dir00_msg
  call puts
  ldb *drive_letter
  push %bx
  int $02
  lds dir01_msg
  call puts
  lds $001F
.dirloop:
  ldb $F1
  call dstrtok
    cmp %bx $01
    jme com_govnos.aftexec
  ldb $F2
  call dstrtok

  ldg dir_tag
  call dstrcmp
    cmp %ax $00
    jme .file_process

  lds dir_00msg
  call puts
  jmp .dirloop
  jmp com_govnos.aftexec ; Go to a new task after execution
.file_process:
  ldb $F1
  call dbstrtok
  add %si 2
  ldb $F2
  call dpputs
  push $0A
  int 2
  jmp .dirloop

com_govnosEXEC_cls:
  lds cls_seq
  call puts
  jmp com_govnos.aftexec

com_govnosEXEC_help:
  lds help_msg
  call puts
  jmp com_govnos.aftexec

com_govnosEXEC_hlt:
  lds kp_6_0msg
  call puts
  hlt

com_govnosEXEC_drive:
  ldd $01
  cpuid
  cmp %dx $00
  jme com_govnosEXEC_driveDSC
com_govnosEXEC_driveCNN:
  lds drvCNN_msg00
  call puts
  lds *drive_letter
  push %si
  int $02
  lds drvCNN_msg01
  call puts

  jmp com_govnos.aftexec
com_govnosEXEC_driveDSC:
  lds drvDSC_msg
  call puts
  jmp com_govnos.aftexec

com_govnosEXEC_echo:
  ; Progress to space
  lds cline
  ldb $20
  call strtok
  inx %si
  call puts
  push $0A
  int $02
  jmp com_govnos.aftexec

com_govnosEXEC_exit:
  lds exit_term_msg
  call puts
  jmp com_govnos.term

com_govnosEXEC_reboot:
  lds cls_seq
  call puts
  int 4

fre:
  ldd $0000
  ldb bootsecend
  sub %dx %bx
  add %dx $02
  lda %dx

  ldb *locale_delim
  call putid ; Output the number

  lds fre00_msg
  call puts
  ret

; Shutdown and termination
com_govnos.shutdown:
  lds exit_msg
  call puts
com_govnos.term:
  push $00
  int $00

; Text
st_msg:        bytes "GovnBoot bootloader$^@"
exit_msg:      bytes "$Shutting down ...$^@"
exit_term_msg: bytes "exit$^@"
welcome_msg:   bytes "Load the kernel by typing `kernel.bin`$^@"
livecd_msg:    bytes "^[[91mLoaded from \"Live Floppy\" image$"
               bytes "Some commands using the GovnFS driver might not work^[[0m$^@"
bschk:         bytes "Backspace$^@"
dir_00msg:     bytes "^[[91mdir is not fully implemented^[[0m$^@"
help_msg:      bytes "GovnOS Help manual page 1/1$"
               bytes "  cls       Clear the screen$"
               bytes "  color     Change the text color$"
               bytes "  date      Show the current date$"
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
fre00_msg:     bytes " bytes free$^@"
dir00_msg:     bytes "Drive ^@"
dir01_msg:     bytes "$Contents of the drive:$  ^@"
color00_msg:   bytes "Enter the color number (0-7): ^@"
fnf_msg:       bytes "Bad command or file name.$^@"
; GSFETCH
gsfc_stM:      bytes "             ^[[97mgsfetch$^[[0m             ---------$^@"
gsfc_hostM:    bytes "             ^[[97mHost: ^[[0m^@"
gsfc_osM:      bytes "$             ^[[97mOS: ^[[0m^@"
gsfc_cpuM:     bytes "$             ^[[97mCPU: ^[[0m^@"
gsfc_memM:     bytes "             ^[[97mMemory: ^[[0m^@"
gsfc_diskM:    bytes "             ^[[97mDisk space: ^[[0m^@"
gsfc_memN:     bytes "KB/^@"
gsfc_memO:     bytes "KB$^@"
gsfc_diskN:    bytes "B/^@"
gsfc_backM:    bytes "^[[7A^[[33m  .     . .$"
               bytes            "     A     .$"
               bytes            "    (=) .$"
               bytes            "  (=====)$"
               bytes            " (========)^[[0m$$^@"

; Kernel panic
; 0        Processor error
; 1        Disk/Filesystem error
; 41       Unknown error
kp_0_0msg:     bytes "Kernel panic: Unable to find processor type(0,0)$^@"
kp_1_0msg:     bytes "Kernel panic: Unknown filesystem(1,0)$^@"
kp_1_1msg:     bytes "Kernel panic: Could not read disk(1,1)$^@"
kp_6_0msg:     bytes "Kernel panic: Triggered halt(6,0)$^@"
kp_41_0msg:    bytes "Kernel panic: Kernel error(41,0)$^@"

; CPU types
proc_00_msg:   bytes "CPU: ^[[32mGovno Core 16X^[[0m$$^@"
proc_unk_msg:  bytes "CPU: ^[[31mUnknown^[[0m$$^@"
proc_00M:      bytes "Govno Core 16X$^@"
proc_unkM:     bytes "Unknown :($^@"

drvCNN_msg00:  bytes "Disk connected as ^@"
drvCNN_msg01:  bytes "/$^@"
drvDSC_msg:    bytes "Disk disconnected, A/ is an empty byte stream.$"
               bytes "Loading without a disk can have serious issues for commands that use GovnFS filesystem$^@"

; Environment variables
; 11 bytes
env_PS:        bytes "^[[93m />^[[0m ^@"
env_PCNAME:    bytes "GovnPC Ultra^@^@^@^@"
; Constant environment variables
envc_OSNAME:   bytes "GovnOS 0.0.4^@^@^@^@"

; Data
errno:         reserve 1 bytes
dynptr:        reserve 1 bytes

date_locale_delim: bytes "-"
locale_delim:      bytes ","

; GovnFS signatures
com_file_sign: bytes $F2 "com/" $F2 $00
txt_file_sign: bytes $F2 "txt/" $F2 $00
com_file_full: reserve 96 bytes
dir_tag:       bytes $F2 "com/" $F2 $00

; Control sequences
bs_seq:        bytes "^H ^H^@"
cls_seq:       bytes "^[[H^[[2J^@"

; Commands
instFULL_dir:  bytes "dir^@"
instFULL_cls:  bytes "cls^@"
instFULL_help: bytes "help^@"
instFULL_hlt:  bytes "hlt^@"
instFULL_exit: bytes "exit^@"
instFULL_retr: bytes "retr^@"
instFULL_info: bytes "info^@"
instFULL_drve: bytes "drive^@"

instFULL_echo: bytes "echo "

; Buffers
cline:         reserve 64 bytes
qptr:          reserve 1 bytes

bootsecend:    bytes $AA $55

start:
  jmp	boot
puts:		; Address stays at S after end
  ldd	%s
  lodsb
  push	%s
  int	#02
  inx	%s
  cmp	%s #00
  jmne	puts
boot:		; Boot OS
  ; Show starting message
  lds	stm
  call	puts

  ; Find kernel in the disk
  lds	errm
  call	puts

stm:		bytes "Welcome to GovnOS!$^@"
logfirstm:	bytes "Loading kernel...$^@"
errm:		bytes "Error.$^@"

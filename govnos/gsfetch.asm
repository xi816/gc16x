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
  cmp %dx $00
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
  sub %dx %bx
  add %dx $02
  lda %dx
  div %ax 1024
  inx %ax ; maybe
  ldb 64
  sub %bx %ax
  lda %bx
  call puti

  lds gsfc_memN
  call puts

  ldd $02
  cpuid ; Get memory size
  lda %dx
  dex %ax ; in case of 65,536 being 0
  div %ax 1024
  inx %ax ; maybe
  call puti

  lds gsfc_memO
  call puts

  lds gsfc_diskM
  call puts

  call gfs_disk_space
  ldb *locale_delim
  call putid
  lds gsfc_diskN
  call puts

  ldd $03 ; Get disk size
  cpuid
  lda %dx
  dex %ax ; in case of disk size being 65,536 (0)
  div %ax 1024
  inx %ax ; maybe
  call puti
  lds gsfc_memO
  call puts

  lds gsfc_backM ; Logo
  call puts

  push $0A
  int $02
  ret

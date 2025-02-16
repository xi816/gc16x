date_main:
  int 3
  ; DX = date
  lds %dx
  ; Year
  lda %si
  div %ax 371
  add %ax 1970
  ldb 1000
  call zputi

  ldg *date_locale_delim
  push %gi
  int 2

  ; Month
  lda %si
  div %ax 31
  div %ax 12
  lda %dx
  inx %ax
  ldb 10
  call zputi

  ldb *date_locale_delim
  push %bx
  int 2

  ; Day
  lda %si
  div %ax 31
  lda %dx
  inx %ax
  ldb 10
  call zputi

  push $0A
  int 2
  ret

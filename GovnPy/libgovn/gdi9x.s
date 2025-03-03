border: ; (x, y, w, h, pat)
    pop %bp
    pop %gi
    pop %dx
    pop %bx
    pop %si
    pop %ax
    mul %si 340
    add %si %ax
    call border_asm
    push 0
    push %bp
ret
border_asm:
    push %si
.bloop:
    lda *%gi
    cmp %ax $10
    jme .fill
    cmp %ax $20
    jme .middle
    cmp %ax $30
    jme .eline
    cmp %ax $F0
    jme .end
    int $C
    inx %si
    inx %gi
    jmp .bloop

.fill:
    inx %gi
    lda *%gi
    ldc %bx
    dex %cx
.floop:
    int $C
    inx %si
    loop .floop
    inx %gi
    jmp .bloop

.eline:
    pop %si
    add %si 340
    push %si
    inx %gi
    jmp .bloop

.middle:
    inx %gi
    push %gi
    ldc %dx
    dex %cx
.mloop:
    lda *%gi
    cmp %ax $10
    jme .mfill
    cmp %ax $F0
    jme .mend
    int $C
    inx %si
    inx %gi
    jmp .mloop
.mend:
    lda %gi
    pop %gi
    pop %si
    add %si 340
    push %si
    push %gi
    loop .mloop
    pop %gi
    ldg %ax
    inx %gi
    jmp .bloop

.mfill:
    add %si %bx
    inx %gi
    jmp .mloop

.end:
    pop %ax
    ret

taskbar:
    pop %bp
    lds 58480
    lda 7
    ldc 340
.floop:
    int $C
    inx %si
    loop .floop
    lda 15
    ldc 340
.sloop:
    int $C
    inx %si
    loop .sloop
    lda 7
    ldc 5440
.tloop:
    int $C
    inx %si
    loop .tloop
    push %bp
ret

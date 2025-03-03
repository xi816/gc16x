vputs: ; (x,y,c,t)
    pop %bp
    pop %gi
    pop %ax
    pop %dx
    pop %si
    mul %dx 340
    add %si %dx
    call vputs-
    push %dx
    push %bp
ret

cls: ; (c)
    pop %bp
    pop %ax
    call cls-
    push 0
    push %bp
ret

box: ; (x,y,w,h,c)
    pop %bp
    pop %ax
    pop %dx
    pop %bx
    pop %cx
    pop %si
    mul %cx 340
    add %si %cx
    call box-
    push 0
    push %bp
ret
show:
    pop %bp
    int $11
    push 0
    push %bp
ret

spr: ; (x, y, c, data)
    pop %bp
    pop %gi
    pop %ax
    pop %si
    pop %dx
    mul %si 340
    add %si %dx
    call spr-
    push 0
    push %bp
ret

vputs-: ; G: char
    push %si
.line:
    push %gi
    lodgb
    cmp %gi 0
    jme .end
    cmp %gi 10
    jme .ln
    cmp %gi 27
    jme .undr
    cmp %gi $D0
    jme .gu8_d0
    cmp %gi $D1
    jme .gu8_d1
    sub %gi $20
.pchar:
    mul %gi 9
    ldd res.font
    add %gi %dx
    call spr-
    ldd *%gi
    sub %si 2720
    ;add %si %dx
    add %si 6
    pop %gi
    inx %gi
jmp .line
.gu8_d0:
    pop %gi
    inx %gi
    push %gi
    lodgb
    sub %gi $20
jmp .pchar
.gu8_d1:
    pop %gi
    inx %gi
    push %gi
    lodgb
    add %gi $20 ; $40 - $20
jmp .pchar
.ln:
    pop %gi
    pop %si
    add %si 2720
    inx %gi
jmp vputs-
.undr:
    ldg res.underline
    call spr-
    sub %si 2720
    pop %gi
    inx %gi
jmp .line
.end:
    pop %gi
    ldd %si
    pop %si
ret

; Gravno Display Interface 16
cls-: ; A: color
    lds 0
    ldc 64599
.loop:
    int $C
    inx %si
    loop .loop
ret
box-: ; A: color, B: width, D: height, S: start
    ldc %bx
    dex %cx
.pix:
    int $C
    inx %si
    loop .pix
    sub %si %bx
    add %si 340
    dex %dx
    cmp %dx 0
    jmne box-
ret

spr-: ; A: color, G: sprite data
    ldb 8
.line:
    push %gi
    lodgb
    ldc 7
.pix:
    div %gi 2
    cmp %dx 0
    jme .pix_next
    int $C
    ;inx %si
    ;int $C
    ;dex %si
.pix_next:
    inx %si
    loop .pix

    pop %gi
    inx %gi

    add %si 332

    dex %bx
    cmp %bx 0
    jmne .line
ret

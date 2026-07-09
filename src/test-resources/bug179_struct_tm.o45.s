    .o45
    .extern __sp_base
    .weak __static_chain
    .weak __zp_scratch
    .weak __zp_scratch2
    .weak __zp_scratch3
    .weak __zp_scratch4
    __static_chain = $06
    __zp_scratch = $08
    __zp_scratch2 = $0A
    __zp_scratch3 = $0C
    __zp_scratch4 = $0E

    .extern _mktime

    .global _result_addr
    .global _main

    .segment "data"
    .byte 0
_result_addr:
    .word 16384

    .segment "code"

; function _main
    proc _main
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler.dev_v1.0.5/bin/../lib/include/time.h", 6
; frame: 24 bytes (frame-allocated vRegs only)
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    tsx
    txa
    clc
    adc #1
    sta $FD
    lda #$01
    adc #0
    sta $FE
    .local __vr0 = 4
    .local __vr28 = 0
    .local __vr29 = 22

@entry:
    .loc "bug179_struct_tm.c", 13
    lda #126
    ldx #0
    sta $20
    stx $21
    leax.fp 4
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #10
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "bug179_struct_tm.c", 14
    lda #6
    ldx #0
    sta $20
    stx $21
    leax.fp 4
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #8
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "bug179_struct_tm.c", 15
    lda #3
    ldx #0
    sta $20
    stx $21
    leax.fp 4
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #6
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "bug179_struct_tm.c", 16
    lda #12
    ldx #0
    sta $20
    stx $21
    leax.fp 4
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #4
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "bug179_struct_tm.c", 17
    lda #0
    sta $20
    sta $21
    leax.fp 4
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #2
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "bug179_struct_tm.c", 18
    lda #0
    sta $20
    sta $21
    leax.fp 4
    sta $22
    stx $23
    lda $20
    ldx $21
    ldy #0
    sta ($22),y
    txa
    iny
    sta ($22),y
    .loc "bug179_struct_tm.c", 19
    lda #0
    sta $20
    sta $21
    leax.fp 4
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #12
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "bug179_struct_tm.c", 20
    lda #0
    sta $20
    sta $21
    leax.fp 4
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #14
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "bug179_struct_tm.c", 21
    lda #0
    sta $20
    sta $21
    leax.fp 4
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #16
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "bug179_struct_tm.c", 24
    leax.fp 4
    stax.fp __vr29
    ldax.fp __vr29
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _mktime
    phx
    pha
    tsx
    txa
    clc
    adc #1
    sta $FD
    lda #$01
    adc #0
    sta $FE
    pla
    plx
    stz @__restore_caller_z_0+1
    plz
    plz
@__restore_caller_z_0:
    ldz #0
    .var _fp = _fp - 2
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    staxyz.fp __vr28
    .loc "bug179_struct_tm.c", 27
    lda #255
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    ldy #0
    ldz #0
    sta $22
    stx $23
    sty $24
    stz $25
    ldaxyz.fp __vr28
    and $22
    sta $26
    stx $27
    sty $28
    stz $29
    lda _result_addr
    ldx _result_addr+1
    sta $20
    stx $21
    lda #0
    sta $22
    sta $23
    lda $26
    ldx $27
    ldy $28
    ldz $29
    pha
    .noopt_start
    addr_elem.16 __zp_scratch, $20, $22, #1
    ldy #0
    .noopt_end
    pla
    sta (__zp_scratch),y
    .loc "bug179_struct_tm.c", 28
    ldaxyz.fp __vr28
    txa
    pha
    tya
    tax
    tza
    tay
    ldz #0
    pla
    sta $20
    stx $21
    sty $22
    stz $23
    lda #255
    ldx #0
    sta $24
    stx $25
    lda $24
    ldx $25
    ldy #0
    ldz #0
    sta $26
    stx $27
    sty $28
    stz $29
    lda $20
    ldx $21
    ldy $22
    ldz $23
    and $26
    sta $2A
    stx $2B
    sty $2C
    stz $2D
    lda _result_addr
    ldx _result_addr+1
    sta $20
    stx $21
    lda #1
    ldx #0
    sta $22
    stx $23
    lda $2A
    ldx $2B
    ldy $2C
    ldz $2D
    pha
    .noopt_start
    addr_elem.16 __zp_scratch, $20, $22, #1
    ldy #0
    .noopt_end
    pla
    sta (__zp_scratch),y
    .loc "bug179_struct_tm.c", 29
    ldaxyz.fp __vr28
    tza
    tax
    tya
    ldy #0
    ldz #0
    sta $20
    stx $21
    sty $22
    stz $23
    lda #255
    ldx #0
    sta $24
    stx $25
    lda $24
    ldx $25
    ldy #0
    ldz #0
    sta $26
    stx $27
    sty $28
    stz $29
    lda $20
    ldx $21
    ldy $22
    ldz $23
    and $26
    sta $2A
    stx $2B
    sty $2C
    stz $2D
    lda _result_addr
    ldx _result_addr+1
    sta $20
    stx $21
    lda #2
    ldx #0
    sta $22
    stx $23
    lda $2A
    ldx $2B
    ldy $2C
    ldz $2D
    pha
    .noopt_start
    addr_elem.16 __zp_scratch, $20, $22, #1
    ldy #0
    .noopt_end
    pla
    sta (__zp_scratch),y
    .loc "bug179_struct_tm.c", 30
    ldaxyz.fp __vr28
    tza
    ldx #0
    ldy #0
    ldz #0
    sta $20
    stx $21
    sty $22
    stz $23
    lda #255
    ldx #0
    sta $24
    stx $25
    lda $24
    ldx $25
    ldy #0
    ldz #0
    sta $26
    stx $27
    sty $28
    stz $29
    lda $20
    ldx $21
    ldy $22
    ldz $23
    and $26
    sta $2A
    stx $2B
    sty $2C
    stz $2D
    lda _result_addr
    ldx _result_addr+1
    sta $20
    stx $21
    lda #3
    ldx #0
    sta $22
    stx $23
    lda $2A
    ldx $2B
    ldy $2C
    ldz $2D
    pha
    .noopt_start
    addr_elem.16 __zp_scratch, $20, $22, #1
    ldy #0
    .noopt_end
    pla
    sta (__zp_scratch),y
    .loc "bug179_struct_tm.c", 33
    lda #170
    sta $20
    lda _result_addr
    ldx _result_addr+1
    sta $22
    stx $23
    lda #4
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx #0
    pha
    .noopt_start
    addr_elem.16 __zp_scratch, $22, $24, #1
    ldy #0
    .noopt_end
    pla
    sta (__zp_scratch),y
@__return:
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    .func_flags stack_call
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc


__zp_save_buf:
    .res 248

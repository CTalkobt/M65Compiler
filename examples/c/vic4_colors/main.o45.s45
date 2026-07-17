    .o45
    .extern __sp_base
    __zp_scratch = $08
    __zp_scratch2 = $0A
    __zp_scratch3 = $0C
    __zp_scratch4 = $0E

    .extern _puts

    .global _VIC4
    .global _main

    .segment "data"
_VIC4:
    .word 53280

    .segment "code"

; function _unlock_mega65_io
    proc _unlock_mega65_io
    .var _fp = 0
    .loc "/home/duck/m65/in_devel/ccomp/bin/../lib/include/stdio.h", 11

@entry:
    .loc "main.c", 22
    lda #$47
    .loc "main.c", 23
    sta $d02f
    .loc "main.c", 24
    lda #$53
    .loc "main.c", 25
    sta $d02f
@__return:
    .func_flags stack_call, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc

; function _main
    proc _main
    .var _fp = 0
    .loc "main.c", 15
; frame: 2 bytes (frame-allocated vRegs only)
    phw #0
    .local __vr6 = 0

@entry:
    .loc "main.c", 29
    jsr _unlock_mega65_io
    .loc "main.c", 32
    lda #1
    sta $20
    lda _VIC4
    ldx _VIC4+1
    sta $22
    stx $23
    lda $22
    clc
    adc #0
    sta $24
    lda $23
    adc #0
    sta $25
    lda $20
    ldy #0
    sta ($24),y
    .loc "main.c", 33
    lda #1
    sta $20
    lda _VIC4
    ldx _VIC4+1
    sta $22
    stx $23
    lda $22
    clc
    adc #1
    sta $24
    lda $23
    adc #0
    sta $25
    lda $20
    ldy #0
    sta ($24),y
    .loc "main.c", 35
    ldax #__str_0
    stax.fp __vr6
    ldax.fp __vr6
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _puts
    plz
    plz
    .var _fp = _fp - 2
    .loc "main.c", 36
    lda #0
    ldx #0
@__return:
    plz
    plz
    .func_flags stack_call
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc


    .segment "data"
__str_0:
    .text "done."
    .byte 0

__zp_save_buf:

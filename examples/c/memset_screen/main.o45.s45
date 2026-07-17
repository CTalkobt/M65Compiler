    .o45
    .extern __sp_base
    __zp_scratch = $08
    __zp_scratch2 = $0A
    __zp_scratch3 = $0C
    __zp_scratch4 = $0E

    .extern _memset

    .global _main

    .segment "code"

; function _main
    proc _main
    .var _fp = 0
    .loc "/home/duck/m65/in_devel/ccomp/bin/../lib/include/string.h", 7
; frame: 8 bytes (frame-allocated vRegs only)
    phw #0
    phw #0
    phw #0
    phw #0
    .local __vr0 = 0
    .local __vr8 = 2
    .local __vr9 = 4
    .local __vr10 = 6
    .local @_l_ch = 0

@entry:
    .loc "main.c", 15
    lda #6
    sta $D020
    .loc "main.c", 16
    lda #0
    sta $D021
    .loc "main.c", 17
    sei
@while_body3:
    .loc "main.c", 21
    lda #0
    sta $20
    sta.fp __vr0
@for_cond5:
@for_body6:
    .loc "main.c", 23
    lda #0
    ldz #8
    staz.fp __vr8
    lda #232
    ldz #3
    staz.fp __vr9
    lda.fp __vr0
    ldx #0
    pha
    txa
    taz
    pla
    staz.fp __vr10
    ldax.fp __vr9
    sta $28
    stx $29
    ldax.fp __vr10
    sta $2C
    stx $2D
    ldax.fp __vr8
    sta $30
    stx $31
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    lda $2C
    ldx $2D
    push .ax
    .var _fp = _fp + 2
    lda $30
    ldx $31
    push .ax
    .var _fp = _fp + 2
    jsr _memset
    plz
    plz
    plz
    plz
    plz
    plz
    .var _fp = _fp - 6
    .loc "main.c", 24
    lda.fp __vr0
    cmp #255
    bcs @if_then9
    bra @if_end11
@if_then9:
    bra @for_end8
@if_end11:
@for_inc7:
    .loc "main.c", 21
    lda.fp __vr0
    inc a
    sta $20
    sta.fp __vr0
    bra @for_cond5
@for_end8:
    bra @while_body3
@__return:
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

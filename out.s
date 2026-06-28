* = $2000
    __sp_base = $0101
; Save ZP $08-$FF to BSS buffer
    ldx #0
@__zp_save_loop:
    lda $08,x
    sta __zp_save_buf,x
    inx
    cpx #248
    bne @__zp_save_loop
    jsr _main
    sta $02
    stx $03
; Restore ZP $08-$FF from BSS buffer
    ldx #0
@__zp_restore_loop:
    lda __zp_save_buf,x
    sta $08,x
    inx
    cpx #248
    bne @__zp_restore_loop
    lda $02
    ldx $03
__halt:
    jmp __halt
    .global __static_chain
    .global __zp_scratch
    .global __zp_scratch2
    .global __zp_scratch3
    .global __zp_scratch4
    __static_chain = $06
    __zp_scratch = $08
    __zp_scratch2 = $0A
    __zp_scratch3 = $0C
    __zp_scratch4 = $0E


; function _test
    proc _test, W#@_p_x, W#@_p_y
    .var _fp = 0
    .loc "/tmp/test_non_const.c", 2
    .var @_l_i = $26
    .var @_l_product = $28
    .var @_l_sum = $24
    .var @_p_x = 2
    .var @_p_y = 4

    ldax.fp @_p_x
    sta $20
    stx $21
    ldax.fp @_p_y
    sta $22
    stx $23
@entry:
    .loc "/tmp/test_non_const.c", 3
    lda #0
    ldx #0
    sta $24
    stx $25
    .loc "/tmp/test_non_const.c", 6
    lda #0
    ldx #0
    sta $26
    stx $27
    lda $26
    clc
    adc #1
    sta $2A
    lda $27
    adc #0
    sta $2B
@for_cond0:
    lda $26
    ldx $27
    cmp.16 .AX, #5
    bcc @for_body1
    bra @for_end3
@for_body1:
    .loc "/tmp/test_non_const.c", 7
    lda $20
    ldx $21
    mul.16 .AX, $22
    sta $2C
    stx $2D
    lda $2C
    ldx $2D
    sta $28
    stx $29
    .loc "/tmp/test_non_const.c", 8
    lda $24
    clc
    adc $28
    sta $24
    lda $25
    adc $28+1
    sta $25
@for_inc2:
    lda $2A
    ldx $2B
    sta $26
    stx $27
    bra @for_cond0
@for_end3:
    .loc "/tmp/test_non_const.c", 11
    lda $24
    ldx $25
@__return:
    .func_flags stack_call, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    endproc

; function _main
    proc _main
    .var _fp = 0
    .loc "/tmp/test_non_const.c", 14

@entry:
    .loc "/tmp/test_non_const.c", 15
    lda #5
    ldx #0
    push .ax
    .var _fp = _fp + 2
    lda #10
    ldx #0
    push .ax
    .var _fp = _fp + 2
    jsr _test
    plz
    plz
    plz
    plz
    .var _fp = _fp - 4
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    .func_flags stack_call
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc


__zp_save_buf:

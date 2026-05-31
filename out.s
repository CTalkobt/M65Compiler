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
    __zp_scratch = $08
    __zp_scratch2 = $0A
    __zp_scratch3 = $0C
    __zp_scratch4 = $0E


; function _add
    proc _add, W#@_p_a, W#@_p_b
    .var _fp = 0
    .loc "/tmp/test_header.c", 1
    .var @_p_a = 2
    .var @_p_b = 4

    ldax.fp @_p_a
    sta $20
    stx $21
    ldax.fp @_p_b
    sta $22
    stx $23
@entry:
    .loc "/tmp/test_header.c", 2
    lda $20
    clc
    adc $22
    sta $24
    lda $21
    adc $22+1
    sta $25
    lda $24
    ldx $25
@__return:
    .func_flags stack_call, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    endproc

; function _main
    proc _main
    .var _fp = 0
    .loc "/tmp/test_header.c", 3
; frame: 2 bytes (frame-allocated vRegs only)
    phw #0
    .local __vr0 = 0
    .local @_l_result = 0

@entry:
    .loc "/tmp/test_header.c", 4
    phw #4
    .var _fp = _fp + 2
    phw #3
    .var _fp = _fp + 2
    jsr _add
    .var _fp = _fp - 4
    sta $20
    stx $21
    lda $20
    ldx $21
    stax.fp __vr0
    .loc "/tmp/test_header.c", 5
    ldax.fp __vr0
@__return:
    plz
    plz
    .func_flags stack_call
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc


__zp_save_buf:

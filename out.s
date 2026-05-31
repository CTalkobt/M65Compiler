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


; function _signed_div
    proc _signed_div, W#@_p_a, W#@_p_b
    .var _fp = 0
    .loc "/tmp/test_div_signedness.c", 2
    .var @_p_a = 2
    .var @_p_b = 4

    ldax.fp @_p_a
    sta $20
    stx $21
    ldax.fp @_p_b
    sta $22
    stx $23
@entry:
    .loc "/tmp/test_div_signedness.c", 3
    lda $20
    ldx $21
    div.16 .AX, $22
    sta $24
    stx $25
    lda $24
    ldx $25
@__return:
    .func_flags stack_call, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    endproc

; function _unsigned_div
    proc _unsigned_div, W#@_p_a, W#@_p_b
    .var _fp = 0
    .loc "/tmp/test_div_signedness.c", 6
    .var @_p_a = 2
    .var @_p_b = 4

    ldax.fp @_p_a
    sta $20
    stx $21
    ldax.fp @_p_b
    sta $22
    stx $23
@entry:
    .loc "/tmp/test_div_signedness.c", 7
    lda $20
    ldx $21
    div.16 .AX, $22
    sta $24
    stx $25
    lda $24
    ldx $25
@__return:
    .func_flags stack_call, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    endproc

; function _signed_mod
    proc _signed_mod, W#@_p_a, W#@_p_b
    .var _fp = 0
    .loc "/tmp/test_div_signedness.c", 10
    .var @_p_a = 2
    .var @_p_b = 4

    ldax.fp @_p_a
    sta $20
    stx $21
    ldax.fp @_p_b
    sta $22
    stx $23
@entry:
    .loc "/tmp/test_div_signedness.c", 11
    lda $20
    ldx $21
    mod.16 .AX, $22
    sta $24
    stx $25
    lda $24
    ldx $25
@__return:
    .func_flags stack_call, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    endproc

; function _unsigned_mod
    proc _unsigned_mod, W#@_p_a, W#@_p_b
    .var _fp = 0
    .loc "/tmp/test_div_signedness.c", 14
    .var @_p_a = 2
    .var @_p_b = 4

    ldax.fp @_p_a
    sta $20
    stx $21
    ldax.fp @_p_b
    sta $22
    stx $23
@entry:
    .loc "/tmp/test_div_signedness.c", 15
    lda $20
    ldx $21
    mod.16 .AX, $22
    sta $24
    stx $25
    lda $24
    ldx $25
@__return:
    .func_flags stack_call, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    endproc


__zp_save_buf:

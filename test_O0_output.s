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

; function _get_long42
    proc _get_long42
    .var _fp = 0
    .loc "test_cast_fold_tmp.c", 1

@entry:
    lda #42
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
    lda $22
    ldx $23
    ldy $24
    ldz $25
@__return:
    .func_flags zp_call, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers N, Z
    endproc

    .global _get_long42

    .segment "code"


__zp_save_buf:
    .res 248

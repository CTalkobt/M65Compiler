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
    ldy #0
    ldz #0
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
@__return:
    .func_flags zp_call, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers N, Z
    endproc

    .global _get_long42

    .segment "code"


__zp_save_buf:
    .res 248

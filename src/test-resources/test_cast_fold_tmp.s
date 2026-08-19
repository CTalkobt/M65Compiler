    .o45
    .org $2000
    .weak __sp_base
    __sp_base = $0101
    .weak __static_chain
    .weak __zp_scratch
    .weak __zp_scratch2
    .weak __zp_scratch3
    .weak __zp_scratch4
    .weak cc45.zeroPageStart
    __static_chain = $06
    __zp_scratch = $08
    __zp_scratch2 = $0A
    __zp_scratch3 = $0C
    __zp_scratch4 = $0E
    cc45.zeroPageStart = $08

    .global _get_long42

    .segment "code"

; function _get_long42

    ; Static buffer for struct return from _get_long42
    _get_long42__struct_buf:
    .byte 0, 0, 0, 0

; SAC zero-alloc leaf: no storage overhead
    proc _get_long42
; Phase 51: zero-alloc leaf (all parameters constant)
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
    rts
    .func_flags stack_call, static_alloc, zeroalloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers N, Z
    .frame_size 0
    endproc


__zp_save_buf:

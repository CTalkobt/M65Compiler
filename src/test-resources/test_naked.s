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

    .global _fast_nop
    .global _main

    .segment "code"

; function _fast_nop
    proc _fast_nop
@entry:
    .loc "test_naked.c", 4
    clv
    .loc "test_naked.c", 5
    rts
    bra @__return
    endproc

; function _main
; SAC inline storage: 0 bytes
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_naked.c", 8

@entry:
    .loc "test_naked.c", 9
    jsr _fast_nop
    .loc "test_naked.c", 10
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 0
    endproc


__zp_save_buf:

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

    .global _test_alignas_4byte

    .segment "code"

; function _test_alignas_4byte
; SAC inline storage: 4 bytes
    _test_alignas_4byte__local_0: .word 0
    _test_alignas_4byte__local_1: .word 0
    proc _test_alignas_4byte
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_alignas_simple.c", 2
    .local @_l_x = 0
    .local @_l_y = 2
; .debug_var: __test_alignas_4byte @_l_x offset=0 size=2 type=int8 scope=local
; .debug_var: __test_alignas_4byte @_l_y offset=2 size=2 type=int16 scope=local

@entry:
    .loc "test_alignas_simple.c", 5
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers N, Z
    .frame_size 4
    endproc


__zp_save_buf:

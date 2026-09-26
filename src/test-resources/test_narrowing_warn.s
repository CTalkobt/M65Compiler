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

    .global _main

    .segment "code"

; function _main
; SAC inline storage: 20 bytes
    _main__local_0: .word 0
    _main__local_2: .word 0
    _main__local_3: .word 0
    _main__local_5: .word 0
    _main__local_7: .word 0
    _main__local_9: .word 0
    _main__local_10: .word 0
    _main__local_11: .word 0
    _main__local_12: .word 0
    _main__local_14: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_narrowing_warn.c", 5
    .local @_l_big = 0
    .local @_l_c1 = 2
    .local @_l_c2 = 4
    .local @_l_c3 = 6
    .local @_l_c4 = 12
    .local @_l_c5 = 18
    .local @_l_i2 = 14
    .local @_l_p = 16
    .local @_l_small = 8
    .local @_l_w = 10
; .debug_var: __main @_l_big offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_c1 offset=2 size=2 type=int8 scope=local
; .debug_var: __main @_l_c2 offset=4 size=2 type=int8 scope=local
; .debug_var: __main @_l_c3 offset=6 size=2 type=int8 scope=local
; .debug_var: __main @_l_c4 offset=12 size=2 type=int8 scope=local
; .debug_var: __main @_l_c5 offset=18 size=2 type=int8 scope=local
; .debug_var: __main @_l_i2 offset=14 size=2 type=int16 scope=local
; .debug_var: __main @_l_p offset=16 size=2 type=ptr scope=local
; .debug_var: __main @_l_small offset=8 size=2 type=int8 scope=local
; .debug_var: __main @_l_w offset=10 size=2 type=int16 scope=local

@entry:
    .loc "test_narrowing_warn.c", 7
    lda #244
    sta _main__local_0
    lda #1
    sta _main__local_0+1
    .loc "test_narrowing_warn.c", 8
    lda _main__local_0
    ldx _main__local_0+1
    sta _main__local_2
    .loc "test_narrowing_warn.c", 12
    .loc "test_narrowing_warn.c", 15
    .loc "test_narrowing_warn.c", 18
    .loc "test_narrowing_warn.c", 19
    .loc "test_narrowing_warn.c", 22
    .loc "test_narrowing_warn.c", 25
    .loc "test_narrowing_warn.c", 28
    .loc "test_narrowing_warn.c", 29
    .loc "test_narrowing_warn.c", 31
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers N, Z
    .frame_size 20
    endproc


__zp_save_buf:

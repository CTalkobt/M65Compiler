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
; SAC inline storage: 4 bytes
    _main__local_0: .word 0
    _main__local_2: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_do_while.c", 1
    .local @_l_i = 0
    .local @_l_sum = 2
; .debug_var: __main @_l_i offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_sum offset=2 size=2 type=int16 scope=local

@entry:
    .loc "test_do_while.c", 2
    lda #0
    sta _main__local_0
    sta _main__local_0+1
    .loc "test_do_while.c", 3
    lda #0
    sta _main__local_2
    sta _main__local_2+1
@do_body0:
    .loc "test_do_while.c", 5
    lda _main__local_0
    ldx _main__local_0+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_2
    ldx _main__local_2+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    sta _main__local_2
    stx _main__local_2+1
    .loc "test_do_while.c", 6
    inc.16f __vr0
@do_cond1:
    .loc "test_do_while.c", 7
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, #10
    bcc @do_body0
@do_end2:
    .loc "test_do_while.c", 8
    lda _main__local_2
    ldx _main__local_2+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc


__zp_save_buf:

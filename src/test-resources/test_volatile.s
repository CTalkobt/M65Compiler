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
; SAC inline storage: 8 bytes
    _main__local_0: .word 0
    _main__local_2: .word 0
    _main__local_4: .word 0
    _main__local_6: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_volatile.c", 1
    .local @_l_a = 6
    .local @_l_x = 0
    .local @_l_y = 2
    .local @_l_z = 4
; .debug_var: __main @_l_a offset=6 size=2 type=int16 scope=local
; .debug_var: __main @_l_x offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_y offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_z offset=4 size=2 type=int16 scope=local

@entry:
    .loc "test_volatile.c", 2
    lda #10
    sta _main__local_0
    lda #0
    sta _main__local_0+1
    .loc "test_volatile.c", 3
    lda #20
    sta _main__local_2
    lda #0
    sta _main__local_2+1
    .loc "test_volatile.c", 4
    lda _main__local_2
    ldx _main__local_2+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_0
    ldx _main__local_0+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    sta _main__local_4
    stx _main__local_4+1
    .loc "test_volatile.c", 5
    .loc "test_volatile.c", 8
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 8
    endproc


__zp_save_buf:

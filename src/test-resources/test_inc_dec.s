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
; SAC inline storage: 6 bytes
    _main__local_0: .word 0
    _main__local_7: .word 0
    _main__local_17: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_inc_dec.c", 1
    .local @_l_c = 2
    .local @_l_x = 0
    .local @_l_y = 4
; .debug_var: __main @_l_c offset=2 size=2 type=int8 scope=local
; .debug_var: __main @_l_x offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_y offset=4 size=2 type=int16 scope=local

@entry:
    .loc "test_inc_dec.c", 2
    lda #10
    sta _main__local_0
    lda #0
    sta _main__local_0+1
    .loc "test_inc_dec.c", 3
    lda _main__local_0
    ldx _main__local_0+1
    sta $20
    stx $21
    lda $20
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta _main__local_0
    stx _main__local_0+1
    .loc "test_inc_dec.c", 4
    inc.16f __vr0
    .loc "test_inc_dec.c", 5
    inc.16f __vr0
    .loc "test_inc_dec.c", 7
    lda #5
    sta _main__local_7
    lda #0
    sta _main__local_7+1
    .loc "test_inc_dec.c", 8
    lda _main__local_7
    sta $20
    lda $20
    dec a
    sta $22
    sta _main__local_7
    .loc "test_inc_dec.c", 9
    lda _main__local_7
    dec a
    sta $20
    sta _main__local_7
    .loc "test_inc_dec.c", 10
    lda #1
    sta $20
    lda _main__local_7
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $22
    sec
    sbc $24
    sta $20
    lda $23
    sbc $24+1
    sta $21
    lda $20
    ldx $21
    sta $22
    lda $22
    ldx #0
    sta _main__local_7
    .loc "test_inc_dec.c", 12
    lda _main__local_0
    ldx _main__local_0+1
    sta $20
    stx $21
    lda $20
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta _main__local_0
    stx _main__local_0+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc


__zp_save_buf:

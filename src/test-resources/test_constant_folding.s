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
; SAC inline storage: 34 bytes
    _main__local_0: .word 0
    _main__local_2: .word 0
    _main__local_4: .word 0
    _main__local_6: .word 0
    _main__local_8: .word 0
    _main__local_10: .word 0
    _main__local_12: .word 0
    _main__local_14: .word 0
    _main__local_16: .word 0
    _main__local_18: .word 0
    _main__local_20: .word 0
    _main__local_22: .word 0
    _main__local_24: .word 0
    _main__local_26: .word 0
    _main__local_28: .word 0
    _main__local_30: .word 0
    _main__local_33: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_constant_folding.c", 1
    .local @_l_a = 0
    .local @_l_b = 2
    .local @_l_c = 4
    .local @_l_d = 6
    .local @_l_e = 8
    .local @_l_f = 10
    .local @_l_g = 12
    .local @_l_h = 14
    .local @_l_i = 16
    .local @_l_j = 18
    .local @_l_k = 20
    .local @_l_l = 22
    .local @_l_m = 24
    .local @_l_n = 26
    .local @_l_o = 28
    .local @_l_p = 30
    .local @_l_x = 32
; .debug_var: __main @_l_a offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_b offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_c offset=4 size=2 type=int16 scope=local
; .debug_var: __main @_l_d offset=6 size=2 type=int16 scope=local
; .debug_var: __main @_l_e offset=8 size=2 type=int16 scope=local
; .debug_var: __main @_l_f offset=10 size=2 type=int16 scope=local
; .debug_var: __main @_l_g offset=12 size=2 type=int16 scope=local
; .debug_var: __main @_l_h offset=14 size=2 type=int16 scope=local
; .debug_var: __main @_l_i offset=16 size=2 type=int16 scope=local
; .debug_var: __main @_l_j offset=18 size=2 type=int16 scope=local
; .debug_var: __main @_l_k offset=20 size=2 type=int16 scope=local
; .debug_var: __main @_l_l offset=22 size=2 type=int16 scope=local
; .debug_var: __main @_l_m offset=24 size=2 type=int16 scope=local
; .debug_var: __main @_l_n offset=26 size=2 type=int16 scope=local
; .debug_var: __main @_l_o offset=28 size=2 type=int16 scope=local
; .debug_var: __main @_l_p offset=30 size=2 type=int16 scope=local
; .debug_var: __main @_l_x offset=32 size=2 type=int16 scope=local

@entry:
    .loc "test_constant_folding.c", 3
    lda #3
    sta _main__local_0
    lda #0
    sta _main__local_0+1
    .loc "test_constant_folding.c", 4
    lda #5
    sta _main__local_2
    lda #0
    sta _main__local_2+1
    .loc "test_constant_folding.c", 5
    lda #12
    sta _main__local_4
    lda #0
    sta _main__local_4+1
    .loc "test_constant_folding.c", 6
    lda #10
    sta _main__local_6
    lda #0
    sta _main__local_6+1
    .loc "test_constant_folding.c", 9
    lda #15
    sta _main__local_8
    lda #0
    sta _main__local_8+1
    .loc "test_constant_folding.c", 12
    lda #16
    sta _main__local_10
    lda #0
    sta _main__local_10+1
    .loc "test_constant_folding.c", 13
    lda #32
    sta _main__local_12
    lda #0
    sta _main__local_12+1
    .loc "test_constant_folding.c", 14
    lda #15
    sta _main__local_14
    lda #0
    sta _main__local_14+1
    .loc "test_constant_folding.c", 15
    lda #255
    sta _main__local_16
    lda #0
    sta _main__local_16+1
    .loc "test_constant_folding.c", 16
    lda #85
    sta _main__local_18
    lda #0
    sta _main__local_18+1
    .loc "test_constant_folding.c", 19
    lda #1
    sta _main__local_20
    lda #0
    sta _main__local_20+1
    .loc "test_constant_folding.c", 20
    lda #0
    sta _main__local_22
    sta _main__local_22+1
    .loc "test_constant_folding.c", 21
    lda #1
    sta _main__local_24
    lda #0
    sta _main__local_24+1
    .loc "test_constant_folding.c", 22
    lda #0
    sta _main__local_26
    sta _main__local_26+1
    .loc "test_constant_folding.c", 25
    lda #0
    sta _main__local_28
    sta _main__local_28+1
    .loc "test_constant_folding.c", 26
    lda #1
    sta _main__local_30
    lda #0
    sta _main__local_30+1
    .loc "test_constant_folding.c", 34
    lda #42
    sta _main__local_2
    lda #0
    sta _main__local_2+1
    .loc "test_constant_folding.c", 43
    .loc "test_constant_folding.c", 47
    lda _main__local_2
    ldx _main__local_2+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_0
    ldx _main__local_0+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda _main__local_4
    ldx _main__local_4+1
    add.16 .AX, $20
    sta $22
    stx $23
    lda _main__local_6
    ldx _main__local_6+1
    add.16 .AX, $22
    sta $20
    stx $21
    lda _main__local_8
    ldx _main__local_8+1
    add.16 .AX, $20
    sta $22
    stx $23
    lda _main__local_10
    ldx _main__local_10+1
    add.16 .AX, $22
    sta $20
    stx $21
    lda _main__local_12
    ldx _main__local_12+1
    add.16 .AX, $20
    sta $22
    stx $23
    lda _main__local_14
    ldx _main__local_14+1
    add.16 .AX, $22
    sta $20
    stx $21
    lda _main__local_16
    ldx _main__local_16+1
    add.16 .AX, $20
    sta $22
    stx $23
    lda _main__local_18
    ldx _main__local_18+1
    add.16 .AX, $22
    sta $20
    stx $21
    lda _main__local_20
    ldx _main__local_20+1
    add.16 .AX, $20
    sta $22
    stx $23
    lda _main__local_22
    ldx _main__local_22+1
    add.16 .AX, $22
    sta $20
    stx $21
    lda _main__local_24
    ldx _main__local_24+1
    add.16 .AX, $20
    sta $22
    stx $23
    lda _main__local_26
    ldx _main__local_26+1
    add.16 .AX, $22
    sta $20
    stx $21
    lda _main__local_28
    ldx _main__local_28+1
    add.16 .AX, $20
    sta $22
    stx $23
    lda _main__local_30
    ldx _main__local_30+1
    add.16 .AX, $22
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 34
    endproc


__zp_save_buf:

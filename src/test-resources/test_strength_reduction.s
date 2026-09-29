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
; SAC inline storage: 16 bytes
    _main__local_0: .word 0
    _main__local_2: .word 0
    _main__local_5: .word 0
    _main__local_8: .word 0
    _main__local_11: .word 0
    _main__local_14: .word 0
    _main__local_17: .word 0
    _main__local_18: .word 0
    _main__local_19: .word 0
    _main__local_20: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_strength_reduction.c", 1
    .local @_l_a = 0
    .local @_l_b = 2
    .local @_l_c = 4
    .local @_l_d = 6
    .local @_l_e = 8
    .local @_l_f = 10
    .local @_l_g = 12
    .local @_l_h = 14
; .debug_var: __main @_l_a offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_b offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_c offset=4 size=2 type=int16 scope=local
; .debug_var: __main @_l_d offset=6 size=2 type=int16 scope=local
; .debug_var: __main @_l_e offset=8 size=2 type=int16 scope=local
; .debug_var: __main @_l_f offset=10 size=2 type=int16 scope=local
; .debug_var: __main @_l_g offset=12 size=2 type=int16 scope=local
; .debug_var: __main @_l_h offset=14 size=2 type=int16 scope=local

@entry:
    .loc "test_strength_reduction.c", 2
    lda #10
    sta _main__local_0
    lda #0
    sta _main__local_0+1
    .loc "test_strength_reduction.c", 3
    lda #2
    ldx #0
    sta $20
    stx $21
    lda _main__local_0
    ldx _main__local_0+1
    lsl.16 .AX
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_2
    stx _main__local_2+1
    .loc "test_strength_reduction.c", 4
    lda #4
    ldx #0
    sta $20
    stx $21
    lda _main__local_0
    ldx _main__local_0+1
    lsl.16 .AX
    lsl.16 .AX
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_5
    stx _main__local_5+1
    .loc "test_strength_reduction.c", 5
    lda #3
    ldx #0
    sta $22
    stx $23
    lda _main__local_0
    ldx _main__local_0+1
    mul.16 .AX, $22
    sta $24
    stx $25
    lda $24
    ldx $25
    sta _main__local_8
    stx _main__local_8+1
    .loc "test_strength_reduction.c", 6
    lda #2
    ldx #0
    sta $22
    stx $23
    lda _main__local_0
    ldx _main__local_0+1
    lsr.16 .AX
    sta $22
    stx $23
    lda $22
    ldx $23
    sta _main__local_11
    stx _main__local_11+1
    .loc "test_strength_reduction.c", 7
    lda #4
    ldx #0
    sta $24
    stx $25
    lda _main__local_0
    ldx _main__local_0+1
    and.16 .AX, #3
    sta $24
    stx $25
    lda $24
    ldx $25
    sta _main__local_14
    stx _main__local_14+1
    .loc "test_strength_reduction.c", 8
    lda $20
    ldx $21
    sta _main__local_17
    stx _main__local_17+1
    .loc "test_strength_reduction.c", 9
    lda $22
    ldx $23
    sta _main__local_19
    stx _main__local_19+1
    .loc "test_strength_reduction.c", 11
    lda _main__local_5
    ldx _main__local_5+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_2
    ldx _main__local_2+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda _main__local_8
    ldx _main__local_8+1
    add.16 .AX, $20
    sta $22
    stx $23
    lda _main__local_11
    ldx _main__local_11+1
    add.16 .AX, $22
    sta $20
    stx $21
    lda _main__local_14
    ldx _main__local_14+1
    add.16 .AX, $20
    sta $22
    stx $23
    lda _main__local_17
    ldx _main__local_17+1
    add.16 .AX, $22
    sta $20
    stx $21
    lda _main__local_19
    ldx _main__local_19+1
    add.16 .AX, $20
    sta $22
    stx $23
    lda $22
    ldx $23
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 16
    endproc


__zp_save_buf:

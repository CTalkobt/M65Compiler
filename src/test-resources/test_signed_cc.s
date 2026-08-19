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

    .global _zero
    .global _main

    .segment "data"
    .byte 0
_zero:
; .debug_var: @global _zero offset=0 size=2 type=int16 scope=global
    .word 0

    .segment "code"

; function _main
; SAC inline storage: 8 bytes
    _main__local_0: .word 0
    _main__local_4: .word 0
    _main__local_9: .word 0
    _main__local_13: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_signed_cc.c", 3
    .local @_l_a = 0
    .local @_l_b = 2
    .local @_l_ua = 4
    .local @_l_ub = 6
; .debug_var: __main @_l_a offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_b offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_ua offset=4 size=2 type=int16 scope=local
; .debug_var: __main @_l_ub offset=6 size=2 type=int16 scope=local

@entry:
    .loc "test_signed_cc.c", 4
    lda #156
    ldx #255
    sta $20
    stx $21
    lda _zero
    ldx _zero+1
    sta $22
    stx $23
    lda $20
    clc
    adc $22
    sta $24
    lda $21
    adc $22+1
    sta $25
    lda $24
    ldx $25
    sta _main__local_0
    stx _main__local_0+1
    .loc "test_signed_cc.c", 5
    lda #50
    ldx #0
    sta $20
    stx $21
    lda _zero
    ldx _zero+1
    sta $22
    stx $23
    lda $20
    clc
    adc $22
    sta $24
    lda $21
    adc $22+1
    sta $25
    lda $24
    ldx $25
    sta _main__local_4
    stx _main__local_4+1
    .loc "test_signed_cc.c", 8
    lda _main__local_4
    ldx _main__local_4+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_0
    ldx _main__local_0+1
    cmp.s16 .AX, __zp_scratch2
    bcc @if_end2
@if_else1:
    .loc "test_signed_cc.c", 11
    lda #1
    ldx #0
    bra @__return
@if_end2:
    .loc "test_signed_cc.c", 14
    lda #156
    ldx #255
    sta $20
    stx $21
    lda _zero
    ldx _zero+1
    sta $22
    stx $23
    lda $20
    clc
    adc $22
    sta $24
    lda $21
    adc $22+1
    sta $25
    lda $24
    ldx $25
    sta _main__local_9
    stx _main__local_9+1
    .loc "test_signed_cc.c", 15
    lda #50
    ldx #0
    sta $20
    stx $21
    lda _zero
    ldx _zero+1
    sta $22
    stx $23
    lda $20
    clc
    adc $22
    sta $24
    lda $21
    adc $22+1
    sta $25
    lda $24
    ldx $25
    sta _main__local_13
    stx _main__local_13+1
    .loc "test_signed_cc.c", 18
    lda _main__local_13
    ldx _main__local_13+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_9
    ldx _main__local_9+1
    cmp.16 .AX, __zp_scratch2
    bcc @if_then3
    bra @if_end5
@if_then3:
    .loc "test_signed_cc.c", 19
    lda #2
    ldx #0
    bra @__return
@if_end5:
    .loc "test_signed_cc.c", 24
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

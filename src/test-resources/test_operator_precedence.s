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
; SAC inline storage: 32 bytes
    _main__local_0: .word 0
    _main__local_4: .word 0
    _main__local_8: .word 0
    _main__local_12: .word 0
    _main__local_16: .word 0
    _main__local_20: .word 0
    _main__local_24: .word 0
    _main__local_28: .word 0
    _main__local_32: .word 0
    _main__local_36: .word 0
    _main__local_40: .word 0
    _main__local_44: .word 0
    _main__local_46: .word 0
    _main__local_48: .word 0
    _main__local_50: .word 0
    _main__local_52: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_operator_precedence.c", 4
    .local @_l_a = 22
    .local @_l_b = 24
    .local @_l_c = 26
    .local @_l_d = 28
    .local @_l_r1 = 0
    .local @_l_r10 = 18
    .local @_l_r11 = 20
    .local @_l_r12 = 30
    .local @_l_r2 = 2
    .local @_l_r3 = 4
    .local @_l_r4 = 6
    .local @_l_r5 = 8
    .local @_l_r6 = 10
    .local @_l_r7 = 12
    .local @_l_r8 = 14
    .local @_l_r9 = 16
; .debug_var: __main @_l_a offset=22 size=2 type=int16 scope=local
; .debug_var: __main @_l_b offset=24 size=2 type=int16 scope=local
; .debug_var: __main @_l_c offset=26 size=2 type=int16 scope=local
; .debug_var: __main @_l_d offset=28 size=2 type=int16 scope=local
; .debug_var: __main @_l_r1 offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_r10 offset=18 size=2 type=int16 scope=local
; .debug_var: __main @_l_r11 offset=20 size=2 type=int16 scope=local
; .debug_var: __main @_l_r12 offset=30 size=2 type=int16 scope=local
; .debug_var: __main @_l_r2 offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_r3 offset=4 size=2 type=int16 scope=local
; .debug_var: __main @_l_r4 offset=6 size=2 type=int16 scope=local
; .debug_var: __main @_l_r5 offset=8 size=2 type=int16 scope=local
; .debug_var: __main @_l_r6 offset=10 size=2 type=int16 scope=local
; .debug_var: __main @_l_r7 offset=12 size=2 type=int16 scope=local
; .debug_var: __main @_l_r8 offset=14 size=2 type=int16 scope=local
; .debug_var: __main @_l_r9 offset=16 size=2 type=int16 scope=local

@entry:
    .loc "test_operator_precedence.c", 6
    lda #14
    sta _main__local_0
    lda #0
    sta _main__local_0+1
    .loc "test_operator_precedence.c", 7
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, #14
    bne @if_then0
    bra @if_end2
@if_then0:
    lda #1
    ldx #0
    bra @__return
@if_end2:
    .loc "test_operator_precedence.c", 10
    lda #4
    sta _main__local_4
    lda #0
    sta _main__local_4+1
    .loc "test_operator_precedence.c", 11
    lda _main__local_4
    ldx _main__local_4+1
    cmp.16 .AX, #4
    bne @if_then3
    bra @if_end5
@if_then3:
    lda #2
    ldx #0
    bra @__return
@if_end5:
    .loc "test_operator_precedence.c", 14
    lda #12
    sta _main__local_8
    lda #0
    sta _main__local_8+1
    .loc "test_operator_precedence.c", 15
    lda _main__local_8
    ldx _main__local_8+1
    cmp.16 .AX, #12
    bne @if_then6
    bra @if_end8
@if_then6:
    lda #3
    ldx #0
    bra @__return
@if_end8:
    .loc "test_operator_precedence.c", 18
    lda #1
    sta _main__local_12
    lda #0
    sta _main__local_12+1
    .loc "test_operator_precedence.c", 19
    lda _main__local_12
    ldx _main__local_12+1
    cmp.16 .AX, #1
    bne @if_then9
    bra @if_end11
@if_then9:
    lda #4
    ldx #0
    bra @__return
@if_end11:
    .loc "test_operator_precedence.c", 22
    lda #3
    sta _main__local_16
    lda #0
    sta _main__local_16+1
    .loc "test_operator_precedence.c", 23
    lda _main__local_16
    ldx _main__local_16+1
    cmp.16 .AX, #3
    bne @if_then12
    bra @if_end14
@if_then12:
    lda #5
    ldx #0
    bra @__return
@if_end14:
    .loc "test_operator_precedence.c", 28
    lda #2
    sta _main__local_20
    lda #0
    sta _main__local_20+1
    .loc "test_operator_precedence.c", 29
    lda _main__local_20
    ldx _main__local_20+1
    cmp.16 .AX, #2
    bne @if_then15
    bra @if_end17
@if_then15:
    lda #6
    ldx #0
    bra @__return
@if_end17:
    .loc "test_operator_precedence.c", 39
    lda #24
    sta _main__local_24
    lda #0
    sta _main__local_24+1
    .loc "test_operator_precedence.c", 40
    lda _main__local_24
    ldx _main__local_24+1
    cmp.16 .AX, #24
    bne @if_then18
    bra @if_end20
@if_then18:
    lda #7
    ldx #0
    bra @__return
@if_end20:
    .loc "test_operator_precedence.c", 43
    lda #20
    sta _main__local_28
    lda #0
    sta _main__local_28+1
    .loc "test_operator_precedence.c", 44
    lda _main__local_28
    ldx _main__local_28+1
    cmp.16 .AX, #20
    bne @if_then21
    bra @if_end23
@if_then21:
    lda #8
    ldx #0
    bra @__return
@if_end23:
    .loc "test_operator_precedence.c", 47
    lda #1
    sta _main__local_32
    lda #0
    sta _main__local_32+1
    .loc "test_operator_precedence.c", 50
    lda _main__local_32
    ldx _main__local_32+1
    cmp.16 .AX, #1
    bne @if_then24
    bra @if_end26
@if_then24:
    lda #9
    ldx #0
    bra @__return
@if_end26:
    .loc "test_operator_precedence.c", 53
    lda #250
    sta _main__local_36
    lda #255
    sta _main__local_36+1
    .loc "test_operator_precedence.c", 54
    lda _main__local_36
    ldx _main__local_36+1
    cmp.16 .AX, #-6
    bne @if_then27
    bra @if_end29
@if_then27:
    lda #10
    ldx #0
    bra @__return
@if_end29:
    .loc "test_operator_precedence.c", 58
    lda #1
    sta _main__local_40
    lda #0
    sta _main__local_40+1
    .loc "test_operator_precedence.c", 59
    lda _main__local_40
    ldx _main__local_40+1
    cmp.16 .AX, #1
    bne @if_then30
    bra @if_end32
@if_then30:
    lda #11
    ldx #0
    bra @__return
@if_end32:
    .loc "test_operator_precedence.c", 62
    lda #2
    sta _main__local_44
    lda #0
    sta _main__local_44+1
    .loc "test_operator_precedence.c", 63
    lda #3
    sta _main__local_46
    lda #0
    sta _main__local_46+1
    .loc "test_operator_precedence.c", 64
    lda #4
    sta _main__local_48
    lda #0
    sta _main__local_48+1
    .loc "test_operator_precedence.c", 65
    lda #5
    sta _main__local_50
    lda #0
    sta _main__local_50+1
    .loc "test_operator_precedence.c", 66
    lda _main__local_48
    ldx _main__local_48+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_46
    ldx _main__local_46+1
    mul.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda _main__local_44
    ldx _main__local_44+1
    add.16 .AX, $20
    sta $22
    stx $23
    lda _main__local_50
    ldx _main__local_50+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $22
    ldx $23
    sub.16 .AX, __zp_scratch2
    sta $20
    stx $21
    sta _main__local_52
    stx _main__local_52+1
    .loc "test_operator_precedence.c", 67
    lda _main__local_52
    ldx _main__local_52+1
    cmp.16 .AX, #9
    bne @if_then33
    bra @if_end35
@if_then33:
    lda #12
    ldx #0
    bra @__return
@if_end35:
    .loc "test_operator_precedence.c", 69
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 32
    endproc


__zp_save_buf:

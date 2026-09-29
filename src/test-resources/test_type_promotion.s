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
; SAC inline storage: 42 bytes
    _main__local_0: .word 0
    _main__local_2: .word 0
    _main__local_4: .word 0
    _main__local_9: .word 0
    _main__local_11: .word 0
    _main__local_18: .word 0
    _main__local_20: .word 0
    _main__local_25: .word 0
    _main__local_27: .word 0
    _main__local_31: .word 0
    _main__local_33: .word 0
    _main__local_35: .word 0
    _main__local_43: .word 0
    _main__local_49: .word 0
    _main__local_51: .word 0
    _main__local_52: .word 0
    _main__local_59: .word 0
    _main__local_61: .word 0
    _main__local_64: .word 0
    _main__local_66: .word 0
    _main__local_71: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_type_promotion.c", 4
    .local @_l_a = 18
    .local @_l_b = 20
    .local @_l_big = 10
    .local @_l_c1 = 0
    .local @_l_c2 = 6
    .local @_l_c3 = 14
    .local @_l_cond_result = 38
    .local @_l_ff = 32
    .local @_l_flag = 36
    .local @_l_i1 = 2
    .local @_l_i3 = 16
    .local @_l_low = 12
    .local @_l_r1 = 4
    .local @_l_r2 = 8
    .local @_l_sum_c = 22
    .local @_l_sum_i = 24
    .local @_l_wide = 34
    .local @_l_x = 26
    .local @_l_x_trunc = 28
    .local @_l_y = 30
    .local @_l_zero = 40
; .debug_var: __main @_l_a offset=18 size=2 type=int8 scope=local
; .debug_var: __main @_l_b offset=20 size=2 type=int8 scope=local
; .debug_var: __main @_l_big offset=10 size=2 type=int16 scope=local
; .debug_var: __main @_l_c1 offset=0 size=2 type=int8 scope=local
; .debug_var: __main @_l_c2 offset=6 size=2 type=int8 scope=local
; .debug_var: __main @_l_c3 offset=14 size=2 type=int8 scope=local
; .debug_var: __main @_l_cond_result offset=38 size=2 type=int16 scope=local
; .debug_var: __main @_l_ff offset=32 size=2 type=int8 scope=local
; .debug_var: __main @_l_flag offset=36 size=2 type=int8 scope=local
; .debug_var: __main @_l_i1 offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_i3 offset=16 size=2 type=int16 scope=local
; .debug_var: __main @_l_low offset=12 size=2 type=int8 scope=local
; .debug_var: __main @_l_r1 offset=4 size=2 type=int16 scope=local
; .debug_var: __main @_l_r2 offset=8 size=2 type=int16 scope=local
; .debug_var: __main @_l_sum_c offset=22 size=2 type=int8 scope=local
; .debug_var: __main @_l_sum_i offset=24 size=2 type=int16 scope=local
; .debug_var: __main @_l_wide offset=34 size=2 type=int16 scope=local
; .debug_var: __main @_l_x offset=26 size=2 type=int16 scope=local
; .debug_var: __main @_l_x_trunc offset=28 size=2 type=int8 scope=local
; .debug_var: __main @_l_y offset=30 size=2 type=int16 scope=local
; .debug_var: __main @_l_zero offset=40 size=2 type=int8 scope=local

@entry:
    .loc "test_type_promotion.c", 6
    lda #200
    sta _main__local_0
    lda #0
    sta _main__local_0+1
    .loc "test_type_promotion.c", 7
    lda #44
    sta _main__local_2
    lda #1
    sta _main__local_2+1
    lda _main__local_0
    ldx #0
    sta $20
    stx $21
    .loc "test_type_promotion.c", 8
    lda _main__local_2
    ldx _main__local_2+1
    add.16 .AX, $20
    sta $22
    stx $23
    sta _main__local_4
    stx _main__local_4+1
    .loc "test_type_promotion.c", 9
    lda _main__local_4
    ldx _main__local_4+1
    cmp.16 .AX, #500
    bne @if_then0
    bra @if_end2
@if_then0:
    lda #1
    ldx #0
    bra @__return
@if_end2:
    .loc "test_type_promotion.c", 12
    lda #10
    sta _main__local_9
    lda #0
    sta _main__local_9+1
    .loc "test_type_promotion.c", 13
    lda #30
    sta $20
    lda _main__local_9
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    mul.16 .AX, $24
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_11
    stx _main__local_11+1
    .loc "test_type_promotion.c", 14
    lda _main__local_11
    ldx _main__local_11+1
    cmp.16 .AX, #300
    bne @if_then3
    bra @if_end5
@if_then3:
    lda #2
    ldx #0
    bra @__return
@if_end5:
    .loc "test_type_promotion.c", 17
    lda #52
    sta _main__local_18
    lda #18
    sta _main__local_18+1
    .loc "test_type_promotion.c", 18
    lda _main__local_18
    ldx _main__local_18+1
    sta _main__local_20
    .loc "test_type_promotion.c", 19
    lda #52
    sta $20
    lda _main__local_20
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    cmp.16 .AX, $24
    bne @if_then6
    bra @if_end8
@if_then6:
    lda #3
    ldx #0
    bra @__return
@if_end8:
    .loc "test_type_promotion.c", 22
    lda #255
    sta _main__local_25
    lda #0
    sta _main__local_25+1
    .loc "test_type_promotion.c", 23
    lda #255
    sta _main__local_27
    lda #0
    sta _main__local_27+1
    lda _main__local_25
    ldx #0
    sta $20
    stx $21
    .loc "test_type_promotion.c", 24
    lda _main__local_27
    ldx _main__local_27+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx $21
    cmp.16 .AX, __zp_scratch2
    bne @if_then9
    bra @if_end11
@if_then9:
    lda #4
    ldx #0
    bra @__return
@if_end11:
    .loc "test_type_promotion.c", 27
    lda #200
    sta _main__local_31
    lda #0
    sta _main__local_31+1
    .loc "test_type_promotion.c", 28
    lda #100
    sta _main__local_33
    lda #0
    sta _main__local_33+1
    lda _main__local_31
    ldx #0
    sta $20
    stx $21
    lda _main__local_33
    ldx #0
    sta $22
    stx $23
    .loc "test_type_promotion.c", 29
    lda $20
    clc
    adc $22
    sta $24
    lda $21
    adc $22+1
    sta $25
    lda $24
    ldx $25
    sta _main__local_35
    .loc "test_type_promotion.c", 30
    lda #44
    sta $20
    lda _main__local_35
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    cmp.16 .AX, $24
    bne @if_then12
    bra @if_end14
@if_then12:
    lda #5
    ldx #0
    bra @__return
@if_end14:
    lda _main__local_31
    ldx #0
    sta $20
    stx $21
    lda _main__local_33
    ldx #0
    sta $22
    stx $23
    .loc "test_type_promotion.c", 33
    lda $20
    clc
    adc $22
    sta $24
    lda $21
    adc $22+1
    sta $25
    lda $24
    ldx $25
    sta _main__local_43
    stx _main__local_43+1
    .loc "test_type_promotion.c", 34
    lda _main__local_43
    ldx _main__local_43+1
    cmp.16 .AX, #300
    bne @if_then15
    bra @if_end17
@if_then15:
    lda #6
    ldx #0
    bra @__return
@if_end17:
    .loc "test_type_promotion.c", 37
    lda #232
    sta _main__local_49
    lda #3
    sta _main__local_49+1
    .loc "test_type_promotion.c", 38
    lda _main__local_49
    ldx _main__local_49+1
    sta _main__local_51
    .loc "test_type_promotion.c", 39
    lda #1
    sta $20
    lda _main__local_51
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $22
    clc
    adc $24
    sta $20
    lda $23
    adc $24+1
    sta $21
    lda $20
    ldx $21
    sta _main__local_52
    stx _main__local_52+1
    .loc "test_type_promotion.c", 40
    lda _main__local_52
    ldx _main__local_52+1
    cmp.16 .AX, #233
    bne @if_then18
    bra @if_end20
@if_then18:
    lda #7
    ldx #0
    bra @__return
@if_end20:
    .loc "test_type_promotion.c", 43
    lda #255
    sta _main__local_59
    lda #0
    sta _main__local_59+1
    .loc "test_type_promotion.c", 44
    lda _main__local_59
    sta _main__local_61
    stx _main__local_61+1
    .loc "test_type_promotion.c", 45
    lda _main__local_61
    ldx _main__local_61+1
    cmp.16 .AX, #255
    bne @if_then21
    bra @if_end23
@if_then21:
    lda #8
    ldx #0
    bra @__return
@if_end23:
    .loc "test_type_promotion.c", 48
    lda #1
    sta _main__local_64
    lda #0
    sta _main__local_64+1
    .loc "test_type_promotion.c", 49
    lda #0
    sta _main__local_66
    sta _main__local_66+1
    .loc "test_type_promotion.c", 50
    lda _main__local_64
    bne @if_then24
    bra @if_end26
@if_then24:
    .loc "test_type_promotion.c", 51
    lda #42
    sta _main__local_66
    lda #0
    sta _main__local_66+1
@if_end26:
    .loc "test_type_promotion.c", 53
    lda _main__local_66
    ldx _main__local_66+1
    cmp.16 .AX, #42
    bne @if_then27
    bra @if_end29
@if_then27:
    lda #9
    ldx #0
    bra @__return
@if_end29:
    .loc "test_type_promotion.c", 56
    lda #0
    sta _main__local_71
    sta _main__local_71+1
    .loc "test_type_promotion.c", 57
    lda #99
    sta _main__local_66
    lda #0
    sta _main__local_66+1
    .loc "test_type_promotion.c", 58
    lda _main__local_71
    bne @if_then30
    bra @if_end32
@if_then30:
    .loc "test_type_promotion.c", 59
    lda #0
    sta _main__local_66
    sta _main__local_66+1
@if_end32:
    .loc "test_type_promotion.c", 61
    lda _main__local_66
    ldx _main__local_66+1
    cmp.16 .AX, #99
    bne @if_then33
    bra @if_end35
@if_then33:
    lda #10
    ldx #0
    bra @__return
@if_end35:
    .loc "test_type_promotion.c", 63
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 42
    endproc


__zp_save_buf:

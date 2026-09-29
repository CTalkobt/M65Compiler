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
; SAC inline storage: 36 bytes
    _main__local_0: .word 0
    _main__local_2: .word 0
    _main__local_8: .word 0
    _main__local_10: .word 0
    _main__local_14: .word 0
    _main__local_18: .word 0
    _main__local_20: .word 0
    _main__local_22: .word 0
    _main__local_28: .word 0
    _main__local_35: .word 0
    _main__local_37: .word 0
    _main__local_43: .word 0
    _main__local_45: .word 0
    _main__local_53: .word 0
    _main__local_55: .word 0
    _main__local_57: .word 0
    _main__local_61: .word 0
    _main__local_63: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "src/test-resources/test_cast.c", 4
    .local @_l_a = 10
    .local @_l_addr = 30
    .local @_l_arr = 26
    .local @_l_b = 12
    .local @_l_big = 0
    .local @_l_c = 4
    .local @_l_c1 = 32
    .local @_l_low = 2
    .local @_l_n = 18
    .local @_l_n_lo = 20
    .local @_l_orig = 22
    .local @_l_p = 28
    .local @_l_r = 34
    .local @_l_sum = 14
    .local @_l_tmp = 24
    .local @_l_val = 16
    .local @_l_wide = 6
    .local @_l_x = 8
; .debug_var: __main @_l_a offset=10 size=2 type=int8 scope=local
; .debug_var: __main @_l_addr offset=30 size=2 type=int16 scope=local
; .debug_var: __main @_l_arr offset=26 size=2 type=int8 scope=local
; .debug_var: __main @_l_b offset=12 size=2 type=int8 scope=local
; .debug_var: __main @_l_big offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_c offset=4 size=2 type=int8 scope=local
; .debug_var: __main @_l_c1 offset=32 size=2 type=int8 scope=local
; .debug_var: __main @_l_low offset=2 size=2 type=int8 scope=local
; .debug_var: __main @_l_n offset=18 size=2 type=int16 scope=local
; .debug_var: __main @_l_n_lo offset=20 size=2 type=int8 scope=local
; .debug_var: __main @_l_orig offset=22 size=2 type=int16 scope=local
; .debug_var: __main @_l_p offset=28 size=2 type=ptr scope=local
; .debug_var: __main @_l_r offset=34 size=2 type=int16 scope=local
; .debug_var: __main @_l_sum offset=14 size=2 type=int16 scope=local
; .debug_var: __main @_l_tmp offset=24 size=2 type=int8 scope=local
; .debug_var: __main @_l_val offset=16 size=2 type=int16 scope=local
; .debug_var: __main @_l_wide offset=6 size=2 type=int16 scope=local
; .debug_var: __main @_l_x offset=8 size=2 type=int16 scope=local

@entry:
    .loc "src/test-resources/test_cast.c", 6
    lda #52
    sta _main__local_0
    lda #18
    sta _main__local_0+1
    .loc "src/test-resources/test_cast.c", 7
    lda _main__local_0
    ldx _main__local_0+1
    sta _main__local_2
    .loc "src/test-resources/test_cast.c", 8
    lda #52
    lda _main__local_2
    ldx #0
    ldx #0
    sta __zp_scratch2
    stx __zp_scratch2+1
    cmp.16 .AX, __zp_scratch2
    bne @if_then0
    bra @if_end2
@if_then0:
    lda #1
    ldx #0
    bra @__return
@if_end2:
    .loc "src/test-resources/test_cast.c", 11
    lda #200
    sta _main__local_8
    lda #0
    sta _main__local_8+1
    .loc "src/test-resources/test_cast.c", 12
    lda _main__local_8
    ldx #0
    sta _main__local_10
    stx _main__local_10+1
    .loc "src/test-resources/test_cast.c", 13
    lda _main__local_10
    ldx _main__local_10+1
    cmp.16 .AX, #200
    bne @if_then3
    bra @if_end5
@if_then3:
    lda #2
    ldx #0
    bra @__return
@if_end5:
    .loc "src/test-resources/test_cast.c", 16
    lda #255
    sta _main__local_14
    stx _main__local_14+1
    .loc "src/test-resources/test_cast.c", 17
    lda _main__local_14
    ldx _main__local_14+1
    cmp.16 .AX, #255
    bne @if_then6
    bra @if_end8
@if_then6:
    lda #3
    ldx #0
    bra @__return
@if_end8:
    .loc "src/test-resources/test_cast.c", 20
    lda #100
    sta _main__local_18
    lda #0
    sta _main__local_18+1
    .loc "src/test-resources/test_cast.c", 21
    lda #150
    sta _main__local_20
    lda #0
    sta _main__local_20+1
    .loc "src/test-resources/test_cast.c", 22
    lda _main__local_18
    ldx #0
    lda _main__local_20
    ldx #0
    sta __zp_scratch2
    stx __zp_scratch2+1
    add.16 .AX, __zp_scratch2
    sta _main__local_22
    stx _main__local_22+1
    .loc "src/test-resources/test_cast.c", 23
    lda _main__local_22
    ldx _main__local_22+1
    cmp.16 .AX, #250
    bne @if_then9
    bra @if_end11
@if_then9:
    lda #4
    ldx #0
    bra @__return
@if_end11:
    .loc "src/test-resources/test_cast.c", 26
    lda #2
    sta _main__local_28
    lda #1
    sta _main__local_28+1
    .loc "src/test-resources/test_cast.c", 27
    lda _main__local_28
    ldx _main__local_28+1
    lda #2
    ldx #0
    ldx #0
    sta __zp_scratch2
    stx __zp_scratch2+1
    cmp.16 .AX, __zp_scratch2
    bne @if_then12
    bra @if_end14
@if_then12:
    lda #5
    ldx #0
    bra @__return
@if_end14:
    .loc "src/test-resources/test_cast.c", 30
    lda #205
    sta _main__local_35
    lda #171
    sta _main__local_35+1
    .loc "src/test-resources/test_cast.c", 31
    lda _main__local_35
    ldx _main__local_35+1
    sta _main__local_37
    .loc "src/test-resources/test_cast.c", 32
    lda #205
    lda _main__local_37
    ldx #0
    ldx #0
    sta __zp_scratch2
    stx __zp_scratch2+1
    cmp.16 .AX, __zp_scratch2
    bne @if_then15
    bra @if_end17
@if_then15:
    lda #6
    ldx #0
    bra @__return
@if_end17:
    .loc "src/test-resources/test_cast.c", 35
    lda #120
    sta _main__local_43
    lda #86
    sta _main__local_43+1
    .loc "src/test-resources/test_cast.c", 36
    lda _main__local_43
    ldx _main__local_43+1
    sta _main__local_45
    .loc "src/test-resources/test_cast.c", 37
    lda _main__local_43
    ldx _main__local_43+1
    cmp.16 .AX, #22136
    bne @if_then18
    bra @if_end20
@if_then18:
    lda #7
    ldx #0
    bra @__return
@if_end20:
    .loc "src/test-resources/test_cast.c", 38
    lda #120
    lda _main__local_45
    ldx #0
    ldx #0
    sta __zp_scratch2
    stx __zp_scratch2+1
    cmp.16 .AX, __zp_scratch2
    bne @if_then21
    bra @if_end23
@if_then21:
    lda #8
    ldx #0
    bra @__return
@if_end23:
    .loc "src/test-resources/test_cast.c", 41
    lda #42
    sta _main__local_53
    lda #0
    sta _main__local_53+1
    .loc "src/test-resources/test_cast.c", 42
    leax.local 26
    sta _main__local_55
    stx _main__local_55+1
    .loc "src/test-resources/test_cast.c", 43
    lda _main__local_55
    ldx _main__local_55+1
    sta _main__local_57
    stx _main__local_57+1
    .loc "src/test-resources/test_cast.c", 44
    lda _main__local_57
    ldx _main__local_57+1
    stx __zp_scratch
    ora __zp_scratch
    beq @if_then24
    bra @if_end26
@if_then24:
    lda #9
    ldx #0
    bra @__return
@if_end26:
    .loc "src/test-resources/test_cast.c", 47
    lda #10
    sta _main__local_61
    lda #0
    sta _main__local_61+1
    .loc "src/test-resources/test_cast.c", 48
    lda _main__local_61
    ldx #0
    lda #30
    ldx #0
    sta __zp_scratch2
    stx __zp_scratch2+1
    mul.16 .AX, __zp_scratch2
    sta _main__local_63
    stx _main__local_63+1
    .loc "src/test-resources/test_cast.c", 49
    lda _main__local_63
    ldx _main__local_63+1
    cmp.16 .AX, #300
    bne @if_then27
    bra @if_end29
@if_then27:
    lda #10
    ldx #0
    bra @__return
@if_end29:
    .loc "src/test-resources/test_cast.c", 51
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 36
    endproc


__zp_save_buf:

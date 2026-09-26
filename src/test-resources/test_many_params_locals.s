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

    .global _add4
    .global _add5
    .global _weighted_sum
    .global _many_locals
    .global _param_after_locals
    .global _mixed_char_int
    .global _call_chain
    .global _deeply_nested
    .global _main

    .segment "code"

; function _add4
; SAC inline storage: 8 bytes
    .global _add4__param_a
    _add4__param_a: .word 0
    .global _add4__param_b
    _add4__param_b: .word 0
    .global _add4__param_c
    _add4__param_c: .word 0
    .global _add4__param_d
    _add4__param_d: .word 0
    _add4__local_0: .word 0
    _add4__local_1: .word 0
    _add4__local_2: .word 0
    _add4__local_3: .word 0
    proc _add4, W#@_p_a, W#@_p_b, W#@_p_c, W#@_p_d
    .sac
    .var _fp = 0
    .loc "test_many_params_locals.c", 4
    .var @_p_a = 2
    .var @_p_b = 4
    .var @_p_c = 6
    .var @_p_d = 8
; .debug_var: __add4 @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __add4 @_p_b offset=4 size=2 type=int16 scope=parameter
; .debug_var: __add4 @_p_c offset=6 size=2 type=int16 scope=parameter
; .debug_var: __add4 @_p_d offset=8 size=2 type=int16 scope=parameter

@entry:
    .loc "test_many_params_locals.c", 5
    lda _add4__param_b
    ldx _add4__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _add4__param_a
    ldx _add4__param_a+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda _add4__param_c
    ldx _add4__param_c+1
    add.16 .AX, $20
    sta $22
    stx $23
    lda _add4__param_d
    ldx _add4__param_d+1
    add.16 .AX, $22
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 8
    endproc

; function _add5
; SAC inline storage: 10 bytes
    .global _add5__param_a
    _add5__param_a: .word 0
    .global _add5__param_b
    _add5__param_b: .word 0
    .global _add5__param_c
    _add5__param_c: .word 0
    .global _add5__param_d
    _add5__param_d: .word 0
    .global _add5__param_e
    _add5__param_e: .word 0
    _add5__local_0: .word 0
    _add5__local_1: .word 0
    _add5__local_2: .word 0
    _add5__local_3: .word 0
    _add5__local_4: .word 0
    proc _add5, W#@_p_a, W#@_p_b, W#@_p_c, W#@_p_d, W#@_p_e
    .sac
    .var _fp = 0
    .loc "test_many_params_locals.c", 8
    .var @_p_a = 2
    .var @_p_b = 4
    .var @_p_c = 6
    .var @_p_d = 8
    .var @_p_e = 10
; .debug_var: __add5 @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __add5 @_p_b offset=4 size=2 type=int16 scope=parameter
; .debug_var: __add5 @_p_c offset=6 size=2 type=int16 scope=parameter
; .debug_var: __add5 @_p_d offset=8 size=2 type=int16 scope=parameter
; .debug_var: __add5 @_p_e offset=10 size=2 type=int16 scope=parameter

@entry:
    .loc "test_many_params_locals.c", 9
    lda _add5__param_b
    ldx _add5__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _add5__param_a
    ldx _add5__param_a+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda _add5__param_c
    ldx _add5__param_c+1
    add.16 .AX, $20
    sta $22
    stx $23
    lda _add5__param_d
    ldx _add5__param_d+1
    add.16 .AX, $22
    sta $20
    stx $21
    lda _add5__param_e
    ldx _add5__param_e+1
    add.16 .AX, $20
    sta $22
    stx $23
    lda $22
    ldx $23
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 10
    endproc

; function _weighted_sum
; SAC inline storage: 14 bytes
    .global _weighted_sum__param_a
    _weighted_sum__param_a: .word 0
    .global _weighted_sum__param_b
    _weighted_sum__param_b: .word 0
    .global _weighted_sum__param_c
    _weighted_sum__param_c: .word 0
    _weighted_sum__local_0: .word 0
    _weighted_sum__local_1: .word 0
    _weighted_sum__local_2: .word 0
    _weighted_sum__local_3: .word 0
    _weighted_sum__local_6: .word 0
    _weighted_sum__local_9: .word 0
    _weighted_sum__local_12: .word 0
    proc _weighted_sum, W#@_p_a, W#@_p_b, W#@_p_c
    .sac
    .var _fp = 0
    .loc "test_many_params_locals.c", 12
    .local @_l_total = 12
    .local @_l_w1 = 6
    .local @_l_w2 = 8
    .local @_l_w3 = 10
; .debug_var: __weighted_sum @_l_total offset=12 size=2 type=int16 scope=local
; .debug_var: __weighted_sum @_l_w1 offset=6 size=2 type=int16 scope=local
; .debug_var: __weighted_sum @_l_w2 offset=8 size=2 type=int16 scope=local
; .debug_var: __weighted_sum @_l_w3 offset=10 size=2 type=int16 scope=local
    .var @_p_a = 2
    .var @_p_b = 4
    .var @_p_c = 6
; .debug_var: __weighted_sum @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __weighted_sum @_p_b offset=4 size=2 type=int16 scope=parameter
; .debug_var: __weighted_sum @_p_c offset=6 size=2 type=int16 scope=parameter

@entry:
    .loc "test_many_params_locals.c", 14
    lda #3
    ldx #0
    sta $20
    stx $21
    lda _weighted_sum__param_a
    ldx _weighted_sum__param_a+1
    mul.16 .AX, $20
    sta $22
    stx $23
    lda $22
    ldx $23
    sta _weighted_sum__local_3
    stx _weighted_sum__local_3+1
    .loc "test_many_params_locals.c", 15
    lda #2
    ldx #0
    sta $20
    stx $21
    lda _weighted_sum__param_b
    ldx _weighted_sum__param_b+1
    lsl.16 .AX
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _weighted_sum__local_6
    stx _weighted_sum__local_6+1
    .loc "test_many_params_locals.c", 16
    lda #1
    ldx #0
    sta $20
    stx $21
    lda _weighted_sum__param_c
    ldx _weighted_sum__param_c+1
    sta $20
    stx $21
    lda _weighted_sum__param_c
    ldx _weighted_sum__param_c+1
    sta _weighted_sum__local_9
    stx _weighted_sum__local_9+1
    .loc "test_many_params_locals.c", 17
    lda _weighted_sum__local_6
    ldx _weighted_sum__local_6+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _weighted_sum__local_3
    ldx _weighted_sum__local_3+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda _weighted_sum__local_9
    ldx _weighted_sum__local_9+1
    add.16 .AX, $20
    sta $22
    stx $23
    sta _weighted_sum__local_12
    stx _weighted_sum__local_12+1
    .loc "test_many_params_locals.c", 18
    lda _weighted_sum__local_12
    ldx _weighted_sum__local_12+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 14
    endproc

; function _many_locals
; SAC inline storage: 16 bytes
    .global _many_locals__param_p1
    _many_locals__param_p1: .word 0
    .global _many_locals__param_p2
    _many_locals__param_p2: .word 0
    _many_locals__local_0: .word 0
    _many_locals__local_1: .word 0
    _many_locals__local_2: .word 0
    _many_locals__local_5: .word 0
    _many_locals__local_8: .word 0
    _many_locals__local_10: .word 0
    _many_locals__local_12: .word 0
    _many_locals__local_14: .word 0
    proc _many_locals, W#@_p_p1, W#@_p_p2
    .sac
    .var _fp = 0
    .loc "test_many_params_locals.c", 21
    .local @_l_a = 4
    .local @_l_b = 6
    .local @_l_c = 8
    .local @_l_d = 10
    .local @_l_e = 12
    .local @_l_f = 14
; .debug_var: __many_locals @_l_a offset=4 size=2 type=int16 scope=local
; .debug_var: __many_locals @_l_b offset=6 size=2 type=int16 scope=local
; .debug_var: __many_locals @_l_c offset=8 size=2 type=int16 scope=local
; .debug_var: __many_locals @_l_d offset=10 size=2 type=int16 scope=local
; .debug_var: __many_locals @_l_e offset=12 size=2 type=int16 scope=local
; .debug_var: __many_locals @_l_f offset=14 size=2 type=int16 scope=local
    .var @_p_p1 = 2
    .var @_p_p2 = 4
; .debug_var: __many_locals @_p_p1 offset=2 size=2 type=int16 scope=parameter
; .debug_var: __many_locals @_p_p2 offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_many_params_locals.c", 23
    lda _many_locals__param_p1
    ldx _many_locals__param_p1+1
    add.16 .AX, #1
    sta $22
    stx $23
    sta _many_locals__local_2
    stx _many_locals__local_2+1
    .loc "test_many_params_locals.c", 24
    lda _many_locals__param_p2
    ldx _many_locals__param_p2+1
    add.16 .AX, #2
    sta $22
    stx $23
    sta _many_locals__local_5
    stx _many_locals__local_5+1
    .loc "test_many_params_locals.c", 25
    lda _many_locals__local_5
    ldx _many_locals__local_5+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _many_locals__local_2
    ldx _many_locals__local_2+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    sta _many_locals__local_8
    stx _many_locals__local_8+1
    .loc "test_many_params_locals.c", 26
    lda _many_locals__param_p1
    ldx _many_locals__param_p1+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _many_locals__local_8
    ldx _many_locals__local_8+1
    sub.16 .AX, __zp_scratch2
    sta $20
    stx $21
    sta _many_locals__local_10
    stx _many_locals__local_10+1
    .loc "test_many_params_locals.c", 27
    lda _many_locals__param_p2
    ldx _many_locals__param_p2+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _many_locals__local_10
    ldx _many_locals__local_10+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    sta _many_locals__local_12
    stx _many_locals__local_12+1
    .loc "test_many_params_locals.c", 28
    lda _many_locals__local_12
    ldx _many_locals__local_12+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _many_locals__local_2
    ldx _many_locals__local_2+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    sta _many_locals__local_14
    stx _many_locals__local_14+1
    .loc "test_many_params_locals.c", 29
    lda _many_locals__local_14
    ldx _many_locals__local_14+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 16
    endproc

; function _param_after_locals
; SAC inline storage: 12 bytes
    .global _param_after_locals__param_x
    _param_after_locals__param_x: .word 0
    .global _param_after_locals__param_y
    _param_after_locals__param_y: .word 0
    .global _param_after_locals__param_z
    _param_after_locals__param_z: .word 0
    _param_after_locals__local_0: .word 0
    _param_after_locals__local_1: .word 0
    _param_after_locals__local_2: .word 0
    _param_after_locals__local_3: .word 0
    _param_after_locals__local_5: .word 0
    _param_after_locals__local_7: .word 0
    proc _param_after_locals, W#@_p_x, W#@_p_y, W#@_p_z
    .sac
    .var _fp = 0
    .loc "test_many_params_locals.c", 32
    .local @_l_tmp1 = 6
    .local @_l_tmp2 = 8
    .local @_l_tmp3 = 10
; .debug_var: __param_after_locals @_l_tmp1 offset=6 size=2 type=int16 scope=local
; .debug_var: __param_after_locals @_l_tmp2 offset=8 size=2 type=int16 scope=local
; .debug_var: __param_after_locals @_l_tmp3 offset=10 size=2 type=int16 scope=local
    .var @_p_x = 2
    .var @_p_y = 4
    .var @_p_z = 6
; .debug_var: __param_after_locals @_p_x offset=2 size=2 type=int16 scope=parameter
; .debug_var: __param_after_locals @_p_y offset=4 size=2 type=int16 scope=parameter
; .debug_var: __param_after_locals @_p_z offset=6 size=2 type=int16 scope=parameter

@entry:
    .loc "test_many_params_locals.c", 34
    lda #0
    sta _param_after_locals__local_3
    sta _param_after_locals__local_3+1
    .loc "test_many_params_locals.c", 35
    lda #0
    sta _param_after_locals__local_5
    sta _param_after_locals__local_5+1
    .loc "test_many_params_locals.c", 36
    lda #0
    sta _param_after_locals__local_7
    sta _param_after_locals__local_7+1
    .loc "test_many_params_locals.c", 37
    lda _param_after_locals__param_x
    ldx _param_after_locals__param_x+1
    add.16 .AX, #1
    sta $22
    stx $23
    sta _param_after_locals__local_3
    stx _param_after_locals__local_3+1
    .loc "test_many_params_locals.c", 38
    lda _param_after_locals__param_y
    ldx _param_after_locals__param_y+1
    add.16 .AX, #2
    sta $22
    stx $23
    sta _param_after_locals__local_5
    stx _param_after_locals__local_5+1
    .loc "test_many_params_locals.c", 39
    lda _param_after_locals__param_z
    ldx _param_after_locals__param_z+1
    add.16 .AX, #3
    sta $22
    stx $23
    sta _param_after_locals__local_7
    stx _param_after_locals__local_7+1
    .loc "test_many_params_locals.c", 40
    lda _param_after_locals__local_5
    ldx _param_after_locals__local_5+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _param_after_locals__local_3
    ldx _param_after_locals__local_3+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda _param_after_locals__local_7
    ldx _param_after_locals__local_7+1
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
    .frame_size 12
    endproc

; function _mixed_char_int
; SAC inline storage: 10 bytes
    .global _mixed_char_int__param_a
    _mixed_char_int__param_a: .word 0
    .global _mixed_char_int__param_b
    _mixed_char_int__param_b: .word 0
    .global _mixed_char_int__param_c
    _mixed_char_int__param_c: .word 0
    .global _mixed_char_int__param_d
    _mixed_char_int__param_d: .word 0
    _mixed_char_int__local_0: .word 0
    _mixed_char_int__local_1: .word 0
    _mixed_char_int__local_2: .word 0
    _mixed_char_int__local_3: .word 0
    _mixed_char_int__local_4: .word 0
    proc _mixed_char_int, W#@_p_a, B#@_p_b, W#@_p_c, B#@_p_d
    .sac
    .var _fp = 0
    .loc "test_many_params_locals.c", 43
    .local @_l_r = 8
; .debug_var: __mixed_char_int @_l_r offset=8 size=2 type=int16 scope=local
    .var @_p_a = 2
    .var @_p_b = 4
    .var @_p_c = 6
    .var @_p_d = 8
; .debug_var: __mixed_char_int @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __mixed_char_int @_p_b offset=4 size=2 type=int8 scope=parameter
; .debug_var: __mixed_char_int @_p_c offset=6 size=2 type=int16 scope=parameter
; .debug_var: __mixed_char_int @_p_d offset=8 size=2 type=int8 scope=parameter

@entry:
    lda _mixed_char_int__param_b
    ldx #0
    sta $20
    stx $21
    .loc "test_many_params_locals.c", 44
    lda _mixed_char_int__param_a
    ldx _mixed_char_int__param_a+1
    add.16 .AX, $20
    sta $22
    stx $23
    lda _mixed_char_int__param_c
    ldx _mixed_char_int__param_c+1
    add.16 .AX, $22
    sta $20
    stx $21
    lda _mixed_char_int__param_d
    ldx #0
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
    sta _mixed_char_int__local_4
    stx _mixed_char_int__local_4+1
    .loc "test_many_params_locals.c", 45
    lda _mixed_char_int__local_4
    ldx _mixed_char_int__local_4+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 10
    endproc

; function _call_chain
; SAC inline storage: 10 bytes
    .global _call_chain__param_a
    _call_chain__param_a: .word 0
    .global _call_chain__param_b
    _call_chain__param_b: .word 0
    .global _call_chain__param_c
    _call_chain__param_c: .word 0
    _call_chain__local_0: .word 0
    _call_chain__local_1: .word 0
    _call_chain__local_2: .word 0
    _call_chain__local_3: .word 0
    _call_chain__local_13: .word 0
    proc _call_chain, W#@_p_a, W#@_p_b, W#@_p_c
    .sac
    .var _fp = 0
    .loc "test_many_params_locals.c", 48
    .local @_l_partial = 6
    .local @_l_result = 8
; .debug_var: __call_chain @_l_partial offset=6 size=2 type=int16 scope=local
; .debug_var: __call_chain @_l_result offset=8 size=2 type=int16 scope=local
    .var @_p_a = 2
    .var @_p_b = 4
    .var @_p_c = 6
; .debug_var: __call_chain @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __call_chain @_p_b offset=4 size=2 type=int16 scope=parameter
; .debug_var: __call_chain @_p_c offset=6 size=2 type=int16 scope=parameter

@entry:
    .loc "test_many_params_locals.c", 50
    lda #10
    ldx #0
    sta $20
    stx $21
    lda _call_chain__param_a
    ldx _call_chain__param_a+1
    sta $22
    stx $23
    lda _call_chain__param_b
    ldx _call_chain__param_b+1
    sta $22
    stx $23
    lda _call_chain__param_c
    ldx _call_chain__param_c+1
    sta $22
    stx $23
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_many_params_locals.c", 5
    lda _call_chain__param_b
    ldx _call_chain__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _call_chain__param_a
    ldx _call_chain__param_a+1
    add.16 .AX, __zp_scratch2
    sta $22
    stx $23
    lda _call_chain__param_c
    ldx _call_chain__param_c+1
    add.16 .AX, $22
    sta $24
    stx $25
    lda $24
    clc
    adc #10
    sta $22
    lda $25
    adc #0
    sta $23
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end0:
    .loc "test_many_params_locals.c", 50
    lda $20
    ldx $21
    sta _call_chain__local_3
    stx _call_chain__local_3+1
    .loc "test_many_params_locals.c", 51
    lda _call_chain__local_3
    ldx _call_chain__local_3+1
    sta $20
    stx $21
    lda _call_chain__param_a
    ldx _call_chain__param_a+1
    sta $20
    stx $21
    lda _call_chain__param_b
    ldx _call_chain__param_b+1
    sta $20
    stx $21
    lda _call_chain__param_c
    ldx _call_chain__param_c+1
    sta $20
    stx $21
    .loc "test_many_params_locals.c", 5
    lda _call_chain__param_a
    ldx _call_chain__param_a+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _call_chain__local_3
    ldx _call_chain__local_3+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda _call_chain__param_b
    ldx _call_chain__param_b+1
    add.16 .AX, $20
    sta $22
    stx $23
    lda _call_chain__param_c
    ldx _call_chain__param_c+1
    add.16 .AX, $22
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end2:
    .loc "test_many_params_locals.c", 51
    lda $22
    ldx $23
    sta _call_chain__local_13
    stx _call_chain__local_13+1
    .loc "test_many_params_locals.c", 52
    lda _call_chain__local_13
    ldx _call_chain__local_13+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 10
    endproc

; function _deeply_nested
; SAC inline storage: 18 bytes
    .global _deeply_nested__param_a
    _deeply_nested__param_a: .word 0
    .global _deeply_nested__param_b
    _deeply_nested__param_b: .word 0
    .global _deeply_nested__param_c
    _deeply_nested__param_c: .word 0
    .global _deeply_nested__param_d
    _deeply_nested__param_d: .word 0
    _deeply_nested__local_0: .word 0
    _deeply_nested__local_1: .word 0
    _deeply_nested__local_2: .word 0
    _deeply_nested__local_3: .word 0
    _deeply_nested__local_4: .word 0
    _deeply_nested__local_6: .word 0
    _deeply_nested__local_8: .word 0
    _deeply_nested__local_10: .word 0
    _deeply_nested__local_12: .word 0
    proc _deeply_nested, W#@_p_a, W#@_p_b, W#@_p_c, W#@_p_d
    .sac
    .var _fp = 0
    .loc "test_many_params_locals.c", 55
    .local @_l_v = 16
    .local @_l_w = 14
    .local @_l_x = 8
    .local @_l_y = 10
    .local @_l_z = 12
; .debug_var: __deeply_nested @_l_v offset=16 size=2 type=int16 scope=local
; .debug_var: __deeply_nested @_l_w offset=14 size=2 type=int16 scope=local
; .debug_var: __deeply_nested @_l_x offset=8 size=2 type=int16 scope=local
; .debug_var: __deeply_nested @_l_y offset=10 size=2 type=int16 scope=local
; .debug_var: __deeply_nested @_l_z offset=12 size=2 type=int16 scope=local
    .var @_p_a = 2
    .var @_p_b = 4
    .var @_p_c = 6
    .var @_p_d = 8
; .debug_var: __deeply_nested @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __deeply_nested @_p_b offset=4 size=2 type=int16 scope=parameter
; .debug_var: __deeply_nested @_p_c offset=6 size=2 type=int16 scope=parameter
; .debug_var: __deeply_nested @_p_d offset=8 size=2 type=int16 scope=parameter

@entry:
    .loc "test_many_params_locals.c", 57
    lda _deeply_nested__param_b
    ldx _deeply_nested__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _deeply_nested__param_a
    ldx _deeply_nested__param_a+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    sta _deeply_nested__local_4
    stx _deeply_nested__local_4+1
    .loc "test_many_params_locals.c", 58
    lda _deeply_nested__param_d
    ldx _deeply_nested__param_d+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _deeply_nested__param_c
    ldx _deeply_nested__param_c+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    sta _deeply_nested__local_6
    stx _deeply_nested__local_6+1
    .loc "test_many_params_locals.c", 59
    lda _deeply_nested__local_6
    ldx _deeply_nested__local_6+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _deeply_nested__local_4
    ldx _deeply_nested__local_4+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    sta _deeply_nested__local_8
    stx _deeply_nested__local_8+1
    .loc "test_many_params_locals.c", 60
    lda _deeply_nested__param_a
    ldx _deeply_nested__param_a+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _deeply_nested__local_8
    ldx _deeply_nested__local_8+1
    sub.16 .AX, __zp_scratch2
    sta $20
    stx $21
    sta _deeply_nested__local_10
    stx _deeply_nested__local_10+1
    .loc "test_many_params_locals.c", 61
    lda _deeply_nested__param_d
    ldx _deeply_nested__param_d+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _deeply_nested__local_10
    ldx _deeply_nested__local_10+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    sta _deeply_nested__local_12
    stx _deeply_nested__local_12+1
    .loc "test_many_params_locals.c", 62
    lda _deeply_nested__local_12
    ldx _deeply_nested__local_12+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 18
    endproc

; function _main
; SAC inline storage: 70 bytes
    _main__local_0: .word 0
    _main__local_15: .word 0
    _main__local_33: .word 0
    _main__local_41: .word 0
    _main__local_44: .word 0
    _main__local_47: .word 0
    _main__local_50: .word 0
    _main__local_55: .word 0
    _main__local_71: .word 0
    _main__local_73: .word 0
    _main__local_77: .word 0
    _main__local_85: .word 0
    _main__local_87: .word 0
    _main__local_89: .word 0
    _main__local_101: .word 0
    _main__local_113: .word 0
    _main__local_121: .word 0
    _main__local_129: .word 0
    _main__local_139: .word 0
    _main__local_150: .word 0
    _main__local_160: .word 0
    _main__local_162: .word 0
    _main__local_164: .word 0
    _main__local_166: .word 0
    _main__local_168: .word 0
    _main__local_172: .word 0
    _main__local_174: .word 0
    _main__local_176: .word 0
    _main__local_178: .word 0
    _main__local_180: .word 0
    _main__local_191: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_many_params_locals.c", 65
    .local @_l_a = 58
    .local @_l_b = 60
    .local @_l_c = 62
    .local @_l_d = 64
    .local @_l_e = 24
    .local @_l_f = 26
    .local @_l_partial = 42
    .local @_l_r = 38
    .local @_l_r1 = 0
    .local @_l_r10 = 68
    .local @_l_r2 = 2
    .local @_l_r3 = 4
    .local @_l_r4 = 14
    .local @_l_r5 = 28
    .local @_l_r6 = 36
    .local @_l_r7 = 40
    .local @_l_r8 = 46
    .local @_l_r9 = 66
    .local @_l_result = 44
    .local @_l_tmp1 = 30
    .local @_l_tmp2 = 32
    .local @_l_tmp3 = 34
    .local @_l_total = 12
    .local @_l_v = 56
    .local @_l_w = 54
    .local @_l_w1 = 6
    .local @_l_w2 = 8
    .local @_l_w3 = 10
    .local @_l_x = 48
    .local @_l_y = 50
    .local @_l_z = 52
; .debug_var: __main @_l_a offset=58 size=2 type=int16 scope=local
; .debug_var: __main @_l_b offset=60 size=2 type=int16 scope=local
; .debug_var: __main @_l_c offset=62 size=2 type=int16 scope=local
; .debug_var: __main @_l_d offset=64 size=2 type=int16 scope=local
; .debug_var: __main @_l_e offset=24 size=2 type=int16 scope=local
; .debug_var: __main @_l_f offset=26 size=2 type=int16 scope=local
; .debug_var: __main @_l_partial offset=42 size=2 type=int16 scope=local
; .debug_var: __main @_l_r offset=38 size=2 type=int16 scope=local
; .debug_var: __main @_l_r1 offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_r10 offset=68 size=2 type=int16 scope=local
; .debug_var: __main @_l_r2 offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_r3 offset=4 size=2 type=int16 scope=local
; .debug_var: __main @_l_r4 offset=14 size=2 type=int16 scope=local
; .debug_var: __main @_l_r5 offset=28 size=2 type=int16 scope=local
; .debug_var: __main @_l_r6 offset=36 size=2 type=int16 scope=local
; .debug_var: __main @_l_r7 offset=40 size=2 type=int16 scope=local
; .debug_var: __main @_l_r8 offset=46 size=2 type=int16 scope=local
; .debug_var: __main @_l_r9 offset=66 size=2 type=int16 scope=local
; .debug_var: __main @_l_result offset=44 size=2 type=int16 scope=local
; .debug_var: __main @_l_tmp1 offset=30 size=2 type=int16 scope=local
; .debug_var: __main @_l_tmp2 offset=32 size=2 type=int16 scope=local
; .debug_var: __main @_l_tmp3 offset=34 size=2 type=int16 scope=local
; .debug_var: __main @_l_total offset=12 size=2 type=int16 scope=local
; .debug_var: __main @_l_v offset=56 size=2 type=int16 scope=local
; .debug_var: __main @_l_w offset=54 size=2 type=int16 scope=local
; .debug_var: __main @_l_w1 offset=6 size=2 type=int16 scope=local
; .debug_var: __main @_l_w2 offset=8 size=2 type=int16 scope=local
; .debug_var: __main @_l_w3 offset=10 size=2 type=int16 scope=local
; .debug_var: __main @_l_x offset=48 size=2 type=int16 scope=local
; .debug_var: __main @_l_y offset=50 size=2 type=int16 scope=local
; .debug_var: __main @_l_z offset=52 size=2 type=int16 scope=local

@entry:
    .loc "test_many_params_locals.c", 67
    lda #10
    ldx #0
    sta $20
    stx $21
    lda #20
    ldx #0
    sta $22
    stx $23
    lda #30
    ldx #0
    sta $24
    stx $25
    lda #40
    ldx #0
    sta $26
    stx $27
    lda $20
    ldx $21
    sta $28
    stx $29
    lda $22
    ldx $23
    sta $28
    stx $29
    lda $24
    ldx $25
    sta $28
    stx $29
    lda $26
    ldx $27
    sta $28
    stx $29
    .loc "test_many_params_locals.c", 5
    lda $20
    clc
    adc #20
    sta $28
    lda $21
    adc #0
    sta $29
    lda $28
    clc
    adc #30
    sta $20
    lda $29
    adc #0
    sta $21
    lda $20
    clc
    adc #40
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end4:
    .loc "test_many_params_locals.c", 67
    lda $20
    ldx $21
    sta _main__local_0
    stx _main__local_0+1
    .loc "test_many_params_locals.c", 68
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, #100
    bne @if_then6
    bra @if_end8
@if_then6:
    lda #1
    ldx #0
    bra @__return
@if_end8:
    .loc "test_many_params_locals.c", 71
    lda #1
    ldx #0
    sta $20
    stx $21
    lda #2
    ldx #0
    sta $22
    stx $23
    lda #3
    ldx #0
    sta $24
    stx $25
    lda #4
    ldx #0
    sta $26
    stx $27
    lda #5
    ldx #0
    sta $28
    stx $29
    lda $20
    ldx $21
    sta $2A
    stx $2B
    lda $22
    ldx $23
    sta $2A
    stx $2B
    lda $24
    ldx $25
    sta $2A
    stx $2B
    lda $26
    ldx $27
    sta $2A
    stx $2B
    lda $28
    ldx $29
    sta $2A
    stx $2B
    .loc "test_many_params_locals.c", 9
    lda $20
    clc
    adc #2
    sta $2A
    lda $21
    adc #0
    sta $2B
    lda $2A
    clc
    adc #3
    sta $20
    lda $2B
    adc #0
    sta $21
    lda $20
    clc
    adc #4
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    clc
    adc #5
    sta $20
    lda $23
    adc #0
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end9:
    .loc "test_many_params_locals.c", 71
    lda $22
    ldx $23
    sta _main__local_15
    stx _main__local_15+1
    .loc "test_many_params_locals.c", 72
    lda _main__local_15
    ldx _main__local_15+1
    cmp.16 .AX, #15
    bne @if_then11
    bra @if_end13
@if_then11:
    lda #2
    ldx #0
    bra @__return
@if_end13:
    .loc "test_many_params_locals.c", 75
    lda #10
    ldx #0
    sta $20
    stx $21
    lda #20
    ldx #0
    sta $22
    stx $23
    lda #30
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx $21
    sta $26
    stx $27
    lda $22
    ldx $23
    sta $26
    stx $27
    lda $24
    ldx $25
    sta $26
    stx $27
    .loc "test_many_params_locals.c", 14
    lda #3
    ldx #0
    sta $26
    stx $27
    lda $20
    ldx $21
    mul.16 .AX, $26
    sta $28
    stx $29
    lda $28
    ldx $29
    sta _main__local_41
    stx _main__local_41+1
    .loc "test_many_params_locals.c", 15
    lda #2
    ldx #0
    sta $20
    stx $21
    lda $22
    ldx $23
    lsl.16 .AX
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_44
    stx _main__local_44+1
    .loc "test_many_params_locals.c", 16
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $24
    ldx $25
    sta $20
    stx $21
    lda $24
    ldx $25
    sta _main__local_47
    stx _main__local_47+1
    .loc "test_many_params_locals.c", 17
    lda _main__local_44
    ldx _main__local_44+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_41
    ldx _main__local_41+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda _main__local_47
    ldx _main__local_47+1
    add.16 .AX, $20
    sta $22
    stx $23
    sta _main__local_50
    stx _main__local_50+1
    .loc "test_many_params_locals.c", 18
    lda _main__local_50
    ldx _main__local_50+1
    sta $20
    stx $21
@inline_end14:
    .loc "test_many_params_locals.c", 75
    lda $20
    ldx $21
    sta _main__local_33
    stx _main__local_33+1
    .loc "test_many_params_locals.c", 76
    lda _main__local_33
    ldx _main__local_33+1
    cmp.16 .AX, #100
    bne @if_then16
    bra @if_end18
@if_then16:
    lda #3
    ldx #0
    bra @__return
@if_end18:
    .loc "test_many_params_locals.c", 79
    lda #10
    ldx #0
    sta $20
    stx $21
    lda #5
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    sta $24
    stx $25
    lda $22
    ldx $23
    sta $24
    stx $25
    .loc "test_many_params_locals.c", 23
    lda $20
    clc
    adc #1
    sta $24
    lda $21
    adc #0
    sta $25
    .loc "test_many_params_locals.c", 24
    lda $22
    clc
    adc #2
    sta $26
    lda $23
    adc #0
    sta $27
    .loc "test_many_params_locals.c", 25
    lda $24
    clc
    adc $26
    sta $26
    lda $25
    adc $26+1
    sta $27
    .loc "test_many_params_locals.c", 26
    lda $26
    sec
    sbc #10
    sta $20
    lda $27
    sbc #0
    sta $21
    .loc "test_many_params_locals.c", 27
    lda $20
    clc
    adc #5
    sta $26
    lda $21
    adc #0
    sta $27
    lda $26
    ldx $27
    sta _main__local_71
    stx _main__local_71+1
    .loc "test_many_params_locals.c", 28
    lda _main__local_71
    ldx _main__local_71+1
    add.16 .AX, $24
    sta $20
    stx $21
    sta _main__local_73
    stx _main__local_73+1
    .loc "test_many_params_locals.c", 29
    lda _main__local_73
    ldx _main__local_73+1
    sta $20
    stx $21
@inline_end19:
    .loc "test_many_params_locals.c", 79
    lda $20
    ldx $21
    sta _main__local_55
    stx _main__local_55+1
    .loc "test_many_params_locals.c", 80
    lda _main__local_55
    ldx _main__local_55+1
    cmp.16 .AX, #23
    bne @if_then21
    bra @if_end23
@if_then21:
    lda #4
    ldx #0
    bra @__return
@if_end23:
    .loc "test_many_params_locals.c", 83
    lda #10
    ldx #0
    sta $20
    stx $21
    lda #20
    ldx #0
    sta $22
    stx $23
    lda #30
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx $21
    sta $26
    stx $27
    lda $22
    ldx $23
    sta $26
    stx $27
    lda $24
    ldx $25
    sta $26
    stx $27
    .loc "test_many_params_locals.c", 34
    lda #0
    sta _main__local_85
    sta _main__local_85+1
    .loc "test_many_params_locals.c", 35
    lda #0
    sta _main__local_87
    sta _main__local_87+1
    .loc "test_many_params_locals.c", 36
    lda #0
    sta _main__local_89
    sta _main__local_89+1
    .loc "test_many_params_locals.c", 37
    lda $20
    clc
    adc #1
    sta $28
    lda $21
    adc #0
    sta $29
    lda $28
    ldx $29
    sta _main__local_85
    stx _main__local_85+1
    .loc "test_many_params_locals.c", 38
    lda $22
    clc
    adc #2
    sta $26
    lda $23
    adc #0
    sta $27
    lda $26
    ldx $27
    sta _main__local_87
    stx _main__local_87+1
    .loc "test_many_params_locals.c", 39
    lda $24
    clc
    adc #3
    sta $22
    lda $25
    adc #0
    sta $23
    lda $22
    ldx $23
    sta _main__local_89
    stx _main__local_89+1
    .loc "test_many_params_locals.c", 40
    lda _main__local_87
    ldx _main__local_87+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_85
    ldx _main__local_85+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda _main__local_89
    ldx _main__local_89+1
    add.16 .AX, $20
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end24:
    .loc "test_many_params_locals.c", 83
    lda $20
    ldx $21
    sta _main__local_77
    stx _main__local_77+1
    .loc "test_many_params_locals.c", 84
    lda _main__local_77
    ldx _main__local_77+1
    cmp.16 .AX, #66
    bne @if_then26
    bra @if_end28
@if_then26:
    lda #5
    ldx #0
    bra @__return
@if_end28:
    .loc "test_many_params_locals.c", 87
    lda #100
    ldx #0
    sta $20
    stx $21
    lda #10
    ldx #0
    sta $22
    stx $23
    lda #50
    ldx #0
    sta $24
    stx $25
    lda #5
    ldx #0
    sta $26
    stx $27
    lda $22
    ldx $23
    sta $28
    lda $26
    ldx $27
    sta $22
    lda $20
    ldx $21
    sta $26
    stx $27
    lda $28
    ldx #0
    sta $26
    lda $24
    ldx $25
    sta $26
    stx $27
    lda $22
    ldx #0
    sta $26
    lda $28
    ldx #0
    ldx #0
    sta $26
    stx $27
    .loc "test_many_params_locals.c", 44
    lda $20
    clc
    adc $26
    sta $28
    lda $21
    adc $26+1
    sta $29
    lda $28
    clc
    adc #50
    sta $20
    lda $29
    adc #0
    sta $21
    lda $22
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $20
    clc
    adc $24
    sta $22
    lda $21
    adc $24+1
    sta $23
    lda $22
    ldx $23
    sta _main__local_113
    stx _main__local_113+1
    .loc "test_many_params_locals.c", 45
    lda _main__local_113
    ldx _main__local_113+1
    sta $20
    stx $21
@inline_end29:
    .loc "test_many_params_locals.c", 87
    lda $20
    ldx $21
    sta _main__local_101
    stx _main__local_101+1
    .loc "test_many_params_locals.c", 88
    lda _main__local_101
    ldx _main__local_101+1
    cmp.16 .AX, #165
    bne @if_then31
    bra @if_end33
@if_then31:
    lda #6
    ldx #0
    bra @__return
@if_end33:
    .loc "test_many_params_locals.c", 91
    lda #1
    ldx #0
    sta $20
    stx $21
    lda #2
    ldx #0
    sta $22
    stx $23
    lda #3
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx $21
    sta $26
    stx $27
    lda $22
    ldx $23
    sta $28
    stx $29
    lda $24
    ldx $25
    sta $2A
    stx $2B
    .loc "test_many_params_locals.c", 50
    lda #10
    ldx #0
    sta $2C
    stx $2D
    lda $20
    ldx $21
    sta $2E
    stx $2F
    lda $22
    ldx $23
    sta $2E
    stx $2F
    lda $24
    ldx $25
    sta $2E
    stx $2F
    lda $2C
    ldx $2D
    sta $2E
    stx $2F
    .loc "test_many_params_locals.c", 5
    lda $20
    clc
    adc #2
    sta $2E
    lda $21
    adc #0
    sta $2F
    lda $2E
    clc
    adc #3
    sta $20
    lda $2F
    adc #0
    sta $21
    lda $20
    clc
    adc #10
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end35:
    .loc "test_many_params_locals.c", 50
    lda $20
    ldx $21
    sta _main__local_129
    stx _main__local_129+1
    .loc "test_many_params_locals.c", 51
    lda _main__local_129
    ldx _main__local_129+1
    sta $20
    stx $21
    lda $26
    ldx $27
    sta $20
    stx $21
    lda $28
    ldx $29
    sta $20
    stx $21
    lda $2A
    ldx $2B
    sta $20
    stx $21
    .loc "test_many_params_locals.c", 5
    lda _main__local_129
    ldx _main__local_129+1
    add.16 .AX, $26
    sta $20
    stx $21
    lda $20
    clc
    adc $28
    sta $22
    lda $21
    adc $28+1
    sta $23
    lda $22
    clc
    adc $2A
    sta $20
    lda $23
    adc $2A+1
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end37:
    .loc "test_many_params_locals.c", 51
    lda $22
    ldx $23
    sta _main__local_139
    stx _main__local_139+1
    .loc "test_many_params_locals.c", 52
    lda _main__local_139
    ldx _main__local_139+1
    sta $20
    stx $21
@inline_end34:
    .loc "test_many_params_locals.c", 91
    lda $20
    ldx $21
    sta _main__local_121
    stx _main__local_121+1
    .loc "test_many_params_locals.c", 92
    lda _main__local_121
    ldx _main__local_121+1
    cmp.16 .AX, #22
    bne @if_then40
    bra @if_end42
@if_then40:
    lda #7
    ldx #0
    bra @__return
@if_end42:
    .loc "test_many_params_locals.c", 95
    lda #10
    ldx #0
    sta $20
    stx $21
    lda #20
    ldx #0
    sta $22
    stx $23
    lda #30
    ldx #0
    sta $24
    stx $25
    lda #40
    ldx #0
    sta $26
    stx $27
    lda $20
    ldx $21
    sta $28
    stx $29
    lda $22
    ldx $23
    sta $28
    stx $29
    lda $24
    ldx $25
    sta $28
    stx $29
    lda $26
    ldx $27
    sta $28
    stx $29
    .loc "test_many_params_locals.c", 57
    lda $20
    clc
    adc #20
    sta $28
    lda $21
    adc #0
    sta $29
    lda $28
    ldx $29
    sta _main__local_160
    stx _main__local_160+1
    .loc "test_many_params_locals.c", 58
    lda $24
    clc
    adc #40
    sta $22
    lda $25
    adc #0
    sta $23
    lda $22
    ldx $23
    sta _main__local_162
    stx _main__local_162+1
    .loc "test_many_params_locals.c", 59
    lda _main__local_162
    ldx _main__local_162+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_160
    ldx _main__local_160+1
    add.16 .AX, __zp_scratch2
    sta $22
    stx $23
    sta _main__local_164
    stx _main__local_164+1
    .loc "test_many_params_locals.c", 60
    lda _main__local_164
    ldx _main__local_164+1
    sub.16 .AX, $20
    sta $22
    stx $23
    sta _main__local_166
    stx _main__local_166+1
    .loc "test_many_params_locals.c", 61
    lda _main__local_166
    ldx _main__local_166+1
    add.16 .AX, $26
    sta $20
    stx $21
    sta _main__local_168
    stx _main__local_168+1
    .loc "test_many_params_locals.c", 62
    lda _main__local_168
    ldx _main__local_168+1
    sta $20
    stx $21
@inline_end43:
    .loc "test_many_params_locals.c", 95
    lda $20
    ldx $21
    sta _main__local_150
    stx _main__local_150+1
    .loc "test_many_params_locals.c", 96
    lda _main__local_150
    ldx _main__local_150+1
    cmp.16 .AX, #90
    bne @if_then45
    bra @if_end47
@if_then45:
    lda #8
    ldx #0
    bra @__return
@if_end47:
    .loc "test_many_params_locals.c", 99
    lda #5
    sta _main__local_172
    lda #0
    sta _main__local_172+1
    .loc "test_many_params_locals.c", 100
    lda #10
    sta _main__local_174
    lda #0
    sta _main__local_174+1
    .loc "test_many_params_locals.c", 101
    lda #15
    sta _main__local_176
    lda #0
    sta _main__local_176+1
    .loc "test_many_params_locals.c", 102
    lda #20
    sta _main__local_178
    lda #0
    sta _main__local_178+1
    .loc "test_many_params_locals.c", 103
    lda _main__local_172
    ldx _main__local_172+1
    sta $20
    stx $21
    lda _main__local_174
    ldx _main__local_174+1
    sta $20
    stx $21
    lda _main__local_176
    ldx _main__local_176+1
    sta $20
    stx $21
    lda _main__local_178
    ldx _main__local_178+1
    sta $20
    stx $21
    .loc "test_many_params_locals.c", 5
    lda _main__local_174
    ldx _main__local_174+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_172
    ldx _main__local_172+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda _main__local_176
    ldx _main__local_176+1
    add.16 .AX, $20
    sta $22
    stx $23
    lda _main__local_178
    ldx _main__local_178+1
    add.16 .AX, $22
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end48:
    .loc "test_many_params_locals.c", 103
    lda $22
    ldx $23
    sta _main__local_180
    stx _main__local_180+1
    .loc "test_many_params_locals.c", 104
    lda _main__local_180
    ldx _main__local_180+1
    cmp.16 .AX, #50
    bne @if_then50
    bra @if_end52
@if_then50:
    lda #9
    ldx #0
    bra @__return
@if_end52:
    .loc "test_many_params_locals.c", 107
    lda _main__local_174
    ldx _main__local_174+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_172
    ldx _main__local_172+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda _main__local_176
    ldx _main__local_176+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_174
    ldx _main__local_174+1
    add.16 .AX, __zp_scratch2
    sta $22
    stx $23
    lda _main__local_178
    ldx _main__local_178+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_176
    ldx _main__local_176+1
    add.16 .AX, __zp_scratch2
    sta $24
    stx $25
    lda _main__local_172
    ldx _main__local_172+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_178
    ldx _main__local_178+1
    add.16 .AX, __zp_scratch2
    sta $26
    stx $27
    lda #2
    ldx #0
    sta $28
    stx $29
    lda _main__local_172
    ldx _main__local_172+1
    lsl.16 .AX
    sta $28
    stx $29
    lda $20
    ldx $21
    sta $2A
    stx $2B
    lda $22
    ldx $23
    sta $2A
    stx $2B
    lda $24
    ldx $25
    sta $2A
    stx $2B
    lda $26
    ldx $27
    sta $2A
    stx $2B
    lda $28
    ldx $29
    sta $2A
    stx $2B
    .loc "test_many_params_locals.c", 9
    lda $20
    clc
    adc $22
    sta $2A
    lda $21
    adc $22+1
    sta $2B
    lda $2A
    clc
    adc $24
    sta $20
    lda $2B
    adc $24+1
    sta $21
    lda $20
    clc
    adc $26
    sta $22
    lda $21
    adc $26+1
    sta $23
    lda $22
    clc
    adc $28
    sta $20
    lda $23
    adc $28+1
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end53:
    .loc "test_many_params_locals.c", 107
    lda $22
    ldx $23
    sta _main__local_191
    stx _main__local_191+1
    .loc "test_many_params_locals.c", 108
    lda _main__local_191
    ldx _main__local_191+1
    cmp.16 .AX, #110
    bne @if_then55
    bra @if_end57
@if_then55:
    lda #10
    ldx #0
    bra @__return
@if_end57:
    .loc "test_many_params_locals.c", 110
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 70
    endproc


__zp_save_buf:

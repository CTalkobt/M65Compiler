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

    .extern _printf

    .global _global_val
    .global _fib_a
    .global _fib_b
    .global _many_locals
    .global _test_pointer_locals
    .global _sum_array
    .global _complex_expr
    .global _get_global_plus
    .global _return_zero
    .global _level3
    .global _level2
    .global _level1
    .global _deep_nesting_test
    .global _main

    .segment "data"
    .byte 0
_global_val:
; .debug_var: @global _global_val offset=0 size=2 type=int16 scope=global
    .word 42

    .segment "code"

; function _fib_a
; SAC inline storage: 2 bytes
    .global _fib_a__param_n
    _fib_a__param_n: .word 0
    _fib_a__local_0: .word 0
    _fib_a__local_4: .word 0
    proc _fib_a, W#@_p_n
    .sac
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdio.h", 8
    .var @_p_n = 2
; .debug_var: __fib_a @_p_n offset=2 size=2 type=int16 scope=parameter

@entry:
    .loc "test_sac_edge_cases_advanced.c", 13
    lda _fib_a__param_n
    ldx _fib_a__param_n+1
    cmp.16 .AX, #1
    bcc @if_then0
    beq @if_then0
    bra @if_end2
@if_then0:
    lda _fib_a__param_n
    ldx _fib_a__param_n+1
    bra @__return
@if_end2:
    .loc "test_sac_edge_cases_advanced.c", 14
    lda _fib_a__param_n
    ldx _fib_a__param_n+1
    sub.16 .AX, #1
    sta _fib_a__local_4
    stx _fib_a__local_4+1
    lda _fib_a__local_4
    ldx _fib_a__local_4+1
    sta _fib_b__param_0
    stx _fib_b__param_0+1
    jsr _fib_b
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _fib_b
; SAC inline storage: 2 bytes
    .global _fib_b__param_n
    _fib_b__param_n: .word 0
    _fib_b__local_0: .word 0
    _fib_b__local_6: .word 0
    _fib_b__local_12: .word 0
    _fib_b__local_17: .word 0
    proc _fib_b, W#@_p_n
    .sac
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdio.h", 13
    .var @_p_n = 2
; .debug_var: __fib_b @_p_n offset=2 size=2 type=int16 scope=parameter

@entry:
    .loc "test_sac_edge_cases_advanced.c", 18
    lda _fib_b__param_n
    ldx _fib_b__param_n+1
    cmp.16 .AX, #0
    bcc @if_then3
    beq @if_then3
    bra @if_end5
@if_then3:
    lda #0
    ldx #0
    bra @__return
@if_end5:
    .loc "test_sac_edge_cases_advanced.c", 19
    lda _fib_b__param_n
    ldx _fib_b__param_n+1
    sub.16 .AX, #1
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
    .loc "test_sac_edge_cases_advanced.c", 13
    lda $22
    ldx $23
    cmp.16 .AX, #1
    bcc @if_then7
    beq @if_then7
    bra @if_end9
@if_then7:
    lda $20
    ldx $21
    sta _fib_b__local_6
    stx _fib_b__local_6+1
    bra @inline_end6
@if_end9:
    .loc "test_sac_edge_cases_advanced.c", 14
    lda $20
    sec
    sbc #1
    sta $24
    lda $21
    sbc #0
    sta $25
    lda $24
    ldx $25
    sta $20
    stx $21
    .loc "test_sac_edge_cases_advanced.c", 18
    lda $24
    ldx $25
    cmp.16 .AX, #0
    bcc @if_then12
    beq @if_then12
    bra @if_end14
@if_then12:
    lda #0
    sta $22
    sta $23
    lda $22
    ldx $23
    sta _fib_b__local_12
    stx _fib_b__local_12+1
    bra @inline_end11
@if_end14:
    .loc "test_sac_edge_cases_advanced.c", 19
    lda $20
    ldx $21
    sub.16 .AX, #1
    sta _fib_b__local_17
    stx _fib_b__local_17+1
    lda _fib_b__local_17
    ldx _fib_b__local_17+1
    sta _fib_a__param_n
    stx _fib_a__param_n+1
    jsr _fib_a
    sta $20
    stx $21
    lda $20
    clc
    adc #1
    sta $24
    lda $21
    adc #0
    sta $25
    lda $24
    ldx $25
    sta _fib_b__local_12
    stx _fib_b__local_12+1
@inline_end11:
    .loc "test_sac_edge_cases_advanced.c", 14
    lda _fib_b__local_12
    ldx _fib_b__local_12+1
    sta _fib_b__local_6
    stx _fib_b__local_6+1
@inline_end6:
    .loc "test_sac_edge_cases_advanced.c", 19
    lda _fib_b__local_6
    ldx _fib_b__local_6+1
    add.16 .AX, #1
    sta $22
    stx $23
    lda $22
    ldx $23
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _many_locals
; SAC inline storage: 26 bytes
    .global _many_locals__param_a
    _many_locals__param_a: .word 0
    .global _many_locals__param_b
    _many_locals__param_b: .word 0
    .global _many_locals__param_c
    _many_locals__param_c: .word 0
    _many_locals__local_0: .word 0
    _many_locals__local_1: .word 0
    _many_locals__local_2: .word 0
    _many_locals__local_3: .word 0
    _many_locals__local_4: .word 0
    _many_locals__local_5: .word 0
    _many_locals__local_6: .word 0
    _many_locals__local_8: .word 0
    _many_locals__local_10: .word 0
    _many_locals__local_12: .word 0
    _many_locals__local_14: .word 0
    _many_locals__local_16: .word 0
    _many_locals__local_18: .word 0
    proc _many_locals, W#@_p_a, W#@_p_b, W#@_p_c
    .sac
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdio.h", 19
    .local @_l_l1 = 6
    .local @_l_l10 = 24
    .local @_l_l2 = 8
    .local @_l_l3 = 10
    .local @_l_l4 = 12
    .local @_l_l5 = 14
    .local @_l_l6 = 16
    .local @_l_l7 = 18
    .local @_l_l8 = 20
    .local @_l_l9 = 22
; .debug_var: __many_locals @_l_l1 offset=6 size=2 type=int16 scope=local
; .debug_var: __many_locals @_l_l10 offset=24 size=2 type=int16 scope=local
; .debug_var: __many_locals @_l_l2 offset=8 size=2 type=int16 scope=local
; .debug_var: __many_locals @_l_l3 offset=10 size=2 type=int16 scope=local
; .debug_var: __many_locals @_l_l4 offset=12 size=2 type=int16 scope=local
; .debug_var: __many_locals @_l_l5 offset=14 size=2 type=int16 scope=local
; .debug_var: __many_locals @_l_l6 offset=16 size=2 type=int16 scope=local
; .debug_var: __many_locals @_l_l7 offset=18 size=2 type=int16 scope=local
; .debug_var: __many_locals @_l_l8 offset=20 size=2 type=int16 scope=local
; .debug_var: __many_locals @_l_l9 offset=22 size=2 type=int16 scope=local
    .var @_p_a = 2
    .var @_p_b = 4
    .var @_p_c = 6
; .debug_var: __many_locals @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __many_locals @_p_b offset=4 size=2 type=int16 scope=parameter
; .debug_var: __many_locals @_p_c offset=6 size=2 type=int16 scope=parameter

@entry:
    .loc "test_sac_edge_cases_advanced.c", 24
    lda _many_locals__param_a
    ldx _many_locals__param_a+1
    sta _many_locals__local_3
    stx _many_locals__local_3+1
    .loc "test_sac_edge_cases_advanced.c", 25
    lda _many_locals__param_b
    ldx _many_locals__param_b+1
    sta _many_locals__local_4
    stx _many_locals__local_4+1
    .loc "test_sac_edge_cases_advanced.c", 26
    lda _many_locals__param_c
    ldx _many_locals__param_c+1
    sta _many_locals__local_5
    stx _many_locals__local_5+1
    .loc "test_sac_edge_cases_advanced.c", 27
    lda _many_locals__local_4
    ldx _many_locals__local_4+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _many_locals__local_3
    ldx _many_locals__local_3+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    sta _many_locals__local_6
    stx _many_locals__local_6+1
    .loc "test_sac_edge_cases_advanced.c", 28
    lda _many_locals__local_5
    ldx _many_locals__local_5+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _many_locals__local_4
    ldx _many_locals__local_4+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    sta _many_locals__local_8
    stx _many_locals__local_8+1
    .loc "test_sac_edge_cases_advanced.c", 29
    lda _many_locals__local_3
    ldx _many_locals__local_3+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _many_locals__local_5
    ldx _many_locals__local_5+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    sta _many_locals__local_10
    stx _many_locals__local_10+1
    .loc "test_sac_edge_cases_advanced.c", 30
    lda _many_locals__local_8
    ldx _many_locals__local_8+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _many_locals__local_6
    ldx _many_locals__local_6+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    sta _many_locals__local_12
    stx _many_locals__local_12+1
    .loc "test_sac_edge_cases_advanced.c", 31
    lda _many_locals__local_10
    ldx _many_locals__local_10+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _many_locals__local_8
    ldx _many_locals__local_8+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    sta _many_locals__local_14
    stx _many_locals__local_14+1
    .loc "test_sac_edge_cases_advanced.c", 32
    lda _many_locals__local_12
    ldx _many_locals__local_12+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _many_locals__local_10
    ldx _many_locals__local_10+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    sta _many_locals__local_16
    stx _many_locals__local_16+1
    .loc "test_sac_edge_cases_advanced.c", 33
    lda _many_locals__local_14
    ldx _many_locals__local_14+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _many_locals__local_12
    ldx _many_locals__local_12+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    sta _many_locals__local_18
    stx _many_locals__local_18+1
    .loc "test_sac_edge_cases_advanced.c", 34
    lda _many_locals__local_18
    ldx _many_locals__local_18+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _many_locals__local_16
    ldx _many_locals__local_16+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 26
    endproc

; function _test_pointer_locals
; SAC inline storage: 6 bytes
    .global _test_pointer_locals__param_x
    _test_pointer_locals__param_x: .word 0
    _test_pointer_locals__local_0: .word 0
    _test_pointer_locals__local_1: .word 0
    _test_pointer_locals__local_4: .word 0
    proc _test_pointer_locals, W#@_p_x
    .sac
    .var _fp = 0
    .loc "test_sac_edge_cases_advanced.c", 19
    .local @_l_ptr = 4
    .local @_l_val = 2
; .debug_var: __test_pointer_locals @_l_ptr offset=4 size=2 type=ptr scope=local
; .debug_var: __test_pointer_locals @_l_val offset=2 size=2 type=int16 scope=local
    .var @_p_x = 2
; .debug_var: __test_pointer_locals @_p_x offset=2 size=2 type=int16 scope=parameter

@entry:
    .loc "test_sac_edge_cases_advanced.c", 39
    lda #2
    ldx #0
    sta $20
    stx $21
    lda _test_pointer_locals__param_x
    ldx _test_pointer_locals__param_x+1
    lsl.16 .AX
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _test_pointer_locals__local_1
    stx _test_pointer_locals__local_1+1
    .loc "test_sac_edge_cases_advanced.c", 40
    leax.local 2
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _test_pointer_locals__local_4
    stx _test_pointer_locals__local_4+1
    .loc "test_sac_edge_cases_advanced.c", 41
    lda _test_pointer_locals__local_4
    ldx _test_pointer_locals__local_4+1
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    clc
    adc #10
    sta $24
    lda $21
    adc #0
    sta $25
    lda $24
    ldx $25
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _sum_array
; SAC inline storage: 8 bytes
    .global _sum_array__param_arr
    _sum_array__param_arr: .word 0
    .global _sum_array__param_len
    _sum_array__param_len: .word 0
    _sum_array__local_0: .word 0
    _sum_array__local_1: .word 0
    _sum_array__local_2: .word 0
    _sum_array__local_4: .word 0
    proc _sum_array, W#@_p_arr, W#@_p_len
    .sac
    .var _fp = 0
    .loc "test_sac_edge_cases_advanced.c", 26
    .local @_l_i = 6
    .local @_l_sum = 4
; .debug_var: __sum_array @_l_i offset=6 size=2 type=int16 scope=local
; .debug_var: __sum_array @_l_sum offset=4 size=2 type=int16 scope=local
    .var @_p_arr = 2
    .var @_p_len = 4
; .debug_var: __sum_array @_p_arr offset=2 size=2 type=ptr scope=parameter
; .debug_var: __sum_array @_p_len offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_sac_edge_cases_advanced.c", 46
    lda #0
    sta _sum_array__local_2
    sta _sum_array__local_2+1
    .loc "test_sac_edge_cases_advanced.c", 47
    lda #0
    sta _sum_array__local_4
    sta _sum_array__local_4+1
@while_cond18:
    .loc "test_sac_edge_cases_advanced.c", 48
    lda _sum_array__param_len
    ldx _sum_array__param_len+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _sum_array__local_4
    ldx _sum_array__local_4+1
    cmp.16 .AX, __zp_scratch2
    bcc @while_body19
    bra @while_end20
@while_body19:
    .loc "test_sac_edge_cases_advanced.c", 49
    lda _sum_array__param_arr
    ldx _sum_array__param_arr+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _sum_array__local_4
    ldx _sum_array__local_4+1
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda __zp_scratch2
    ldx __zp_scratch2+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $24
    stx $25
    lda _sum_array__local_2
    ldx _sum_array__local_2+1
    add.16 .AX, $24
    sta $26
    stx $27
    sta _sum_array__local_2
    stx _sum_array__local_2+1
    .loc "test_sac_edge_cases_advanced.c", 50
    inc.16f __vr4
    bra @while_cond18
@while_end20:
    .loc "test_sac_edge_cases_advanced.c", 52
    lda _sum_array__local_2
    ldx _sum_array__local_2+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 8
    endproc

; function _complex_expr
; SAC inline storage: 18 bytes
    .global _complex_expr__param_a
    _complex_expr__param_a: .word 0
    .global _complex_expr__param_b
    _complex_expr__param_b: .word 0
    .global _complex_expr__param_c
    _complex_expr__param_c: .word 0
    .global _complex_expr__param_d
    _complex_expr__param_d: .word 0
    _complex_expr__local_0: .word 0
    _complex_expr__local_1: .word 0
    _complex_expr__local_2: .word 0
    _complex_expr__local_3: .word 0
    _complex_expr__local_4: .word 0
    _complex_expr__local_6: .word 0
    _complex_expr__local_8: .word 0
    _complex_expr__local_10: .word 0
    _complex_expr__local_12: .word 0
    proc _complex_expr, W#@_p_a, W#@_p_b, W#@_p_c, W#@_p_d
    .sac
    .var _fp = 0
    .loc "test_sac_edge_cases_advanced.c", 37
    .local @_l_temp1 = 8
    .local @_l_temp2 = 10
    .local @_l_temp3 = 12
    .local @_l_temp4 = 14
    .local @_l_temp5 = 16
; .debug_var: __complex_expr @_l_temp1 offset=8 size=2 type=int16 scope=local
; .debug_var: __complex_expr @_l_temp2 offset=10 size=2 type=int16 scope=local
; .debug_var: __complex_expr @_l_temp3 offset=12 size=2 type=int16 scope=local
; .debug_var: __complex_expr @_l_temp4 offset=14 size=2 type=int16 scope=local
; .debug_var: __complex_expr @_l_temp5 offset=16 size=2 type=int16 scope=local
    .var @_p_a = 2
    .var @_p_b = 4
    .var @_p_c = 6
    .var @_p_d = 8
; .debug_var: __complex_expr @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __complex_expr @_p_b offset=4 size=2 type=int16 scope=parameter
; .debug_var: __complex_expr @_p_c offset=6 size=2 type=int16 scope=parameter
; .debug_var: __complex_expr @_p_d offset=8 size=2 type=int16 scope=parameter

@entry:
    .loc "test_sac_edge_cases_advanced.c", 57
    lda _complex_expr__param_b
    ldx _complex_expr__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _complex_expr__param_a
    ldx _complex_expr__param_a+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    sta _complex_expr__local_4
    stx _complex_expr__local_4+1
    .loc "test_sac_edge_cases_advanced.c", 58
    lda _complex_expr__param_d
    ldx _complex_expr__param_d+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _complex_expr__param_c
    ldx _complex_expr__param_c+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    sta _complex_expr__local_6
    stx _complex_expr__local_6+1
    .loc "test_sac_edge_cases_advanced.c", 59
    lda _complex_expr__local_6
    ldx _complex_expr__local_6+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _complex_expr__local_4
    ldx _complex_expr__local_4+1
    mul.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _complex_expr__local_8
    stx _complex_expr__local_8+1
    .loc "test_sac_edge_cases_advanced.c", 60
    lda _complex_expr__param_a
    ldx _complex_expr__param_a+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _complex_expr__local_8
    ldx _complex_expr__local_8+1
    sub.16 .AX, __zp_scratch2
    sta $20
    stx $21
    sta _complex_expr__local_10
    stx _complex_expr__local_10+1
    .loc "test_sac_edge_cases_advanced.c", 61
    lda #2
    ldx #0
    sta $20
    stx $21
    lda _complex_expr__local_10
    ldx _complex_expr__local_10+1
    lsr.16 .AX
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _complex_expr__local_12
    stx _complex_expr__local_12+1
    .loc "test_sac_edge_cases_advanced.c", 62
    lda _complex_expr__param_c
    ldx _complex_expr__param_c+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _complex_expr__local_12
    ldx _complex_expr__local_12+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 18
    endproc

; function _get_global_plus
; SAC inline storage: 2 bytes
    .global _get_global_plus__param_x
    _get_global_plus__param_x: .word 0
    _get_global_plus__local_0: .word 0
    proc _get_global_plus, W#@_p_x
    .sac
    .var _fp = 0
    .loc "test_sac_edge_cases_advanced.c", 49
    .var @_p_x = 2
; .debug_var: __get_global_plus @_p_x offset=2 size=2 type=int16 scope=parameter

@entry:
    .loc "test_sac_edge_cases_advanced.c", 69
    lda _global_val
    ldx _global_val+1
    sta $20
    stx $21
    lda _get_global_plus__param_x
    ldx _get_global_plus__param_x+1
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
    .frame_size 2
    endproc

; function _return_zero
; SAC zero-alloc leaf: no storage overhead
    proc _return_zero
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_sac_edge_cases_advanced.c", 54

@entry:
    .loc "test_sac_edge_cases_advanced.c", 74
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, zeroalloc, leaf
    .reg_clobbers A, X
    .flag_clobbers N, Z
    .frame_size 0
    endproc

; function _level3
; SAC inline storage: 2 bytes
    .global _level3__param_x
    _level3__param_x: .word 0
    _level3__local_0: .word 0
    proc _level3, W#@_p_x
    .sac
    .var _fp = 0
    .loc "test_sac_edge_cases_advanced.c", 59
    .var @_p_x = 2
; .debug_var: __level3 @_p_x offset=2 size=2 type=int16 scope=parameter

@entry:
    .loc "test_sac_edge_cases_advanced.c", 79
    lda #2
    ldx #0
    sta $20
    stx $21
    lda _level3__param_x
    ldx _level3__param_x+1
    lsl.16 .AX
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z
    .frame_size 2
    endproc

; function _level2
; SAC inline storage: 2 bytes
    .global _level2__param_x
    _level2__param_x: .word 0
    _level2__local_0: .word 0
    proc _level2, W#@_p_x
    .sac
    .var _fp = 0
    .loc "test_sac_edge_cases_advanced.c", 63
    .var @_p_x = 2
; .debug_var: __level2 @_p_x offset=2 size=2 type=int16 scope=parameter

@entry:
    .loc "test_sac_edge_cases_advanced.c", 83
    lda _level2__param_x
    ldx _level2__param_x+1
    sta $20
    stx $21
    .loc "test_sac_edge_cases_advanced.c", 79
    lda #2
    ldx #0
    sta $20
    stx $21
    lda _level2__param_x
    ldx _level2__param_x+1
    lsl.16 .AX
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end21:
    .loc "test_sac_edge_cases_advanced.c", 83
    lda $22
    clc
    adc #1
    sta $24
    lda $23
    adc #0
    sta $25
    lda $24
    ldx $25
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _level1
; SAC inline storage: 2 bytes
    .global _level1__param_x
    _level1__param_x: .word 0
    _level1__local_0: .word 0
    proc _level1, W#@_p_x
    .sac
    .var _fp = 0
    .loc "test_sac_edge_cases_advanced.c", 67
    .var @_p_x = 2
; .debug_var: __level1 @_p_x offset=2 size=2 type=int16 scope=parameter

@entry:
    .loc "test_sac_edge_cases_advanced.c", 87
    lda _level1__param_x
    ldx _level1__param_x+1
    sta $20
    stx $21
    .loc "test_sac_edge_cases_advanced.c", 83
    lda _level1__param_x
    ldx _level1__param_x+1
    sta $20
    stx $21
    .loc "test_sac_edge_cases_advanced.c", 79
    lda #2
    ldx #0
    sta $20
    stx $21
    lda _level1__param_x
    ldx _level1__param_x+1
    lsl.16 .AX
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end24:
    .loc "test_sac_edge_cases_advanced.c", 83
    lda $22
    clc
    adc #1
    sta $24
    lda $23
    adc #0
    sta $25
    lda $24
    ldx $25
    sta $20
    stx $21
@inline_end23:
    .loc "test_sac_edge_cases_advanced.c", 87
    lda $20
    clc
    adc #2
    sta $24
    lda $21
    adc #0
    sta $25
    lda $24
    ldx $25
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _deep_nesting_test
; SAC zero-alloc leaf: no storage overhead
    proc _deep_nesting_test
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_sac_edge_cases_advanced.c", 71

@entry:
    .loc "test_sac_edge_cases_advanced.c", 91
    lda #5
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_sac_edge_cases_advanced.c", 87
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_sac_edge_cases_advanced.c", 83
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_sac_edge_cases_advanced.c", 79
    lda #2
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    lsl.16 .AX
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end29:
    .loc "test_sac_edge_cases_advanced.c", 83
    lda $20
    clc
    adc #1
    sta $24
    lda $21
    adc #0
    sta $25
    lda $24
    ldx $25
    sta $20
    stx $21
@inline_end28:
    .loc "test_sac_edge_cases_advanced.c", 87
    lda $20
    clc
    adc #2
    sta $24
    lda $21
    adc #0
    sta $25
    lda $24
    ldx $25
    sta $20
    stx $21
@inline_end27:
    .loc "test_sac_edge_cases_advanced.c", 91
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc, zeroalloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 0
    endproc

; function _main
; SAC inline storage: 56 bytes
    _main__local_0: .word 0
    _main__local_8: .word 0
    _main__local_9: .word 0
    _main__local_10: .word 0
    _main__local_11: .word 0
    _main__local_13: .word 0
    _main__local_15: .word 0
    _main__local_17: .word 0
    _main__local_19: .word 0
    _main__local_21: .word 0
    _main__local_23: .word 0
    _main__local_29: .word 0
    _main__local_31: .word 0
    _main__local_43: .word 0
    _main__local_49: .word 0
    _main__local_51: .word 0
    _main__local_63: .word 0
    _main__local_65: .word 0
    _main__local_75: .word 0
    _main__local_77: .word 0
    _main__local_79: .word 0
    _main__local_81: .word 0
    _main__local_83: .word 0
    _main__local_90: .word 0
    _main__local_92: .word 0
    _main__local_101: .word 0
    _main__local_103: .word 0
    _main__local_109: .word 0
    _main__local_111: .word 0
    _main__local_132: .word 0
    _main__local_134: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_sac_edge_cases_advanced.c", 75
    .local @_l_arr = 46
    .local @_l_i = 26
    .local @_l_l1 = 2
    .local @_l_l10 = 20
    .local @_l_l2 = 4
    .local @_l_l3 = 6
    .local @_l_l4 = 8
    .local @_l_l5 = 10
    .local @_l_l6 = 12
    .local @_l_l7 = 14
    .local @_l_l8 = 16
    .local @_l_l9 = 18
    .local @_l_r2 = 0
    .local @_l_r4 = 22
    .local @_l_r5 = 28
    .local @_l_r6 = 40
    .local @_l_r7 = 42
    .local @_l_r8 = 44
    .local @_l_sum = 24
    .local @_l_temp1 = 30
    .local @_l_temp2 = 32
    .local @_l_temp3 = 34
    .local @_l_temp4 = 36
    .local @_l_temp5 = 38
; .debug_var: __main @_l_arr offset=46 size=2 type=int16 scope=local
; .debug_var: __main @_l_i offset=26 size=2 type=int16 scope=local
; .debug_var: __main @_l_l1 offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_l10 offset=20 size=2 type=int16 scope=local
; .debug_var: __main @_l_l2 offset=4 size=2 type=int16 scope=local
; .debug_var: __main @_l_l3 offset=6 size=2 type=int16 scope=local
; .debug_var: __main @_l_l4 offset=8 size=2 type=int16 scope=local
; .debug_var: __main @_l_l5 offset=10 size=2 type=int16 scope=local
; .debug_var: __main @_l_l6 offset=12 size=2 type=int16 scope=local
; .debug_var: __main @_l_l7 offset=14 size=2 type=int16 scope=local
; .debug_var: __main @_l_l8 offset=16 size=2 type=int16 scope=local
; .debug_var: __main @_l_l9 offset=18 size=2 type=int16 scope=local
; .debug_var: __main @_l_r2 offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_r4 offset=22 size=2 type=int16 scope=local
; .debug_var: __main @_l_r5 offset=28 size=2 type=int16 scope=local
; .debug_var: __main @_l_r6 offset=40 size=2 type=int16 scope=local
; .debug_var: __main @_l_r7 offset=42 size=2 type=int16 scope=local
; .debug_var: __main @_l_r8 offset=44 size=2 type=int16 scope=local
; .debug_var: __main @_l_sum offset=24 size=2 type=int16 scope=local
; .debug_var: __main @_l_temp1 offset=30 size=2 type=int16 scope=local
; .debug_var: __main @_l_temp2 offset=32 size=2 type=int16 scope=local
; .debug_var: __main @_l_temp3 offset=34 size=2 type=int16 scope=local
; .debug_var: __main @_l_temp4 offset=36 size=2 type=int16 scope=local
; .debug_var: __main @_l_temp5 offset=38 size=2 type=int16 scope=local

@entry:
    .loc "test_sac_edge_cases_advanced.c", 96
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
    sta $26
    stx $27
    lda $24
    ldx $25
    sta $26
    stx $27
    .loc "test_sac_edge_cases_advanced.c", 24
    lda $20
    ldx $21
    sta _main__local_8
    stx _main__local_8+1
    .loc "test_sac_edge_cases_advanced.c", 25
    lda $22
    ldx $23
    sta _main__local_9
    stx _main__local_9+1
    .loc "test_sac_edge_cases_advanced.c", 26
    lda $24
    ldx $25
    sta _main__local_10
    stx _main__local_10+1
    .loc "test_sac_edge_cases_advanced.c", 27
    lda _main__local_9
    ldx _main__local_9+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_8
    ldx _main__local_8+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    sta _main__local_11
    stx _main__local_11+1
    .loc "test_sac_edge_cases_advanced.c", 28
    lda _main__local_10
    ldx _main__local_10+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_9
    ldx _main__local_9+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    sta _main__local_13
    stx _main__local_13+1
    .loc "test_sac_edge_cases_advanced.c", 29
    lda _main__local_8
    ldx _main__local_8+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_10
    ldx _main__local_10+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    sta _main__local_15
    stx _main__local_15+1
    .loc "test_sac_edge_cases_advanced.c", 30
    lda _main__local_13
    ldx _main__local_13+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_11
    ldx _main__local_11+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    sta _main__local_17
    stx _main__local_17+1
    .loc "test_sac_edge_cases_advanced.c", 31
    lda _main__local_15
    ldx _main__local_15+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_13
    ldx _main__local_13+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    sta _main__local_19
    stx _main__local_19+1
    .loc "test_sac_edge_cases_advanced.c", 32
    lda _main__local_17
    ldx _main__local_17+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_15
    ldx _main__local_15+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    sta _main__local_21
    stx _main__local_21+1
    .loc "test_sac_edge_cases_advanced.c", 33
    lda _main__local_19
    ldx _main__local_19+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_17
    ldx _main__local_17+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    sta _main__local_23
    stx _main__local_23+1
    .loc "test_sac_edge_cases_advanced.c", 34
    lda _main__local_23
    ldx _main__local_23+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_21
    ldx _main__local_21+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end33:
    .loc "test_sac_edge_cases_advanced.c", 96
    lda $20
    ldx $21
    sta _main__local_0
    stx _main__local_0+1
    .loc "test_sac_edge_cases_advanced.c", 98
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, #30
    bne @if_then35
    bra @if_end37
@if_then35:
    .loc "test_sac_edge_cases_advanced.c", 99
    ldax #__str_38
    sta _main__local_29
    stx _main__local_29+1
    lda _main__local_29
    ldx _main__local_29+1
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    jsr _printf
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    .loc "test_sac_edge_cases_advanced.c", 100
    bra @__return
@if_end37:
    .loc "test_sac_edge_cases_advanced.c", 104
    leax.local 46
    sta $20
    stx $21
    lda #1
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    pha
    phx
    struct_elem.16 __zp_scratch, $20, #0
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    lda #2
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    pha
    phx
    struct_elem.16 __zp_scratch, $20, #2
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    lda #3
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    pha
    phx
    struct_elem.16 __zp_scratch, $20, #4
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    lda #4
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    pha
    phx
    struct_elem.16 __zp_scratch, $20, #6
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    lda #5
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    pha
    phx
    struct_elem.16 __zp_scratch, $20, #8
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    .loc "test_sac_edge_cases_advanced.c", 105
    leax.local 46
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
    sta $20
    stx $21
    .loc "test_sac_edge_cases_advanced.c", 46
    lda #0
    sta _main__local_49
    sta _main__local_49+1
    .loc "test_sac_edge_cases_advanced.c", 47
    lda #0
    sta _main__local_51
    sta _main__local_51+1
@while_cond40_ph:
    .loc "test_sac_edge_cases_advanced.c", 49
    lda #36
    ldx #0
    sta $22
    stx $23
@while_cond40:
    .loc "test_sac_edge_cases_advanced.c", 48
    lda _main__local_51
    ldx _main__local_51+1
    cmp.16 .AX, $20
    bcc @while_body41
    bra @while_end42
@while_body41:
    .loc "test_sac_edge_cases_advanced.c", 49
    lda _main__local_51
    ldx _main__local_51+1
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $22
    ldx $22+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $28
    stx $29
    lda _main__local_49
    ldx _main__local_49+1
    add.16 .AX, $28
    sta $2A
    stx $2B
    sta _main__local_49
    stx _main__local_49+1
    .loc "test_sac_edge_cases_advanced.c", 50
    inc.16f __vr51
    bra @while_cond40
@while_end42:
    lda _main__local_49
    ldx _main__local_49+1
    sta $20
    stx $21
    .loc "test_sac_edge_cases_advanced.c", 52
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end39:
    .loc "test_sac_edge_cases_advanced.c", 105
    lda $22
    ldx $23
    sta _main__local_43
    stx _main__local_43+1
    .loc "test_sac_edge_cases_advanced.c", 106
    lda _main__local_43
    ldx _main__local_43+1
    cmp.16 .AX, #15
    bne @if_then44
    bra @if_end46
@if_then44:
    .loc "test_sac_edge_cases_advanced.c", 107
    ldax #__str_47
    sta _main__local_63
    stx _main__local_63+1
    lda _main__local_63
    ldx _main__local_63+1
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    jsr _printf
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    .loc "test_sac_edge_cases_advanced.c", 108
    bra @__return
@if_end46:
    .loc "test_sac_edge_cases_advanced.c", 112
    lda #2
    ldx #0
    sta $20
    stx $21
    lda #3
    ldx #0
    sta $22
    stx $23
    lda #4
    ldx #0
    sta $24
    stx $25
    lda #5
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
    .loc "test_sac_edge_cases_advanced.c", 57
    lda $20
    clc
    adc #3
    sta $28
    lda $21
    adc #0
    sta $29
    lda $28
    ldx $29
    sta _main__local_75
    stx _main__local_75+1
    .loc "test_sac_edge_cases_advanced.c", 58
    lda $24
    clc
    adc #5
    sta $22
    lda $25
    adc #0
    sta $23
    lda $22
    ldx $23
    sta _main__local_77
    stx _main__local_77+1
    .loc "test_sac_edge_cases_advanced.c", 59
    lda _main__local_77
    ldx _main__local_77+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_75
    ldx _main__local_75+1
    mul.16 .AX, __zp_scratch2
    sta $22
    stx $23
    lda $22
    ldx $23
    sta _main__local_79
    stx _main__local_79+1
    .loc "test_sac_edge_cases_advanced.c", 60
    lda _main__local_79
    ldx _main__local_79+1
    sub.16 .AX, $20
    sta $22
    stx $23
    sta _main__local_81
    stx _main__local_81+1
    .loc "test_sac_edge_cases_advanced.c", 61
    lda #2
    ldx #0
    sta $20
    stx $21
    lda _main__local_81
    ldx _main__local_81+1
    lsr.16 .AX
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_83
    stx _main__local_83+1
    .loc "test_sac_edge_cases_advanced.c", 62
    lda _main__local_83
    ldx _main__local_83+1
    add.16 .AX, $24
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end48:
    .loc "test_sac_edge_cases_advanced.c", 112
    lda $20
    ldx $21
    sta _main__local_65
    stx _main__local_65+1
    .loc "test_sac_edge_cases_advanced.c", 114
    lda _main__local_65
    ldx _main__local_65+1
    cmp.16 .AX, #25
    bne @if_then50
    bra @if_end52
@if_then50:
    .loc "test_sac_edge_cases_advanced.c", 115
    ldax #__str_53
    sta _main__local_90
    stx _main__local_90+1
    lda _main__local_90
    ldx _main__local_90+1
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    jsr _printf
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    .loc "test_sac_edge_cases_advanced.c", 116
    bra @__return
@if_end52:
    .loc "test_sac_edge_cases_advanced.c", 120
    lda #8
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_sac_edge_cases_advanced.c", 69
    lda _global_val
    ldx _global_val+1
    sta $22
    stx $23
    lda $22
    clc
    adc #8
    sta $24
    lda $23
    adc #0
    sta $25
    lda $24
    ldx $25
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end54:
    .loc "test_sac_edge_cases_advanced.c", 120
    lda $22
    ldx $23
    sta _main__local_92
    stx _main__local_92+1
    .loc "test_sac_edge_cases_advanced.c", 121
    lda _main__local_92
    ldx _main__local_92+1
    cmp.16 .AX, #50
    bne @if_then56
    bra @if_end58
@if_then56:
    .loc "test_sac_edge_cases_advanced.c", 122
    ldax #__str_59
    sta _main__local_101
    stx _main__local_101+1
    lda _main__local_101
    ldx _main__local_101+1
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    jsr _printf
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    .loc "test_sac_edge_cases_advanced.c", 123
    bra @__return
@if_end58:
    .loc "test_sac_edge_cases_advanced.c", 74
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end60:
    .loc "test_sac_edge_cases_advanced.c", 127
    lda $20
    ldx $21
    sta _main__local_103
    stx _main__local_103+1
    .loc "test_sac_edge_cases_advanced.c", 128
    lda _main__local_103
    ldx _main__local_103+1
    stx __zp_scratch
    ora __zp_scratch
    bne @if_then62
    bra @if_end64
@if_then62:
    .loc "test_sac_edge_cases_advanced.c", 129
    ldax #__str_65
    sta _main__local_109
    stx _main__local_109+1
    lda _main__local_109
    ldx _main__local_109+1
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    jsr _printf
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    .loc "test_sac_edge_cases_advanced.c", 130
    bra @__return
@if_end64:
    .loc "test_sac_edge_cases_advanced.c", 91
    lda #5
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_sac_edge_cases_advanced.c", 87
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_sac_edge_cases_advanced.c", 83
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_sac_edge_cases_advanced.c", 79
    lda #2
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    lsl.16 .AX
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end69:
    .loc "test_sac_edge_cases_advanced.c", 83
    lda $22
    clc
    adc #1
    sta $24
    lda $23
    adc #0
    sta $25
    lda $24
    ldx $25
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end68:
    .loc "test_sac_edge_cases_advanced.c", 87
    lda $22
    clc
    adc #2
    sta $24
    lda $23
    adc #0
    sta $25
    lda $24
    ldx $25
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end67:
    lda $22
    ldx $23
    sta $20
    stx $21
    .loc "test_sac_edge_cases_advanced.c", 91
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end66:
    .loc "test_sac_edge_cases_advanced.c", 134
    lda $22
    ldx $23
    sta _main__local_111
    stx _main__local_111+1
    .loc "test_sac_edge_cases_advanced.c", 135
    lda _main__local_111
    ldx _main__local_111+1
    cmp.16 .AX, #13
    bne @if_then74
    bra @if_end76
@if_then74:
    .loc "test_sac_edge_cases_advanced.c", 136
    ldax #__str_77
    sta _main__local_132
    stx _main__local_132+1
    lda _main__local_132
    ldx _main__local_132+1
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    jsr _printf
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    .loc "test_sac_edge_cases_advanced.c", 137
    bra @__return
@if_end76:
    .loc "test_sac_edge_cases_advanced.c", 141
    ldax #__str_78
    sta _main__local_134
    stx _main__local_134+1
    lda _main__local_134
    ldx _main__local_134+1
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    jsr _printf
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    .loc "test_sac_edge_cases_advanced.c", 142
    brk
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 56
    endproc


    .segment "data"
__str_38:
    .text "FAIL: many_locals
"
    .byte 0
__str_47:
    .text "FAIL: sum_array
"
    .byte 0
__str_53:
    .text "FAIL: complex_expr
"
    .byte 0
__str_59:
    .text "FAIL: get_global_plus
"
    .byte 0
__str_65:
    .text "FAIL: return_zero
"
    .byte 0
__str_77:
    .text "FAIL: deep_nesting_test
"
    .byte 0
__str_78:
    .text "SUCCESS: All edge cases passed!
"
    .byte 0

__zp_save_buf:

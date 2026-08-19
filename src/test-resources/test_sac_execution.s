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

    .global _test_marker
    .global _add
    .global _subtract
    .global _multiply
    .global _divide
    .global _sum_with_locals
    .global _nested_math
    .global _five_param
    .global _get_magic_number
    .global _main

    .segment "data"
    .byte 0
_test_marker:
; .debug_var: @global _test_marker offset=0 size=2 type=ptr scope=global
    .word 16384

    .segment "code"

; function _add
; SAC inline storage: 4 bytes
    .global _add__param_a
    _add__param_a: .word 0
    .global _add__param_b
    _add__param_b: .word 0
    _add__local_0: .word 0
    _add__local_1: .word 0
    proc _add, W#@_p_a, W#@_p_b
    .sac
    .var _fp = 0
    .loc "test_sac_execution.c", 13
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __add @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __add @_p_b offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_sac_execution.c", 14
    lda _add__param_b
    ldx _add__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _add__param_a
    ldx _add__param_a+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _subtract
; SAC inline storage: 4 bytes
    .global _subtract__param_a
    _subtract__param_a: .word 0
    .global _subtract__param_b
    _subtract__param_b: .word 0
    _subtract__local_0: .word 0
    _subtract__local_1: .word 0
    proc _subtract, W#@_p_a, W#@_p_b
    .sac
    .var _fp = 0
    .loc "test_sac_execution.c", 18
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __subtract @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __subtract @_p_b offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_sac_execution.c", 19
    lda _subtract__param_b
    ldx _subtract__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _subtract__param_a
    ldx _subtract__param_a+1
    sub.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _multiply
; SAC inline storage: 4 bytes
    .global _multiply__param_a
    _multiply__param_a: .word 0
    .global _multiply__param_b
    _multiply__param_b: .word 0
    _multiply__local_0: .word 0
    _multiply__local_1: .word 0
    proc _multiply, W#@_p_a, W#@_p_b
    .sac
    .var _fp = 0
    .loc "test_sac_execution.c", 23
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __multiply @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __multiply @_p_b offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_sac_execution.c", 24
    lda _multiply__param_b
    ldx _multiply__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _multiply__param_a
    ldx _multiply__param_a+1
    mul.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _divide
; SAC inline storage: 4 bytes
    .global _divide__param_a
    _divide__param_a: .word 0
    .global _divide__param_b
    _divide__param_b: .word 0
    _divide__local_0: .word 0
    _divide__local_1: .word 0
    proc _divide, W#@_p_a, W#@_p_b
    .sac
    .var _fp = 0
    .loc "test_sac_execution.c", 28
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __divide @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __divide @_p_b offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_sac_execution.c", 29
    lda _divide__param_b
    ldx _divide__param_b+1
    stx __zp_scratch
    ora __zp_scratch
    beq @if_then0
    bra @if_end2
@if_then0:
    lda #255
    ldx #255
    bra @__return
@if_end2:
    .loc "test_sac_execution.c", 30
    lda _divide__param_b
    ldx _divide__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _divide__param_a
    ldx _divide__param_a+1
    div.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _sum_with_locals
; SAC inline storage: 8 bytes
    .global _sum_with_locals__param_x
    _sum_with_locals__param_x: .word 0
    .global _sum_with_locals__param_y
    _sum_with_locals__param_y: .word 0
    _sum_with_locals__local_0: .word 0
    _sum_with_locals__local_1: .word 0
    _sum_with_locals__local_2: .word 0
    _sum_with_locals__local_5: .word 0
    proc _sum_with_locals, W#@_p_x, W#@_p_y
    .sac
    .var _fp = 0
    .loc "test_sac_execution.c", 34
    .local @_l_temp1 = 4
    .local @_l_temp2 = 6
; .debug_var: __sum_with_locals @_l_temp1 offset=4 size=2 type=int16 scope=local
; .debug_var: __sum_with_locals @_l_temp2 offset=6 size=2 type=int16 scope=local
    .var @_p_x = 2
    .var @_p_y = 4
; .debug_var: __sum_with_locals @_p_x offset=2 size=2 type=int16 scope=parameter
; .debug_var: __sum_with_locals @_p_y offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_sac_execution.c", 35
    lda _sum_with_locals__param_x
    ldx _sum_with_locals__param_x+1
    add.16 .AX, #10
    sta $22
    stx $23
    sta _sum_with_locals__local_2
    stx _sum_with_locals__local_2+1
    .loc "test_sac_execution.c", 36
    lda _sum_with_locals__param_y
    ldx _sum_with_locals__param_y+1
    add.16 .AX, #20
    sta $22
    stx $23
    sta _sum_with_locals__local_5
    stx _sum_with_locals__local_5+1
    .loc "test_sac_execution.c", 37
    lda _sum_with_locals__local_5
    ldx _sum_with_locals__local_5+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _sum_with_locals__local_2
    ldx _sum_with_locals__local_2+1
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
    .frame_size 8
    endproc

; function _nested_math
; SAC inline storage: 10 bytes
    .global _nested_math__param_a
    _nested_math__param_a: .word 0
    .global _nested_math__param_b
    _nested_math__param_b: .word 0
    .global _nested_math__param_c
    _nested_math__param_c: .word 0
    _nested_math__local_0: .word 0
    _nested_math__local_1: .word 0
    _nested_math__local_2: .word 0
    _nested_math__local_3: .word 0
    _nested_math__local_8: .word 0
    proc _nested_math, W#@_p_a, W#@_p_b, W#@_p_c
    .sac
    .var _fp = 0
    .loc "test_sac_execution.c", 41
    .local @_l_prod = 8
    .local @_l_sum = 6
; .debug_var: __nested_math @_l_prod offset=8 size=2 type=int16 scope=local
; .debug_var: __nested_math @_l_sum offset=6 size=2 type=int16 scope=local
    .var @_p_a = 2
    .var @_p_b = 4
    .var @_p_c = 6
; .debug_var: __nested_math @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __nested_math @_p_b offset=4 size=2 type=int16 scope=parameter
; .debug_var: __nested_math @_p_c offset=6 size=2 type=int16 scope=parameter

@entry:
    .loc "test_sac_execution.c", 42
    lda _nested_math__param_a
    ldx _nested_math__param_a+1
    sta $20
    stx $21
    lda _nested_math__param_b
    ldx _nested_math__param_b+1
    sta $20
    stx $21
    .loc "test_sac_execution.c", 14
    lda _nested_math__param_b
    ldx _nested_math__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _nested_math__param_a
    ldx _nested_math__param_a+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end3:
    .loc "test_sac_execution.c", 42
    lda $22
    ldx $23
    sta _nested_math__local_3
    stx _nested_math__local_3+1
    .loc "test_sac_execution.c", 43
    lda _nested_math__local_3
    ldx _nested_math__local_3+1
    sta $20
    stx $21
    lda _nested_math__param_c
    ldx _nested_math__param_c+1
    sta $20
    stx $21
    .loc "test_sac_execution.c", 24
    lda _nested_math__param_c
    ldx _nested_math__param_c+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _nested_math__local_3
    ldx _nested_math__local_3+1
    mul.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end5:
    .loc "test_sac_execution.c", 43
    lda $22
    ldx $23
    sta _nested_math__local_8
    stx _nested_math__local_8+1
    .loc "test_sac_execution.c", 44
    lda _nested_math__local_8
    ldx _nested_math__local_8+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 10
    endproc

; function _five_param
; SAC inline storage: 10 bytes
    .global _five_param__param_a
    _five_param__param_a: .word 0
    .global _five_param__param_b
    _five_param__param_b: .word 0
    .global _five_param__param_c
    _five_param__param_c: .word 0
    .global _five_param__param_d
    _five_param__param_d: .word 0
    .global _five_param__param_e
    _five_param__param_e: .word 0
    _five_param__local_0: .word 0
    _five_param__local_1: .word 0
    _five_param__local_2: .word 0
    _five_param__local_3: .word 0
    _five_param__local_4: .word 0
    proc _five_param, W#@_p_a, W#@_p_b, W#@_p_c, W#@_p_d, W#@_p_e
    .sac
    .var _fp = 0
    .loc "test_sac_execution.c", 48
    .var @_p_a = 2
    .var @_p_b = 4
    .var @_p_c = 6
    .var @_p_d = 8
    .var @_p_e = 10
; .debug_var: __five_param @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __five_param @_p_b offset=4 size=2 type=int16 scope=parameter
; .debug_var: __five_param @_p_c offset=6 size=2 type=int16 scope=parameter
; .debug_var: __five_param @_p_d offset=8 size=2 type=int16 scope=parameter
; .debug_var: __five_param @_p_e offset=10 size=2 type=int16 scope=parameter

@entry:
    .loc "test_sac_execution.c", 49
    lda _five_param__param_b
    ldx _five_param__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _five_param__param_a
    ldx _five_param__param_a+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda _five_param__param_c
    ldx _five_param__param_c+1
    add.16 .AX, $20
    sta $22
    stx $23
    lda _five_param__param_d
    ldx _five_param__param_d+1
    add.16 .AX, $22
    sta $20
    stx $21
    lda _five_param__param_e
    ldx _five_param__param_e+1
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

; function _get_magic_number
; SAC zero-alloc leaf: no storage overhead
    proc _get_magic_number
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_sac_execution.c", 53

@entry:
    .loc "test_sac_execution.c", 54
    lda #42
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, zeroalloc, leaf
    .reg_clobbers A, X
    .flag_clobbers N, Z
    .frame_size 0
    endproc

; function _main
; SAC inline storage: 8 bytes
    _main__local_55: .word 0
    _main__local_58: .word 0
    _main__local_74: .word 0
    _main__local_80: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_sac_execution.c", 57
    .local @_l_prod = 6
    .local @_l_sum = 4
    .local @_l_temp1 = 0
    .local @_l_temp2 = 2
; .debug_var: __main @_l_prod offset=6 size=2 type=int16 scope=local
; .debug_var: __main @_l_sum offset=4 size=2 type=int16 scope=local
; .debug_var: __main @_l_temp1 offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_temp2 offset=2 size=2 type=int16 scope=local

@entry:
    .loc "test_sac_execution.c", 59
    lda #255
    sta $20
    lda _test_marker
    ldx _test_marker+1
    sta $22
    stx $23
    lda $20
    ldy #0
    sta ($22),y
    .loc "test_sac_execution.c", 62
    lda #10
    ldx #0
    sta $20
    stx $21
    lda #20
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
    .loc "test_sac_execution.c", 14
    lda $20
    clc
    adc #20
    sta $24
    lda $21
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
@inline_end10:
    .loc "test_sac_execution.c", 62
    lda $22
    ldx $23
    cmp.16 .AX, #30
    bne @if_then7
    bra @if_end9
@if_then7:
    .loc "test_sac_execution.c", 63
    lda #1
    sta $20
    lda _test_marker
    ldx _test_marker+1
    sta $22
    stx $23
    lda $20
    ldy #0
    sta ($22),y
    .loc "test_sac_execution.c", 64
    brk
    .loc "test_sac_execution.c", 65
    bra @__return
@if_end9:
    .loc "test_sac_execution.c", 69
    lda #50
    ldx #0
    sta $20
    stx $21
    lda #30
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
    .loc "test_sac_execution.c", 19
    lda $20
    sec
    sbc #30
    sta $24
    lda $21
    sbc #0
    sta $25
    lda $24
    ldx $25
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end15:
    .loc "test_sac_execution.c", 69
    lda $22
    ldx $23
    cmp.16 .AX, #20
    bne @if_then12
    bra @if_end14
@if_then12:
    .loc "test_sac_execution.c", 70
    lda #2
    sta $20
    lda _test_marker
    ldx _test_marker+1
    sta $22
    stx $23
    lda $20
    ldy #0
    sta ($22),y
    .loc "test_sac_execution.c", 71
    brk
    .loc "test_sac_execution.c", 72
    bra @__return
@if_end14:
    .loc "test_sac_execution.c", 76
    lda #6
    ldx #0
    sta $20
    stx $21
    lda #7
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
    .loc "test_sac_execution.c", 24
    lda $20
    ldx $21
    mul.16 .AX, $22
    sta $24
    stx $25
    lda $24
    ldx $25
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end20:
    .loc "test_sac_execution.c", 76
    lda $22
    ldx $23
    cmp.16 .AX, #42
    bne @if_then17
    bra @if_end19
@if_then17:
    .loc "test_sac_execution.c", 77
    lda #3
    sta $20
    lda _test_marker
    ldx _test_marker+1
    sta $22
    stx $23
    lda $20
    ldy #0
    sta ($22),y
    .loc "test_sac_execution.c", 78
    brk
    .loc "test_sac_execution.c", 79
    bra @__return
@if_end19:
    .loc "test_sac_execution.c", 83
    lda #100
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
    sta $20
    stx $21
    .loc "test_sac_execution.c", 29
    lda $22
    ora $23
    beq @if_then26
    bra @if_end28
@if_then26:
    lda #255
    sta $22
    sta $23
    lda $22
    ldx $23
    sta $26
    stx $27
    lda $26
    ldx $27
    sta $22
    stx $23
    bra @inline_end25
@if_end28:
    .loc "test_sac_execution.c", 30
    lda $24
    ldx $25
    div.16 .AX, $20
    sta $26
    stx $27
    lda $26
    ldx $27
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end25:
    .loc "test_sac_execution.c", 83
    lda $22
    ldx $23
    cmp.16 .AX, #20
    bne @if_then22
    bra @if_end24
@if_then22:
    .loc "test_sac_execution.c", 84
    lda #4
    sta $20
    lda _test_marker
    ldx _test_marker+1
    sta $22
    stx $23
    lda $20
    ldy #0
    sta ($22),y
    .loc "test_sac_execution.c", 85
    brk
    .loc "test_sac_execution.c", 86
    bra @__return
@if_end24:
    .loc "test_sac_execution.c", 90
    lda #5
    ldx #0
    sta $20
    stx $21
    lda #15
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
    .loc "test_sac_execution.c", 35
    lda $20
    clc
    adc #10
    sta $26
    lda $21
    adc #0
    sta $27
    lda $26
    ldx $27
    sta _main__local_55
    stx _main__local_55+1
    .loc "test_sac_execution.c", 36
    lda $22
    clc
    adc #20
    sta $24
    lda $23
    adc #0
    sta $25
    lda $24
    ldx $25
    sta _main__local_58
    stx _main__local_58+1
    .loc "test_sac_execution.c", 37
    lda _main__local_58
    ldx _main__local_58+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_55
    ldx _main__local_55+1
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
@inline_end34:
    .loc "test_sac_execution.c", 90
    lda $20
    ldx $21
    cmp.16 .AX, #50
    bne @if_then31
    bra @if_end33
@if_then31:
    .loc "test_sac_execution.c", 91
    lda #5
    sta $20
    lda _test_marker
    ldx _test_marker+1
    sta $22
    stx $23
    lda $20
    ldy #0
    sta ($22),y
    .loc "test_sac_execution.c", 92
    brk
    .loc "test_sac_execution.c", 93
    bra @__return
@if_end33:
    .loc "test_sac_execution.c", 97
    lda #5
    ldx #0
    sta $20
    stx $21
    lda #10
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
    .loc "test_sac_execution.c", 42
    lda $20
    ldx $21
    sta $24
    stx $25
    lda $22
    ldx $23
    sta $24
    stx $25
    .loc "test_sac_execution.c", 14
    lda $20
    clc
    adc #10
    sta $24
    lda $21
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
@inline_end40:
    .loc "test_sac_execution.c", 42
    lda $22
    ldx $23
    sta _main__local_74
    stx _main__local_74+1
    .loc "test_sac_execution.c", 43
    lda _main__local_74
    ldx _main__local_74+1
    sta $20
    stx $21
    lda $26
    ldx $27
    sta $20
    stx $21
    .loc "test_sac_execution.c", 24
    lda _main__local_74
    ldx _main__local_74+1
    mul.16 .AX, $26
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
@inline_end42:
    .loc "test_sac_execution.c", 43
    lda $20
    ldx $21
    sta _main__local_80
    stx _main__local_80+1
    lda _main__local_80
    ldx _main__local_80+1
    sta $20
    stx $21
    .loc "test_sac_execution.c", 44
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end39:
    .loc "test_sac_execution.c", 97
    lda $22
    ldx $23
    cmp.16 .AX, #45
    bne @if_then36
    bra @if_end38
@if_then36:
    .loc "test_sac_execution.c", 98
    lda #6
    sta $20
    lda _test_marker
    ldx _test_marker+1
    sta $22
    stx $23
    lda $20
    ldy #0
    sta ($22),y
    .loc "test_sac_execution.c", 99
    brk
    .loc "test_sac_execution.c", 100
    bra @__return
@if_end38:
    .loc "test_sac_execution.c", 104
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
    .loc "test_sac_execution.c", 49
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
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end48:
    .loc "test_sac_execution.c", 104
    lda $20
    ldx $21
    cmp.16 .AX, #15
    bne @if_then45
    bra @if_end47
@if_then45:
    .loc "test_sac_execution.c", 105
    lda #7
    sta $20
    lda _test_marker
    ldx _test_marker+1
    sta $22
    stx $23
    lda $20
    ldy #0
    sta ($22),y
    .loc "test_sac_execution.c", 106
    brk
    .loc "test_sac_execution.c", 107
    bra @__return
@if_end47:
    .loc "test_sac_execution.c", 54
    lda #42
    ldx #0
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
@inline_end53:
    .loc "test_sac_execution.c", 111
    lda $20
    ldx $21
    cmp.16 .AX, #42
    bne @if_then50
    bra @if_end52
@if_then50:
    .loc "test_sac_execution.c", 112
    lda #8
    sta $20
    lda _test_marker
    ldx _test_marker+1
    sta $22
    stx $23
    lda $20
    ldy #0
    sta ($22),y
    .loc "test_sac_execution.c", 113
    brk
    .loc "test_sac_execution.c", 114
    bra @__return
@if_end52:
    .loc "test_sac_execution.c", 118
    lda #170
    sta $20
    lda _test_marker
    ldx _test_marker+1
    sta $22
    stx $23
    lda $20
    ldy #0
    sta ($22),y
    .loc "test_sac_execution.c", 119
    brk
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 8
    endproc


__zp_save_buf:

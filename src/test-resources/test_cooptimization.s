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

    .global _clamp_min
    .global _clamp_max
    .global _clamp
    .global _sum_range
    .global _product_range
    .global _main

    .segment "code"

; function _clamp_min
; SAC inline storage: 4 bytes
    .global _clamp_min__param_val
    _clamp_min__param_val: .word 0
    .global _clamp_min__param_min
    _clamp_min__param_min: .word 0
    _clamp_min__local_0: .word 0
    _clamp_min__local_1: .word 0
    proc _clamp_min, W#@_p_val, W#@_p_min
    .sac
    .var _fp = 0
    .loc "test_cooptimization.c", 5
    .var @_p_val = 2
    .var @_p_min = 4
; .debug_var: __clamp_min @_p_val offset=2 size=2 type=int16 scope=parameter
; .debug_var: __clamp_min @_p_min offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_cooptimization.c", 6
    lda _clamp_min__param_min
    ldx _clamp_min__param_min+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _clamp_min__param_val
    ldx _clamp_min__param_val+1
    cmp.16 .AX, __zp_scratch2
    bcc @if_then0
    bra @if_end2
@if_then0:
    lda _clamp_min__param_min
    ldx _clamp_min__param_min+1
    bra @__return
@if_end2:
    .loc "test_cooptimization.c", 7
    lda _clamp_min__param_val
    ldx _clamp_min__param_val+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _clamp_max
; SAC inline storage: 4 bytes
    .global _clamp_max__param_val
    _clamp_max__param_val: .word 0
    .global _clamp_max__param_max
    _clamp_max__param_max: .word 0
    _clamp_max__local_0: .word 0
    _clamp_max__local_1: .word 0
    proc _clamp_max, W#@_p_val, W#@_p_max
    .sac
    .var _fp = 0
    .loc "test_cooptimization.c", 10
    .var @_p_val = 2
    .var @_p_max = 4
; .debug_var: __clamp_max @_p_val offset=2 size=2 type=int16 scope=parameter
; .debug_var: __clamp_max @_p_max offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_cooptimization.c", 11
    lda _clamp_max__param_max
    ldx _clamp_max__param_max+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _clamp_max__param_val
    ldx _clamp_max__param_val+1
    cmp.16 .AX, __zp_scratch2
    beq @if_end5
    bcs @if_then3
    bra @if_end5
@if_then3:
    lda _clamp_max__param_max
    ldx _clamp_max__param_max+1
    bra @__return
@if_end5:
    .loc "test_cooptimization.c", 12
    lda _clamp_max__param_val
    ldx _clamp_max__param_val+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _clamp
; SAC inline storage: 6 bytes
    .global _clamp__param_val
    _clamp__param_val: .word 0
    .global _clamp__param_min
    _clamp__param_min: .word 0
    .global _clamp__param_max
    _clamp__param_max: .word 0
    _clamp__local_0: .word 0
    _clamp__local_1: .word 0
    _clamp__local_2: .word 0
    proc _clamp, W#@_p_val, W#@_p_min, W#@_p_max
    .sac
    .var _fp = 0
    .loc "test_cooptimization.c", 15
    .var @_p_val = 2
    .var @_p_min = 4
    .var @_p_max = 6
; .debug_var: __clamp @_p_val offset=2 size=2 type=int16 scope=parameter
; .debug_var: __clamp @_p_min offset=4 size=2 type=int16 scope=parameter
; .debug_var: __clamp @_p_max offset=6 size=2 type=int16 scope=parameter

@entry:
    .loc "test_cooptimization.c", 16
    lda _clamp__param_val
    ldx _clamp__param_val+1
    sta $20
    stx $21
    lda _clamp__param_min
    ldx _clamp__param_min+1
    sta $22
    stx $23
    .loc "test_cooptimization.c", 6
    lda _clamp__param_min
    ldx _clamp__param_min+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _clamp__param_val
    ldx _clamp__param_val+1
    cmp.16 .AX, __zp_scratch2
    bcc @if_then7
    bra @if_end9
@if_then7:
    lda $22
    ldx $23
    sta $24
    stx $25
    bra @inline_end6
@if_end9:
    .loc "test_cooptimization.c", 7
    lda $20
    ldx $21
    sta $24
    stx $25
@inline_end6:
    .loc "test_cooptimization.c", 16
    lda $24
    ldx $25
    sta $20
    stx $21
    lda _clamp__param_max
    ldx _clamp__param_max+1
    sta $22
    stx $23
    .loc "test_cooptimization.c", 11
    lda _clamp__param_max
    ldx _clamp__param_max+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $24
    ldx $25
    cmp.16 .AX, __zp_scratch2
    beq @if_end15
    bcs @if_then13
    bra @if_end15
@if_then13:
    lda $22
    ldx $23
    sta $24
    stx $25
    bra @inline_end12
@if_end15:
    .loc "test_cooptimization.c", 12
    lda $20
    ldx $21
    sta $24
    stx $25
@inline_end12:
    .loc "test_cooptimization.c", 16
    lda $24
    ldx $25
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _sum_range
; SAC inline storage: 8 bytes
    .global _sum_range__param_start
    _sum_range__param_start: .word 0
    .global _sum_range__param_end
    _sum_range__param_end: .word 0
    _sum_range__local_0: .word 0
    _sum_range__local_1: .word 0
    _sum_range__local_2: .word 0
    _sum_range__local_4: .word 0
    proc _sum_range, W#@_p_start, W#@_p_end
    .sac
    .var _fp = 0
    .loc "test_cooptimization.c", 20
    .local @_l_i = 6
    .local @_l_sum = 4
; .debug_var: __sum_range @_l_i offset=6 size=2 type=int16 scope=local
; .debug_var: __sum_range @_l_sum offset=4 size=2 type=int16 scope=local
    .var @_p_start = 2
    .var @_p_end = 4
; .debug_var: __sum_range @_p_start offset=2 size=2 type=int16 scope=parameter
; .debug_var: __sum_range @_p_end offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_cooptimization.c", 21
    lda #0
    sta _sum_range__local_2
    sta _sum_range__local_2+1
    .loc "test_cooptimization.c", 22
    lda _sum_range__param_start
    ldx _sum_range__param_start+1
    sta _sum_range__local_4
    stx _sum_range__local_4+1
@for_cond18:
    lda _sum_range__param_end
    ldx _sum_range__param_end+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _sum_range__local_4
    ldx _sum_range__local_4+1
    cmp.16 .AX, __zp_scratch2
    bcc @for_body19
    beq @for_body19
    bra @for_end21
@for_body19:
    .loc "test_cooptimization.c", 23
    lda _sum_range__local_4
    ldx _sum_range__local_4+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _sum_range__local_2
    ldx _sum_range__local_2+1
    add.16 .AX, __zp_scratch2
    sta $22
    stx $23
    sta _sum_range__local_2
    stx _sum_range__local_2+1
@for_inc20:
    .loc "test_cooptimization.c", 22
    lda _sum_range__local_4
    ldx _sum_range__local_4+1
    sta $24
    stx $25
    lda $24
    clc
    adc #1
    sta $26
    lda $25
    adc #0
    sta $27
    lda $26
    ldx $27
    sta _sum_range__local_4
    stx _sum_range__local_4+1
    bra @for_cond18
@for_end21:
    .loc "test_cooptimization.c", 25
    lda _sum_range__local_2
    ldx _sum_range__local_2+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 8
    endproc

; function _product_range
; SAC inline storage: 8 bytes
    .global _product_range__param_start
    _product_range__param_start: .word 0
    .global _product_range__param_end
    _product_range__param_end: .word 0
    _product_range__local_0: .word 0
    _product_range__local_1: .word 0
    _product_range__local_2: .word 0
    _product_range__local_4: .word 0
    proc _product_range, W#@_p_start, W#@_p_end
    .sac
    .var _fp = 0
    .loc "test_cooptimization.c", 28
    .local @_l_i = 6
    .local @_l_prod = 4
; .debug_var: __product_range @_l_i offset=6 size=2 type=int16 scope=local
; .debug_var: __product_range @_l_prod offset=4 size=2 type=int16 scope=local
    .var @_p_start = 2
    .var @_p_end = 4
; .debug_var: __product_range @_p_start offset=2 size=2 type=int16 scope=parameter
; .debug_var: __product_range @_p_end offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_cooptimization.c", 29
    lda #1
    sta _product_range__local_2
    lda #0
    sta _product_range__local_2+1
    .loc "test_cooptimization.c", 30
    lda _product_range__param_start
    ldx _product_range__param_start+1
    sta _product_range__local_4
    stx _product_range__local_4+1
@for_cond22:
    lda _product_range__param_end
    ldx _product_range__param_end+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _product_range__local_4
    ldx _product_range__local_4+1
    cmp.16 .AX, __zp_scratch2
    bcc @for_body23
    beq @for_body23
    bra @for_end25
@for_body23:
    .loc "test_cooptimization.c", 31
    lda _product_range__local_4
    ldx _product_range__local_4+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _product_range__local_2
    ldx _product_range__local_2+1
    mul.16 .AX, __zp_scratch2
    sta $22
    stx $23
    lda $22
    ldx $23
    sta _product_range__local_2
    stx _product_range__local_2+1
@for_inc24:
    .loc "test_cooptimization.c", 30
    lda _product_range__local_4
    ldx _product_range__local_4+1
    sta $24
    stx $25
    lda $24
    clc
    adc #1
    sta $26
    lda $25
    adc #0
    sta $27
    lda $26
    ldx $27
    sta _product_range__local_4
    stx _product_range__local_4+1
    bra @for_cond22
@for_end25:
    .loc "test_cooptimization.c", 33
    lda _product_range__local_2
    ldx _product_range__local_2+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 8
    endproc

; function _main
; SAC inline storage: 18 bytes
    _main__local_0: .word 0
    _main__local_16: .word 0
    _main__local_32: .word 0
    _main__local_54: .word 0
    _main__local_67: .word 0
    _main__local_69: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_cooptimization.c", 37
    .local @_l_a = 0
    .local @_l_b = 2
    .local @_l_c = 4
    .local @_l_i = 16
    .local @_l_prod = 14
    .local @_l_sum = 8
; .debug_var: __main @_l_a offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_b offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_c offset=4 size=2 type=int16 scope=local
; .debug_var: __main @_l_i offset=16 size=2 type=int16 scope=local
; .debug_var: __main @_l_prod offset=14 size=2 type=int16 scope=local
; .debug_var: __main @_l_sum offset=8 size=2 type=int16 scope=local

@entry:
    .loc "test_cooptimization.c", 38
    lda #15
    ldx #0
    sta $20
    stx $21
    lda #10
    ldx #0
    sta $22
    stx $23
    lda #20
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
    .loc "test_cooptimization.c", 16
    lda $20
    ldx $21
    sta $24
    stx $25
    lda $22
    ldx $23
    sta $28
    stx $29
    .loc "test_cooptimization.c", 6
    lda $20
    ldx $21
    cmp.16 .AX, #10
    bcc @if_then28
    bra @if_end30
@if_then28:
    lda $28
    ldx $29
    sta $20
    stx $21
    bra @inline_end27
@if_end30:
    .loc "test_cooptimization.c", 7
    lda $24
    ldx $25
    sta $20
    stx $21
@inline_end27:
    .loc "test_cooptimization.c", 16
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $26
    ldx $27
    sta $24
    stx $25
    .loc "test_cooptimization.c", 11
    lda $20
    ldx $21
    cmp.16 .AX, $26
    beq @if_end36
    bcs @if_then34
    bra @if_end36
@if_then34:
    lda $24
    ldx $25
    sta $20
    stx $21
    bra @inline_end33
@if_end36:
    .loc "test_cooptimization.c", 12
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end33:
    .loc "test_cooptimization.c", 16
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end26:
    .loc "test_cooptimization.c", 38
    lda $22
    ldx $23
    sta _main__local_0
    stx _main__local_0+1
    .loc "test_cooptimization.c", 39
    lda #5
    ldx #0
    sta $20
    stx $21
    lda #10
    ldx #0
    sta $22
    stx $23
    lda #20
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
    .loc "test_cooptimization.c", 16
    lda $20
    ldx $21
    sta $24
    stx $25
    lda $22
    ldx $23
    sta $28
    stx $29
    .loc "test_cooptimization.c", 6
    lda $20
    ldx $21
    cmp.16 .AX, #10
    bcc @if_then42
    bra @if_end44
@if_then42:
    lda $28
    ldx $29
    sta $20
    stx $21
    bra @inline_end41
@if_end44:
    .loc "test_cooptimization.c", 7
    lda $24
    ldx $25
    sta $20
    stx $21
@inline_end41:
    .loc "test_cooptimization.c", 16
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $26
    ldx $27
    sta $24
    stx $25
    .loc "test_cooptimization.c", 11
    lda $20
    ldx $21
    cmp.16 .AX, $26
    beq @if_end50
    bcs @if_then48
    bra @if_end50
@if_then48:
    lda $24
    ldx $25
    sta $20
    stx $21
    bra @inline_end47
@if_end50:
    .loc "test_cooptimization.c", 12
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end47:
    .loc "test_cooptimization.c", 16
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end40:
    .loc "test_cooptimization.c", 39
    lda $22
    ldx $23
    sta _main__local_16
    stx _main__local_16+1
    .loc "test_cooptimization.c", 40
    lda #25
    ldx #0
    sta $20
    stx $21
    lda #10
    ldx #0
    sta $22
    stx $23
    lda #20
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
    .loc "test_cooptimization.c", 16
    lda $20
    ldx $21
    sta $24
    stx $25
    lda $22
    ldx $23
    sta $28
    stx $29
    .loc "test_cooptimization.c", 6
    lda $20
    ldx $21
    cmp.16 .AX, #10
    bcc @if_then56
    bra @if_end58
@if_then56:
    lda $28
    ldx $29
    sta $20
    stx $21
    bra @inline_end55
@if_end58:
    .loc "test_cooptimization.c", 7
    lda $24
    ldx $25
    sta $20
    stx $21
@inline_end55:
    .loc "test_cooptimization.c", 16
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $26
    ldx $27
    sta $24
    stx $25
    .loc "test_cooptimization.c", 11
    lda $20
    ldx $21
    cmp.16 .AX, $26
    beq @if_end64
    bcs @if_then62
    bra @if_end64
@if_then62:
    lda $24
    ldx $25
    sta $20
    stx $21
    bra @inline_end61
@if_end64:
    .loc "test_cooptimization.c", 12
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end61:
    .loc "test_cooptimization.c", 16
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end54:
    .loc "test_cooptimization.c", 40
    lda $22
    ldx $23
    sta _main__local_32
    stx _main__local_32+1
    .loc "test_cooptimization.c", 42
    lda #1
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
    .loc "test_cooptimization.c", 21
    lda #0
    sta _main__local_54
    sta _main__local_54+1
    .loc "test_cooptimization.c", 22
    lda $20
    ldx $21
    sta $22
    stx $23
@for_cond69:
    lda $22
    ldx $23
    cmp.16 .AX, $24
    bcc @for_body70
    beq @for_body70
    bra @for_end72
@for_body70:
    .loc "test_cooptimization.c", 23
    lda _main__local_54
    ldx _main__local_54+1
    add.16 .AX, $22
    sta $26
    stx $27
    sta _main__local_54
    stx _main__local_54+1
@for_inc71:
    .loc "test_cooptimization.c", 22
    lda $22
    ldx $23
    sta $28
    stx $29
    lda $28
    clc
    adc #1
    sta $22
    lda $29
    adc #0
    sta $23
    bra @for_cond69
@for_end72:
    .loc "test_cooptimization.c", 25
    lda _main__local_54
    ldx _main__local_54+1
    sta $20
    stx $21
@inline_end68:
    .loc "test_cooptimization.c", 42
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_cooptimization.c", 43
    lda #1
    ldx #0
    sta $20
    stx $21
    lda #5
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx $21
    sta $26
    stx $27
    lda $24
    ldx $25
    sta $26
    stx $27
    .loc "test_cooptimization.c", 29
    lda #1
    sta _main__local_67
    lda #0
    sta _main__local_67+1
    .loc "test_cooptimization.c", 30
    lda $20
    ldx $21
    sta _main__local_69
    stx _main__local_69+1
@for_cond75:
    lda _main__local_69
    ldx _main__local_69+1
    cmp.16 .AX, $26
    bcc @for_body76
    beq @for_body76
    bra @for_end78
@for_body76:
    .loc "test_cooptimization.c", 31
    lda _main__local_69
    ldx _main__local_69+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_67
    ldx _main__local_67+1
    mul.16 .AX, __zp_scratch2
    sta $24
    stx $25
    lda $24
    ldx $25
    sta _main__local_67
    stx _main__local_67+1
@for_inc77:
    .loc "test_cooptimization.c", 30
    lda _main__local_69
    ldx _main__local_69+1
    sta $28
    stx $29
    lda $28
    clc
    adc #1
    sta $2A
    lda $29
    adc #0
    sta $2B
    lda $2A
    ldx $2B
    sta _main__local_69
    stx _main__local_69+1
    bra @for_cond75
@for_end78:
    .loc "test_cooptimization.c", 33
    lda _main__local_67
    ldx _main__local_67+1
    sta $20
    stx $21
@inline_end74:
    .loc "test_cooptimization.c", 43
    lda $20
    ldx $21
    sta $24
    stx $25
    .loc "test_cooptimization.c", 45
    lda _main__local_16
    ldx _main__local_16+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_0
    ldx _main__local_0+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda _main__local_32
    ldx _main__local_32+1
    add.16 .AX, $20
    sta $26
    stx $27
    lda $26
    clc
    adc $22
    sta $20
    lda $27
    adc $22+1
    sta $21
    lda $20
    clc
    adc $24
    sta $22
    lda $21
    adc $24+1
    sta $23
    lda $22
    ldx $23
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 18
    endproc


__zp_save_buf:

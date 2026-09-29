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

    .extern _exit
    .extern _printf

    .global _stack_add
    .global _stack_long_add
    .global _stack_three_params
    .global _zp_add
    .global _zp_long_add
    .global _zp_three_params
    .global _zp_mixed_caller
    .global _test_stack_to_stack
    .global _test_stack_to_zp
    .global _zp_test_zp_to_zp
    .global _test_zp_to_zp
    .global _test_long_returns
    .global _test_multi_params
    .global _test_mixed_conventions
    .global _stack_modify_global
    .global _zp_modify_global
    .global _test_global_state
    .global _test_register_clobbering
    .global _main

    .segment "data"
    .byte 0
_global_state:
; .debug_var: @global _global_state offset=0 size=2 type=int16 scope=global
    .word 100

    .segment "code"

; function _stack_add
; SAC inline storage: 4 bytes
    .global _stack_add__param_a
    _stack_add__param_a: .word 0
    .global _stack_add__param_b
    _stack_add__param_b: .word 0
    _stack_add__local_0: .word 0
    _stack_add__local_1: .word 0
    proc _stack_add, W#@_p_a, W#@_p_b
    .sac
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdio.h", 5
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __stack_add @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __stack_add @_p_b offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_cc_safety_matrix.c", 15
    lda _stack_add__param_b
    ldx _stack_add__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _stack_add__param_a
    ldx _stack_add__param_a+1
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

; function _stack_long_add

    ; Static buffer for struct return from _stack_long_add
    _stack_long_add__struct_buf:
    .byte 0, 0, 0, 0

; SAC inline storage: 8 bytes
    .global _stack_long_add__param_a
    _stack_long_add__param_a: .long 0
    .global _stack_long_add__param_b
    _stack_long_add__param_b: .long 0
    _stack_long_add__local_0: .long 0
    _stack_long_add__local_1: .long 0
    proc _stack_long_add, D#@_p_a, D#@_p_b
    .sac
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdio.h", 9
    .var @_p_a = 2
    .var @_p_b = 6
; .debug_var: __stack_long_add @_p_a offset=2 size=4 type=int32 scope=parameter
; .debug_var: __stack_long_add @_p_b offset=6 size=4 type=int32 scope=parameter

@entry:
    .loc "test_cc_safety_matrix.c", 19
    lda _stack_long_add__param_b
    ldx _stack_long_add__param_b+1
    ldy _stack_long_add__param_b+2
    ldz _stack_long_add__param_b+3
    sta $10
    stx $11
    sty $12
    stz $13
    lda _stack_long_add__param_a
    ldx _stack_long_add__param_a+1
    ldy _stack_long_add__param_a+2
    ldz _stack_long_add__param_a+3
    add.32 .AXYZ, $10
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 8
    endproc

; function _stack_three_params
; SAC inline storage: 6 bytes
    .global _stack_three_params__param_a
    _stack_three_params__param_a: .word 0
    .global _stack_three_params__param_b
    _stack_three_params__param_b: .word 0
    .global _stack_three_params__param_c
    _stack_three_params__param_c: .word 0
    _stack_three_params__local_0: .word 0
    _stack_three_params__local_1: .word 0
    _stack_three_params__local_2: .word 0
    proc _stack_three_params, W#@_p_a, W#@_p_b, W#@_p_c
    .sac
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdio.h", 13
    .var @_p_a = 2
    .var @_p_b = 4
    .var @_p_c = 6
; .debug_var: __stack_three_params @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __stack_three_params @_p_b offset=4 size=2 type=int16 scope=parameter
; .debug_var: __stack_three_params @_p_c offset=6 size=2 type=int16 scope=parameter

@entry:
    .loc "test_cc_safety_matrix.c", 23
    lda _stack_three_params__param_c
    ldx _stack_three_params__param_c+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _stack_three_params__param_b
    ldx _stack_three_params__param_b+1
    mul.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda _stack_three_params__param_a
    ldx _stack_three_params__param_a+1
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
    .frame_size 6
    endproc

; function _zp_add
; SAC inline storage: 4 bytes
    .global _zp_add__param_a
    _zp_add__param_a: .word 0
    .global _zp_add__param_b
    _zp_add__param_b: .word 0
    _zp_add__local_0: .word 0
    _zp_add__local_1: .word 0
    proc _zp_add, W#@_p_a, W#@_p_b
    .sac
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdio.h", 18
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __zp_add @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __zp_add @_p_b offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_cc_safety_matrix.c", 28
    lda _zp_add__param_b
    ldx _zp_add__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _zp_add__param_a
    ldx _zp_add__param_a+1
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

; function _zp_long_add

    ; Static buffer for struct return from _zp_long_add
    _zp_long_add__struct_buf:
    .byte 0, 0, 0, 0

; SAC inline storage: 8 bytes
    .global _zp_long_add__param_a
    _zp_long_add__param_a: .long 0
    .global _zp_long_add__param_b
    _zp_long_add__param_b: .long 0
    _zp_long_add__local_0: .long 0
    _zp_long_add__local_1: .long 0
    proc _zp_long_add, D#@_p_a, D#@_p_b
    .sac
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdlib.h", 1
    .var @_p_a = 2
    .var @_p_b = 6
; .debug_var: __zp_long_add @_p_a offset=2 size=4 type=int32 scope=parameter
; .debug_var: __zp_long_add @_p_b offset=6 size=4 type=int32 scope=parameter

@entry:
    .loc "test_cc_safety_matrix.c", 32
    lda _zp_long_add__param_b
    ldx _zp_long_add__param_b+1
    ldy _zp_long_add__param_b+2
    ldz _zp_long_add__param_b+3
    sta $10
    stx $11
    sty $12
    stz $13
    lda _zp_long_add__param_a
    ldx _zp_long_add__param_a+1
    ldy _zp_long_add__param_a+2
    ldz _zp_long_add__param_a+3
    add.32 .AXYZ, $10
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 8
    endproc

; function _zp_three_params
; SAC inline storage: 6 bytes
    .global _zp_three_params__param_a
    _zp_three_params__param_a: .word 0
    .global _zp_three_params__param_b
    _zp_three_params__param_b: .word 0
    .global _zp_three_params__param_c
    _zp_three_params__param_c: .word 0
    _zp_three_params__local_0: .word 0
    _zp_three_params__local_1: .word 0
    _zp_three_params__local_2: .word 0
    proc _zp_three_params, W#@_p_a, W#@_p_b, W#@_p_c
    .sac
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdlib.h", 5
    .var @_p_a = 2
    .var @_p_b = 4
    .var @_p_c = 6
; .debug_var: __zp_three_params @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __zp_three_params @_p_b offset=4 size=2 type=int16 scope=parameter
; .debug_var: __zp_three_params @_p_c offset=6 size=2 type=int16 scope=parameter

@entry:
    .loc "test_cc_safety_matrix.c", 36
    lda _zp_three_params__param_c
    ldx _zp_three_params__param_c+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _zp_three_params__param_b
    ldx _zp_three_params__param_b+1
    mul.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda _zp_three_params__param_a
    ldx _zp_three_params__param_a+1
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
    .frame_size 6
    endproc

; function _zp_mixed_caller
; SAC inline storage: 6 bytes
    .global _zp_mixed_caller__param_x
    _zp_mixed_caller__param_x: .word 0
    _zp_mixed_caller__local_0: .word 0
    _zp_mixed_caller__local_1: .word 0
    _zp_mixed_caller__local_7: .word 0
    proc _zp_mixed_caller, W#@_p_x
    .sac
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stddef.h", 5
    .local @_l_s = 2
    .local @_l_z = 4
; .debug_var: __zp_mixed_caller @_l_s offset=2 size=2 type=int16 scope=local
; .debug_var: __zp_mixed_caller @_l_z offset=4 size=2 type=int16 scope=local
    .var @_p_x = 2
; .debug_var: __zp_mixed_caller @_p_x offset=2 size=2 type=int16 scope=parameter

@entry:
    .loc "test_cc_safety_matrix.c", 41
    lda #1
    ldx #0
    sta $20
    stx $21
    lda _zp_mixed_caller__param_x
    ldx _zp_mixed_caller__param_x+1
    sta $22
    stx $23
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_cc_safety_matrix.c", 15
    lda _zp_mixed_caller__param_x
    ldx _zp_mixed_caller__param_x+1
    add.16 .AX, $20
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end0:
    .loc "test_cc_safety_matrix.c", 41
    lda $20
    ldx $21
    sta _zp_mixed_caller__local_1
    stx _zp_mixed_caller__local_1+1
    .loc "test_cc_safety_matrix.c", 42
    lda #2
    ldx #0
    sta $20
    stx $21
    lda _zp_mixed_caller__param_x
    ldx _zp_mixed_caller__param_x+1
    sta $22
    stx $23
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_cc_safety_matrix.c", 28
    lda _zp_mixed_caller__param_x
    ldx _zp_mixed_caller__param_x+1
    add.16 .AX, $20
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end2:
    .loc "test_cc_safety_matrix.c", 42
    lda $20
    ldx $21
    sta _zp_mixed_caller__local_7
    stx _zp_mixed_caller__local_7+1
    .loc "test_cc_safety_matrix.c", 43
    lda _zp_mixed_caller__local_7
    ldx _zp_mixed_caller__local_7+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _zp_mixed_caller__local_1
    ldx _zp_mixed_caller__local_1+1
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
    .frame_size 6
    endproc

; function _test_stack_to_stack
; SAC inline storage: 2 bytes
    _test_stack_to_stack__local_0: .word 0
    _test_stack_to_stack__local_10: .word 0
    _test_stack_to_stack__local_12: .word 0
    _test_stack_to_stack__local_13: .word 0
    proc _test_stack_to_stack
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stddef.h", 12
    .local @_l_r = 0
; .debug_var: __test_stack_to_stack @_l_r offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_cc_safety_matrix.c", 48
    lda #5
    ldx #0
    sta $20
    stx $21
    lda #3
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
    .loc "test_cc_safety_matrix.c", 15
    lda $20
    clc
    adc #3
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
@inline_end4:
    .loc "test_cc_safety_matrix.c", 48
    lda $22
    ldx $23
    sta _test_stack_to_stack__local_0
    stx _test_stack_to_stack__local_0+1
    .loc "test_cc_safety_matrix.c", 49
    lda _test_stack_to_stack__local_0
    ldx _test_stack_to_stack__local_0+1
    cmp.16 .AX, #8
    bne @if_then6
    bra @if_end8
@if_then6:
    .loc "test_cc_safety_matrix.c", 50
    ldax #__str_9
    sta _test_stack_to_stack__local_10
    stx _test_stack_to_stack__local_10+1
    lda _test_stack_to_stack__local_10
    ldx _test_stack_to_stack__local_10+1
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
    .loc "test_cc_safety_matrix.c", 51
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end8:
    .loc "test_cc_safety_matrix.c", 53
    ldax #__str_10
    sta _test_stack_to_stack__local_13
    stx _test_stack_to_stack__local_13+1
    lda _test_stack_to_stack__local_13
    ldx _test_stack_to_stack__local_13+1
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
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _test_stack_to_zp
; SAC inline storage: 2 bytes
    _test_stack_to_zp__local_0: .word 0
    _test_stack_to_zp__local_10: .word 0
    _test_stack_to_zp__local_12: .word 0
    _test_stack_to_zp__local_13: .word 0
    proc _test_stack_to_zp
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdlib.h", 14
    .local @_l_r = 0
; .debug_var: __test_stack_to_zp @_l_r offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_cc_safety_matrix.c", 58
    lda #7
    ldx #0
    sta $20
    stx $21
    lda #2
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
    .loc "test_cc_safety_matrix.c", 28
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
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end11:
    .loc "test_cc_safety_matrix.c", 58
    lda $22
    ldx $23
    sta _test_stack_to_zp__local_0
    stx _test_stack_to_zp__local_0+1
    .loc "test_cc_safety_matrix.c", 59
    lda _test_stack_to_zp__local_0
    ldx _test_stack_to_zp__local_0+1
    cmp.16 .AX, #9
    bne @if_then13
    bra @if_end15
@if_then13:
    .loc "test_cc_safety_matrix.c", 60
    ldax #__str_16
    sta _test_stack_to_zp__local_10
    stx _test_stack_to_zp__local_10+1
    lda _test_stack_to_zp__local_10
    ldx _test_stack_to_zp__local_10+1
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
    .loc "test_cc_safety_matrix.c", 61
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end15:
    .loc "test_cc_safety_matrix.c", 63
    ldax #__str_17
    sta _test_stack_to_zp__local_13
    stx _test_stack_to_zp__local_13+1
    lda _test_stack_to_zp__local_13
    ldx _test_stack_to_zp__local_13+1
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
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _zp_test_zp_to_zp
; SAC inline storage: 2 bytes
    _zp_test_zp_to_zp__local_0: .word 0
    proc _zp_test_zp_to_zp
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdlib.h", 24
    .local @_l_r = 0
; .debug_var: __zp_test_zp_to_zp @_l_r offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_cc_safety_matrix.c", 68
    lda #4
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
    .loc "test_cc_safety_matrix.c", 28
    lda $20
    clc
    adc #5
    sta $24
    lda $21
    adc #0
    sta $25
    lda $24
    ldx $25
    sta $20
    stx $21
@inline_end18:
    .loc "test_cc_safety_matrix.c", 68
    lda $20
    ldx $21
    sta _zp_test_zp_to_zp__local_0
    stx _zp_test_zp_to_zp__local_0+1
    .loc "test_cc_safety_matrix.c", 69
    lda _zp_test_zp_to_zp__local_0
    ldx _zp_test_zp_to_zp__local_0+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _test_zp_to_zp
; SAC inline storage: 4 bytes
    _test_zp_to_zp__local_2: .word 0
    _test_zp_to_zp__local_13: .word 0
    _test_zp_to_zp__local_15: .word 0
    _test_zp_to_zp__local_16: .word 0
    proc _test_zp_to_zp
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdlib.h", 29
    .local @_l_r = 2
; .debug_var: __test_zp_to_zp @_l_r offset=2 size=2 type=int16 scope=local

@entry:
    .loc "test_cc_safety_matrix.c", 68
    lda #4
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
    .loc "test_cc_safety_matrix.c", 28
    lda $20
    clc
    adc #5
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
@inline_end21:
    .loc "test_cc_safety_matrix.c", 68
    lda $22
    ldx $23
    sta _test_zp_to_zp__local_2
    stx _test_zp_to_zp__local_2+1
    lda _test_zp_to_zp__local_2
    ldx _test_zp_to_zp__local_2+1
    sta $20
    stx $21
    .loc "test_cc_safety_matrix.c", 69
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end20:
    .loc "test_cc_safety_matrix.c", 73
    lda $22
    ldx $23
    sta $20
    stx $21
    .loc "test_cc_safety_matrix.c", 74
    lda $20
    ldx $21
    cmp.16 .AX, #9
    bne @if_then24
    bra @if_end26
@if_then24:
    .loc "test_cc_safety_matrix.c", 75
    ldax #__str_27
    sta _test_zp_to_zp__local_13
    stx _test_zp_to_zp__local_13+1
    lda _test_zp_to_zp__local_13
    ldx _test_zp_to_zp__local_13+1
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
    .loc "test_cc_safety_matrix.c", 76
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end26:
    .loc "test_cc_safety_matrix.c", 78
    ldax #__str_28
    sta _test_zp_to_zp__local_16
    stx _test_zp_to_zp__local_16+1
    lda _test_zp_to_zp__local_16
    ldx _test_zp_to_zp__local_16+1
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
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _test_long_returns
; SAC inline storage: 8 bytes
    _test_long_returns__local_0: .long 0
    _test_long_returns__local_10: .long 0
    _test_long_returns__local_26: .word 0
    _test_long_returns__local_28: .word 0
    _test_long_returns__local_29: .word 0
    proc _test_long_returns
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdlib.h", 39
    .local @_l_s_r = 0
    .local @_l_z_r = 4
; .debug_var: __test_long_returns @_l_s_r offset=0 size=4 type=int32 scope=local
; .debug_var: __test_long_returns @_l_z_r offset=4 size=4 type=int32 scope=local

@entry:
    .loc "test_cc_safety_matrix.c", 83
    lda #0
    ldx #16
    sta $20
    stx $21
    lda #0
    ldx #32
    sta $22
    stx $23
    lda $20
    ldx $21
    ldy #0
    ldz #0
    sta $24
    stx $25
    sty $26
    stz $27
    lda $22
    ldx $23
    ldy #0
    ldz #0
    sta $28
    stx $29
    sty $2A
    stz $2B
    lda $24
    ldx $25
    ldy $26
    ldz $27
    sta $20
    stx $21
    sty $22
    stz $23
    lda $28
    ldx $29
    ldy $2A
    ldz $2B
    sta $20
    stx $21
    sty $22
    stz $23
    .loc "test_cc_safety_matrix.c", 19
    lda $24
    ldx $25
    ldy $26
    ldz $27
    add.32 .AXYZ, $28
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    sta $24
    stx $25
    lda $24
    ldx $25
    sta $20
    stx $21
    sty $22
    stz $23
@inline_end29:
    .loc "test_cc_safety_matrix.c", 83
    lda $20
    ldx $21
    ldy $22
    ldz $23
    sta _test_long_returns__local_0
    stx _test_long_returns__local_0+1
    sty _test_long_returns__local_0+2
    stz _test_long_returns__local_0+3
    .loc "test_cc_safety_matrix.c", 84
    lda #0
    ldx #48
    sta $20
    stx $21
    lda #0
    ldx #64
    sta $22
    stx $23
    lda $20
    ldx $21
    ldy #0
    ldz #0
    sta $24
    stx $25
    sty $26
    stz $27
    lda $22
    ldx $23
    ldy #0
    ldz #0
    sta $28
    stx $29
    sty $2A
    stz $2B
    lda $24
    ldx $25
    ldy $26
    ldz $27
    sta $20
    stx $21
    sty $22
    stz $23
    lda $28
    ldx $29
    ldy $2A
    ldz $2B
    sta $20
    stx $21
    sty $22
    stz $23
    .loc "test_cc_safety_matrix.c", 32
    lda $24
    ldx $25
    ldy $26
    ldz $27
    add.32 .AXYZ, $28
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    sta $24
    stx $25
    lda $24
    ldx $25
    sta $20
    stx $21
    sty $22
    stz $23
@inline_end31:
    .loc "test_cc_safety_matrix.c", 84
    lda $20
    ldx $21
    ldy $22
    ldz $23
    sta _test_long_returns__local_10
    stx _test_long_returns__local_10+1
    sty _test_long_returns__local_10+2
    stz _test_long_returns__local_10+3
    .loc "test_cc_safety_matrix.c", 86
    lda #0
    ldx #48
    sta $20
    stx $21
    lda $20
    ldx $21
    ldy #0
    ldz #0
    sta $22
    stx $23
    sty $24
    stz $25
    lda _test_long_returns__local_0
    ldx _test_long_returns__local_0+1
    ldy _test_long_returns__local_0+2
    ldz _test_long_returns__local_0+3
    cmp.32 .AXYZ, $22
    bne @if_then33
@or_rhs36:
    lda #0
    ldx #112
    sta $20
    stx $21
    lda $20
    ldx $21
    ldy #0
    ldz #0
    sta $22
    stx $23
    sty $24
    stz $25
    lda _test_long_returns__local_10
    ldx _test_long_returns__local_10+1
    ldy _test_long_returns__local_10+2
    ldz _test_long_returns__local_10+3
    cmp.32 .AXYZ, $22
    bne @if_then33
    bra @if_end35
@if_then33:
    .loc "test_cc_safety_matrix.c", 87
    ldax #__str_37
    sta _test_long_returns__local_26
    stx _test_long_returns__local_26+1
    lda _test_long_returns__local_26
    ldx _test_long_returns__local_26+1
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
    .loc "test_cc_safety_matrix.c", 88
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end35:
    .loc "test_cc_safety_matrix.c", 90
    ldax #__str_38
    sta _test_long_returns__local_29
    stx _test_long_returns__local_29+1
    lda _test_long_returns__local_29
    ldx _test_long_returns__local_29+1
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
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 8
    endproc

; function _test_multi_params
; SAC inline storage: 4 bytes
    _test_multi_params__local_0: .word 0
    _test_multi_params__local_11: .word 0
    _test_multi_params__local_26: .word 0
    _test_multi_params__local_28: .word 0
    _test_multi_params__local_29: .word 0
    proc _test_multi_params
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdlib.h", 51
    .local @_l_s_r = 0
    .local @_l_z_r = 2
; .debug_var: __test_multi_params @_l_s_r offset=0 size=2 type=int16 scope=local
; .debug_var: __test_multi_params @_l_z_r offset=2 size=2 type=int16 scope=local

@entry:
    .loc "test_cc_safety_matrix.c", 95
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
    .loc "test_cc_safety_matrix.c", 23
    lda $22
    ldx $23
    mul.16 .AX, $24
    sta $26
    stx $27
    lda $20
    clc
    adc $26
    sta $22
    lda $21
    adc $26+1
    sta $23
    lda $22
    ldx $23
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end39:
    .loc "test_cc_safety_matrix.c", 95
    lda $22
    ldx $23
    sta _test_multi_params__local_0
    stx _test_multi_params__local_0+1
    .loc "test_cc_safety_matrix.c", 96
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
    .loc "test_cc_safety_matrix.c", 36
    lda $22
    ldx $23
    mul.16 .AX, $24
    sta $26
    stx $27
    lda $20
    clc
    adc $26
    sta $22
    lda $21
    adc $26+1
    sta $23
    lda $22
    ldx $23
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end41:
    .loc "test_cc_safety_matrix.c", 96
    lda $22
    ldx $23
    sta _test_multi_params__local_11
    stx _test_multi_params__local_11+1
    .loc "test_cc_safety_matrix.c", 98
    lda _test_multi_params__local_0
    ldx _test_multi_params__local_0+1
    cmp.16 .AX, #14
    bne @if_then43
@or_rhs46:
    lda _test_multi_params__local_11
    ldx _test_multi_params__local_11+1
    cmp.16 .AX, #14
    bne @if_then43
    bra @if_end45
@if_then43:
    .loc "test_cc_safety_matrix.c", 99
    ldax #__str_47
    sta _test_multi_params__local_26
    stx _test_multi_params__local_26+1
    lda _test_multi_params__local_26
    ldx _test_multi_params__local_26+1
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
    .loc "test_cc_safety_matrix.c", 100
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end45:
    .loc "test_cc_safety_matrix.c", 102
    ldax #__str_48
    sta _test_multi_params__local_29
    stx _test_multi_params__local_29+1
    lda _test_multi_params__local_29
    ldx _test_multi_params__local_29+1
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
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _test_mixed_conventions
; SAC inline storage: 6 bytes
    _test_mixed_conventions__local_0: .word 0
    _test_mixed_conventions__local_4: .word 0
    _test_mixed_conventions__local_11: .word 0
    _test_mixed_conventions__local_22: .word 0
    _test_mixed_conventions__local_24: .word 0
    _test_mixed_conventions__local_25: .word 0
    proc _test_mixed_conventions
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_cc_safety_matrix.c", 16
    .local @_l_r = 0
    .local @_l_s = 2
    .local @_l_z = 4
; .debug_var: __test_mixed_conventions @_l_r offset=0 size=2 type=int16 scope=local
; .debug_var: __test_mixed_conventions @_l_s offset=2 size=2 type=int16 scope=local
; .debug_var: __test_mixed_conventions @_l_z offset=4 size=2 type=int16 scope=local

@entry:
    .loc "test_cc_safety_matrix.c", 107
    lda #10
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_cc_safety_matrix.c", 41
    lda #1
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
    .loc "test_cc_safety_matrix.c", 15
    lda $20
    clc
    adc #1
    sta $26
    lda $21
    adc #0
    sta $27
    lda $26
    ldx $27
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $24
    stx $25
@inline_end50:
    .loc "test_cc_safety_matrix.c", 41
    lda $24
    ldx $25
    sta _test_mixed_conventions__local_4
    stx _test_mixed_conventions__local_4+1
    .loc "test_cc_safety_matrix.c", 42
    lda #2
    ldx #0
    sta $20
    stx $21
    lda $22
    ldx $23
    sta $24
    stx $25
    lda $20
    ldx $21
    sta $24
    stx $25
    .loc "test_cc_safety_matrix.c", 28
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
@inline_end52:
    .loc "test_cc_safety_matrix.c", 42
    lda $22
    ldx $23
    sta _test_mixed_conventions__local_11
    stx _test_mixed_conventions__local_11+1
    .loc "test_cc_safety_matrix.c", 43
    lda _test_mixed_conventions__local_11
    ldx _test_mixed_conventions__local_11+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _test_mixed_conventions__local_4
    ldx _test_mixed_conventions__local_4+1
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
@inline_end49:
    .loc "test_cc_safety_matrix.c", 107
    lda $20
    ldx $21
    sta _test_mixed_conventions__local_0
    stx _test_mixed_conventions__local_0+1
    .loc "test_cc_safety_matrix.c", 109
    lda _test_mixed_conventions__local_0
    ldx _test_mixed_conventions__local_0+1
    cmp.16 .AX, #23
    bne @if_then55
    bra @if_end57
@if_then55:
    .loc "test_cc_safety_matrix.c", 110
    ldax #__str_58
    sta _test_mixed_conventions__local_22
    stx _test_mixed_conventions__local_22+1
    lda _test_mixed_conventions__local_22
    ldx _test_mixed_conventions__local_22+1
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
    .loc "test_cc_safety_matrix.c", 111
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end57:
    .loc "test_cc_safety_matrix.c", 113
    ldax #__str_59
    sta _test_mixed_conventions__local_25
    stx _test_mixed_conventions__local_25+1
    lda _test_mixed_conventions__local_25
    ldx _test_mixed_conventions__local_25+1
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
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _stack_modify_global
; SAC inline storage: 2 bytes
    .global _stack_modify_global__param_x
    _stack_modify_global__param_x: .word 0
    _stack_modify_global__local_0: .word 0
    proc _stack_modify_global, W#@_p_x
    .sac
    .var _fp = 0
    .loc "test_cc_safety_matrix.c", 29
    .var @_p_x = 2
; .debug_var: __stack_modify_global @_p_x offset=2 size=2 type=int16 scope=parameter

@entry:
    .loc "test_cc_safety_matrix.c", 120
    lda _global_state
    ldx _global_state+1
    sta $20
    stx $21
    lda _stack_modify_global__param_x
    ldx _stack_modify_global__param_x+1
    add.16 .AX, $20
    sta $22
    stx $23
    lda $22
    ldx $23
    sta _global_state
    stx _global_state+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _zp_modify_global
; SAC inline storage: 2 bytes
    .global _zp_modify_global__param_x
    _zp_modify_global__param_x: .word 0
    _zp_modify_global__local_0: .word 0
    proc _zp_modify_global, W#@_p_x
    .sac
    .var _fp = 0
    .loc "test_cc_safety_matrix.c", 33
    .var @_p_x = 2
; .debug_var: __zp_modify_global @_p_x offset=2 size=2 type=int16 scope=parameter

@entry:
    .loc "test_cc_safety_matrix.c", 124
    lda _global_state
    ldx _global_state+1
    sta $20
    stx $21
    lda _zp_modify_global__param_x
    ldx _zp_modify_global__param_x+1
    add.16 .AX, $20
    sta $22
    stx $23
    lda $22
    ldx $23
    sta _global_state
    stx _global_state+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _test_global_state
; SAC inline storage: 0 bytes
    _test_global_state__local_8: .word 0
    _test_global_state__local_10: .word 0
    _test_global_state__local_18: .word 0
    _test_global_state__local_20: .word 0
    _test_global_state__local_21: .word 0
    proc _test_global_state
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_cc_safety_matrix.c", 37

@entry:
    .loc "test_cc_safety_matrix.c", 128
    lda #100
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _global_state
    stx _global_state+1
    .loc "test_cc_safety_matrix.c", 129
    lda #5
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_cc_safety_matrix.c", 120
    lda _global_state
    ldx _global_state+1
    sta $22
    stx $23
    lda $22
    clc
    adc #5
    sta $24
    lda $23
    adc #0
    sta $25
    lda $24
    ldx $25
    sta _global_state
    stx _global_state+1
@inline_end60:
    .loc "test_cc_safety_matrix.c", 130
    lda _global_state
    ldx _global_state+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #105
    bne @if_then61
    bra @if_end63
@if_then61:
    .loc "test_cc_safety_matrix.c", 131
    ldax #__str_64
    sta _test_global_state__local_8
    stx _test_global_state__local_8+1
    lda _test_global_state__local_8
    ldx _test_global_state__local_8+1
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
    .loc "test_cc_safety_matrix.c", 132
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end63:
    .loc "test_cc_safety_matrix.c", 134
    lda #10
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_cc_safety_matrix.c", 124
    lda _global_state
    ldx _global_state+1
    sta $22
    stx $23
    lda $22
    clc
    adc #10
    sta $24
    lda $23
    adc #0
    sta $25
    lda $24
    ldx $25
    sta _global_state
    stx _global_state+1
@inline_end65:
    .loc "test_cc_safety_matrix.c", 135
    lda _global_state
    ldx _global_state+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #115
    bne @if_then66
    bra @if_end68
@if_then66:
    .loc "test_cc_safety_matrix.c", 136
    ldax #__str_69
    sta _test_global_state__local_18
    stx _test_global_state__local_18+1
    lda _test_global_state__local_18
    ldx _test_global_state__local_18+1
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
    .loc "test_cc_safety_matrix.c", 137
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end68:
    .loc "test_cc_safety_matrix.c", 139
    ldax #__str_70
    sta _test_global_state__local_21
    stx _test_global_state__local_21+1
    lda _test_global_state__local_21
    ldx _test_global_state__local_21+1
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
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 0
    endproc

; function _test_register_clobbering
; SAC inline storage: 6 bytes
    _test_register_clobbering__local_0: .word 0
    _test_register_clobbering__local_2: .word 0
    _test_register_clobbering__local_4: .word 0
    _test_register_clobbering__local_16: .word 0
    _test_register_clobbering__local_18: .word 0
    _test_register_clobbering__local_19: .word 0
    proc _test_register_clobbering
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_cc_safety_matrix.c", 53
    .local @_l_a = 0
    .local @_l_b = 2
    .local @_l_c = 4
; .debug_var: __test_register_clobbering @_l_a offset=0 size=2 type=int16 scope=local
; .debug_var: __test_register_clobbering @_l_b offset=2 size=2 type=int16 scope=local
; .debug_var: __test_register_clobbering @_l_c offset=4 size=2 type=int16 scope=local

@entry:
    .loc "test_cc_safety_matrix.c", 144
    lda #100
    sta _test_register_clobbering__local_0
    lda #0
    sta _test_register_clobbering__local_0+1
    .loc "test_cc_safety_matrix.c", 145
    lda #200
    sta _test_register_clobbering__local_2
    lda #0
    sta _test_register_clobbering__local_2+1
    .loc "test_cc_safety_matrix.c", 148
    lda _test_register_clobbering__local_0
    ldx _test_register_clobbering__local_0+1
    sta $20
    stx $21
    lda _test_register_clobbering__local_2
    ldx _test_register_clobbering__local_2+1
    sta $20
    stx $21
    .loc "test_cc_safety_matrix.c", 28
    lda _test_register_clobbering__local_2
    ldx _test_register_clobbering__local_2+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _test_register_clobbering__local_0
    ldx _test_register_clobbering__local_0+1
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
@inline_end71:
    .loc "test_cc_safety_matrix.c", 148
    lda $20
    ldx $21
    sta _test_register_clobbering__local_4
    stx _test_register_clobbering__local_4+1
    .loc "test_cc_safety_matrix.c", 150
    lda _test_register_clobbering__local_0
    ldx _test_register_clobbering__local_0+1
    cmp.16 .AX, #100
    bne @if_then73
@or_rhs77:
    lda _test_register_clobbering__local_2
    ldx _test_register_clobbering__local_2+1
    cmp.16 .AX, #200
    bne @if_then73
@or_rhs76:
    lda _test_register_clobbering__local_4
    ldx _test_register_clobbering__local_4+1
    cmp.16 .AX, #300
    bne @if_then73
    bra @if_end75
@if_then73:
    .loc "test_cc_safety_matrix.c", 151
    ldax #__str_78
    sta _test_register_clobbering__local_16
    stx _test_register_clobbering__local_16+1
    lda _test_register_clobbering__local_16
    ldx _test_register_clobbering__local_16+1
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
    .loc "test_cc_safety_matrix.c", 152
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end75:
    .loc "test_cc_safety_matrix.c", 154
    ldax #__str_79
    sta _test_register_clobbering__local_19
    stx _test_register_clobbering__local_19+1
    lda _test_register_clobbering__local_19
    ldx _test_register_clobbering__local_19+1
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
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _main
; SAC inline storage: 36 bytes
    _main__local_0: .word 0
    _main__local_11: .word 0
    _main__local_13: .word 0
    _main__local_14: .word 0
    _main__local_25: .word 0
    _main__local_27: .word 0
    _main__local_28: .word 0
    _main__local_41: .word 0
    _main__local_43: .word 0
    _main__local_44: .word 0
    _main__local_46: .long 0
    _main__local_56: .long 0
    _main__local_72: .word 0
    _main__local_74: .word 0
    _main__local_75: .word 0
    _main__local_77: .word 0
    _main__local_87: .word 0
    _main__local_101: .word 0
    _main__local_103: .word 0
    _main__local_104: .word 0
    _main__local_106: .word 0
    _main__local_110: .word 0
    _main__local_116: .word 0
    _main__local_125: .word 0
    _main__local_127: .word 0
    _main__local_128: .word 0
    _main__local_138: .word 0
    _main__local_140: .word 0
    _main__local_148: .word 0
    _main__local_150: .word 0
    _main__local_151: .word 0
    _main__local_153: .word 0
    _main__local_155: .word 0
    _main__local_157: .word 0
    _main__local_168: .word 0
    _main__local_170: .word 0
    _main__local_171: .word 0
    _main__local_173: .word 0
    _main__local_174: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_cc_safety_matrix.c", 68
    .local @_l_a = 22
    .local @_l_b = 24
    .local @_l_c = 26
    .local @_l_r = 16
    .local @_l_s = 18
    .local @_l_s_r = 12
    .local @_l_z = 20
    .local @_l_z_r = 14
; .debug_var: __main @_l_a offset=22 size=2 type=int16 scope=local
; .debug_var: __main @_l_b offset=24 size=2 type=int16 scope=local
; .debug_var: __main @_l_c offset=26 size=2 type=int16 scope=local
; .debug_var: __main @_l_r offset=16 size=2 type=int16 scope=local
; .debug_var: __main @_l_s offset=18 size=2 type=int16 scope=local
; .debug_var: __main @_l_s_r offset=12 size=2 type=int16 scope=local
; .debug_var: __main @_l_z offset=20 size=2 type=int16 scope=local
; .debug_var: __main @_l_z_r offset=14 size=2 type=int16 scope=local

@entry:
    .loc "test_cc_safety_matrix.c", 159
    ldax #__str_80
    sta _main__local_0
    stx _main__local_0+1
    lda _main__local_0
    ldx _main__local_0+1
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
    .loc "test_cc_safety_matrix.c", 48
    lda #5
    ldx #0
    sta $20
    stx $21
    lda #3
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
    .loc "test_cc_safety_matrix.c", 15
    lda $20
    clc
    adc #3
    sta $24
    lda $21
    adc #0
    sta $25
    lda $24
    ldx $25
    sta $20
    stx $21
@inline_end82:
    .loc "test_cc_safety_matrix.c", 48
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_cc_safety_matrix.c", 49
    lda $22
    ldx $23
    cmp.16 .AX, #8
    bne @if_then84
    bra @if_end86
@if_then84:
    .loc "test_cc_safety_matrix.c", 50
    ldax #__str_87
    sta _main__local_11
    stx _main__local_11+1
    lda _main__local_11
    ldx _main__local_11+1
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
    .loc "test_cc_safety_matrix.c", 51
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end86:
    .loc "test_cc_safety_matrix.c", 53
    ldax #__str_88
    sta _main__local_14
    stx _main__local_14+1
    lda _main__local_14
    ldx _main__local_14+1
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
@inline_end81:
    .loc "test_cc_safety_matrix.c", 58
    lda #7
    ldx #0
    sta $20
    stx $21
    lda #2
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
    .loc "test_cc_safety_matrix.c", 28
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
@inline_end90:
    .loc "test_cc_safety_matrix.c", 58
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_cc_safety_matrix.c", 59
    lda $22
    ldx $23
    cmp.16 .AX, #9
    bne @if_then92
    bra @if_end94
@if_then92:
    .loc "test_cc_safety_matrix.c", 60
    ldax #__str_95
    sta _main__local_25
    stx _main__local_25+1
    lda _main__local_25
    ldx _main__local_25+1
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
    .loc "test_cc_safety_matrix.c", 61
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end94:
    .loc "test_cc_safety_matrix.c", 63
    ldax #__str_96
    sta _main__local_28
    stx _main__local_28+1
    lda _main__local_28
    ldx _main__local_28+1
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
@inline_end89:
    .loc "test_cc_safety_matrix.c", 68
    lda #4
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
    .loc "test_cc_safety_matrix.c", 28
    lda $20
    clc
    adc #5
    sta $24
    lda $21
    adc #0
    sta $25
    lda $24
    ldx $25
    sta $20
    stx $21
@inline_end99:
    .loc "test_cc_safety_matrix.c", 68
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_cc_safety_matrix.c", 69
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end98:
    .loc "test_cc_safety_matrix.c", 73
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_cc_safety_matrix.c", 74
    lda $22
    ldx $23
    cmp.16 .AX, #9
    bne @if_then102
    bra @if_end104
@if_then102:
    .loc "test_cc_safety_matrix.c", 75
    ldax #__str_105
    sta _main__local_41
    stx _main__local_41+1
    lda _main__local_41
    ldx _main__local_41+1
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
    .loc "test_cc_safety_matrix.c", 76
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end104:
    .loc "test_cc_safety_matrix.c", 78
    ldax #__str_106
    sta _main__local_44
    stx _main__local_44+1
    lda _main__local_44
    ldx _main__local_44+1
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
@inline_end97:
    .loc "test_cc_safety_matrix.c", 83
    lda #0
    ldx #16
    sta $20
    stx $21
    lda #0
    ldx #32
    sta $22
    stx $23
    lda $20
    ldx $21
    ldy #0
    ldz #0
    sta $24
    stx $25
    sty $26
    stz $27
    lda $22
    ldx $23
    ldy #0
    ldz #0
    sta $28
    stx $29
    sty $2A
    stz $2B
    lda $24
    ldx $25
    ldy $26
    ldz $27
    sta $20
    stx $21
    sty $22
    stz $23
    lda $28
    ldx $29
    ldy $2A
    ldz $2B
    sta $20
    stx $21
    sty $22
    stz $23
    .loc "test_cc_safety_matrix.c", 19
    lda $24
    ldx $25
    ldy $26
    ldz $27
    add.32 .AXYZ, $28
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    sta $24
    stx $25
    lda $24
    ldx $25
    sta $20
    stx $21
    sty $22
    stz $23
@inline_end108:
    .loc "test_cc_safety_matrix.c", 83
    lda $20
    ldx $21
    ldy $22
    ldz $23
    sta _main__local_46
    stx _main__local_46+1
    sty _main__local_46+2
    stz _main__local_46+3
    .loc "test_cc_safety_matrix.c", 84
    lda #0
    ldx #48
    sta $20
    stx $21
    lda #0
    ldx #64
    sta $22
    stx $23
    lda $20
    ldx $21
    ldy #0
    ldz #0
    sta $24
    stx $25
    sty $26
    stz $27
    lda $22
    ldx $23
    ldy #0
    ldz #0
    sta $28
    stx $29
    sty $2A
    stz $2B
    lda $24
    ldx $25
    ldy $26
    ldz $27
    sta $20
    stx $21
    sty $22
    stz $23
    lda $28
    ldx $29
    ldy $2A
    ldz $2B
    sta $20
    stx $21
    sty $22
    stz $23
    .loc "test_cc_safety_matrix.c", 32
    lda $24
    ldx $25
    ldy $26
    ldz $27
    add.32 .AXYZ, $28
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    sta $24
    stx $25
    lda $24
    ldx $25
    sta $20
    stx $21
    sty $22
    stz $23
@inline_end110:
    .loc "test_cc_safety_matrix.c", 84
    lda $20
    ldx $21
    ldy $22
    ldz $23
    sta _main__local_56
    stx _main__local_56+1
    sty _main__local_56+2
    stz _main__local_56+3
    .loc "test_cc_safety_matrix.c", 86
    lda #0
    ldx #48
    sta $20
    stx $21
    lda $20
    ldx $21
    ldy #0
    ldz #0
    sta $22
    stx $23
    sty $24
    stz $25
    lda _main__local_46
    ldx _main__local_46+1
    ldy _main__local_46+2
    ldz _main__local_46+3
    cmp.32 .AXYZ, $22
    bne @if_then112
@or_rhs115:
    lda #0
    ldx #112
    sta $20
    stx $21
    lda $20
    ldx $21
    ldy #0
    ldz #0
    sta $22
    stx $23
    sty $24
    stz $25
    lda _main__local_56
    ldx _main__local_56+1
    ldy _main__local_56+2
    ldz _main__local_56+3
    cmp.32 .AXYZ, $22
    bne @if_then112
    bra @if_end114
@if_then112:
    .loc "test_cc_safety_matrix.c", 87
    ldax #__str_116
    sta _main__local_72
    stx _main__local_72+1
    lda _main__local_72
    ldx _main__local_72+1
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
    .loc "test_cc_safety_matrix.c", 88
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end114:
    .loc "test_cc_safety_matrix.c", 90
    ldax #__str_117
    sta _main__local_75
    stx _main__local_75+1
    lda _main__local_75
    ldx _main__local_75+1
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
@inline_end107:
    .loc "test_cc_safety_matrix.c", 95
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
    .loc "test_cc_safety_matrix.c", 23
    lda $22
    ldx $23
    mul.16 .AX, $24
    sta $26
    stx $27
    lda $20
    clc
    adc $26
    sta $22
    lda $21
    adc $26+1
    sta $23
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end119:
    .loc "test_cc_safety_matrix.c", 95
    lda $20
    ldx $21
    sta _main__local_77
    stx _main__local_77+1
    .loc "test_cc_safety_matrix.c", 96
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
    .loc "test_cc_safety_matrix.c", 36
    lda $22
    ldx $23
    mul.16 .AX, $24
    sta $26
    stx $27
    lda $20
    clc
    adc $26
    sta $22
    lda $21
    adc $26+1
    sta $23
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end121:
    .loc "test_cc_safety_matrix.c", 96
    lda $20
    ldx $21
    sta _main__local_87
    stx _main__local_87+1
    .loc "test_cc_safety_matrix.c", 98
    lda _main__local_77
    ldx _main__local_77+1
    cmp.16 .AX, #14
    bne @if_then123
@or_rhs126:
    lda _main__local_87
    ldx _main__local_87+1
    cmp.16 .AX, #14
    bne @if_then123
    bra @if_end125
@if_then123:
    .loc "test_cc_safety_matrix.c", 99
    ldax #__str_127
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
    .loc "test_cc_safety_matrix.c", 100
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end125:
    .loc "test_cc_safety_matrix.c", 102
    ldax #__str_128
    sta _main__local_104
    stx _main__local_104+1
    lda _main__local_104
    ldx _main__local_104+1
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
@inline_end118:
    .loc "test_cc_safety_matrix.c", 107
    lda #10
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_cc_safety_matrix.c", 41
    lda #1
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
    .loc "test_cc_safety_matrix.c", 15
    lda $20
    clc
    adc #1
    sta $26
    lda $21
    adc #0
    sta $27
    lda $26
    ldx $27
    sta $20
    stx $21
@inline_end131:
    .loc "test_cc_safety_matrix.c", 41
    lda $20
    ldx $21
    sta _main__local_110
    stx _main__local_110+1
    .loc "test_cc_safety_matrix.c", 42
    lda #2
    ldx #0
    sta $20
    stx $21
    lda $22
    ldx $23
    sta $24
    stx $25
    lda $20
    ldx $21
    sta $24
    stx $25
    .loc "test_cc_safety_matrix.c", 28
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
@inline_end133:
    .loc "test_cc_safety_matrix.c", 42
    lda $20
    ldx $21
    sta _main__local_116
    stx _main__local_116+1
    .loc "test_cc_safety_matrix.c", 43
    lda _main__local_116
    ldx _main__local_116+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_110
    ldx _main__local_110+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end130:
    .loc "test_cc_safety_matrix.c", 107
    lda $22
    ldx $23
    sta _main__local_106
    stx _main__local_106+1
    .loc "test_cc_safety_matrix.c", 109
    lda _main__local_106
    ldx _main__local_106+1
    cmp.16 .AX, #23
    bne @if_then136
    bra @if_end138
@if_then136:
    .loc "test_cc_safety_matrix.c", 110
    ldax #__str_139
    sta _main__local_125
    stx _main__local_125+1
    lda _main__local_125
    ldx _main__local_125+1
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
    .loc "test_cc_safety_matrix.c", 111
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end138:
    .loc "test_cc_safety_matrix.c", 113
    ldax #__str_140
    sta _main__local_128
    stx _main__local_128+1
    lda _main__local_128
    ldx _main__local_128+1
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
@inline_end129:
    .loc "test_cc_safety_matrix.c", 128
    lda #100
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _global_state
    stx _global_state+1
    .loc "test_cc_safety_matrix.c", 129
    lda #5
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_cc_safety_matrix.c", 120
    lda _global_state
    ldx _global_state+1
    sta $22
    stx $23
    lda $22
    clc
    adc #5
    sta $24
    lda $23
    adc #0
    sta $25
    lda $24
    ldx $25
    sta _global_state
    stx _global_state+1
@inline_end142:
    .loc "test_cc_safety_matrix.c", 130
    lda _global_state
    ldx _global_state+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #105
    bne @if_then143
    bra @if_end145
@if_then143:
    .loc "test_cc_safety_matrix.c", 131
    ldax #__str_146
    sta _main__local_138
    stx _main__local_138+1
    lda _main__local_138
    ldx _main__local_138+1
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
    .loc "test_cc_safety_matrix.c", 132
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end145:
    .loc "test_cc_safety_matrix.c", 134
    lda #10
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_cc_safety_matrix.c", 124
    lda _global_state
    ldx _global_state+1
    sta $22
    stx $23
    lda $22
    clc
    adc #10
    sta $24
    lda $23
    adc #0
    sta $25
    lda $24
    ldx $25
    sta _global_state
    stx _global_state+1
@inline_end147:
    .loc "test_cc_safety_matrix.c", 135
    lda _global_state
    ldx _global_state+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #115
    bne @if_then148
    bra @if_end150
@if_then148:
    .loc "test_cc_safety_matrix.c", 136
    ldax #__str_151
    sta _main__local_148
    stx _main__local_148+1
    lda _main__local_148
    ldx _main__local_148+1
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
    .loc "test_cc_safety_matrix.c", 137
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end150:
    .loc "test_cc_safety_matrix.c", 139
    ldax #__str_152
    sta _main__local_151
    stx _main__local_151+1
    lda _main__local_151
    ldx _main__local_151+1
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
@inline_end141:
    .loc "test_cc_safety_matrix.c", 144
    lda #100
    sta _main__local_153
    lda #0
    sta _main__local_153+1
    .loc "test_cc_safety_matrix.c", 145
    lda #200
    sta _main__local_155
    lda #0
    sta _main__local_155+1
    .loc "test_cc_safety_matrix.c", 148
    lda _main__local_153
    ldx _main__local_153+1
    sta $20
    stx $21
    lda _main__local_155
    ldx _main__local_155+1
    sta $20
    stx $21
    .loc "test_cc_safety_matrix.c", 28
    lda _main__local_155
    ldx _main__local_155+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_153
    ldx _main__local_153+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end154:
    .loc "test_cc_safety_matrix.c", 148
    lda $22
    ldx $23
    sta _main__local_157
    stx _main__local_157+1
    .loc "test_cc_safety_matrix.c", 150
    lda _main__local_153
    ldx _main__local_153+1
    cmp.16 .AX, #100
    bne @if_then156
@or_rhs160:
    lda _main__local_155
    ldx _main__local_155+1
    cmp.16 .AX, #200
    bne @if_then156
@or_rhs159:
    lda _main__local_157
    ldx _main__local_157+1
    cmp.16 .AX, #300
    bne @if_then156
    bra @if_end158
@if_then156:
    .loc "test_cc_safety_matrix.c", 151
    ldax #__str_161
    sta _main__local_168
    stx _main__local_168+1
    lda _main__local_168
    ldx _main__local_168+1
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
    .loc "test_cc_safety_matrix.c", 152
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end158:
    .loc "test_cc_safety_matrix.c", 154
    ldax #__str_162
    sta _main__local_171
    stx _main__local_171+1
    lda _main__local_171
    ldx _main__local_171+1
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
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 36
    endproc


    .segment "data"
__str_9:
    .text "FAIL: stack_to_stack
"
    .byte 0
__str_10:
    .text "PASS: stack_to_stack
"
    .byte 0
__str_16:
    .text "FAIL: stack_to_zp
"
    .byte 0
__str_17:
    .text "PASS: stack_to_zp
"
    .byte 0
__str_27:
    .text "FAIL: zp_to_zp
"
    .byte 0
__str_28:
    .text "PASS: zp_to_zp
"
    .byte 0
__str_37:
    .text "FAIL: long_returns
"
    .byte 0
__str_38:
    .text "PASS: long_returns
"
    .byte 0
__str_47:
    .text "FAIL: multi_params
"
    .byte 0
__str_48:
    .text "PASS: multi_params
"
    .byte 0
__str_58:
    .text "FAIL: mixed_conventions
"
    .byte 0
__str_59:
    .text "PASS: mixed_conventions
"
    .byte 0
__str_64:
    .text "FAIL: global_state (stack)
"
    .byte 0
__str_69:
    .text "FAIL: global_state (zp)
"
    .byte 0
__str_70:
    .text "PASS: global_state
"
    .byte 0
__str_78:
    .text "FAIL: register_clobbering
"
    .byte 0
__str_79:
    .text "PASS: register_clobbering
"
    .byte 0
__str_80:
    .text "=== Calling Convention Safety Matrix ===
"
    .byte 0
__str_87:
    .text "FAIL: stack_to_stack
"
    .byte 0
__str_88:
    .text "PASS: stack_to_stack
"
    .byte 0
__str_95:
    .text "FAIL: stack_to_zp
"
    .byte 0
__str_96:
    .text "PASS: stack_to_zp
"
    .byte 0
__str_105:
    .text "FAIL: zp_to_zp
"
    .byte 0
__str_106:
    .text "PASS: zp_to_zp
"
    .byte 0
__str_116:
    .text "FAIL: long_returns
"
    .byte 0
__str_117:
    .text "PASS: long_returns
"
    .byte 0
__str_127:
    .text "FAIL: multi_params
"
    .byte 0
__str_128:
    .text "PASS: multi_params
"
    .byte 0
__str_139:
    .text "FAIL: mixed_conventions
"
    .byte 0
__str_140:
    .text "PASS: mixed_conventions
"
    .byte 0
__str_146:
    .text "FAIL: global_state (stack)
"
    .byte 0
__str_151:
    .text "FAIL: global_state (zp)
"
    .byte 0
__str_152:
    .text "PASS: global_state
"
    .byte 0
__str_161:
    .text "FAIL: register_clobbering
"
    .byte 0
__str_162:
    .text "PASS: register_clobbering
"
    .byte 0
__str_163:
    .text "
=== All Calling Convention Tests Passed ===
"
    .byte 0

__zp_save_buf:

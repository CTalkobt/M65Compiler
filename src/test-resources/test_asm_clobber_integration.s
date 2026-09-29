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

    .global _test_x_clobber
    .global _test_a_clobber
    .global _test_y_clobber
    .global _test_multi_register_clobber
    .global _test_asm_variable_access
    .global _test_nested_asm
    .global _test_asm_in_loop
    .global _helper_function
    .global _test_asm_with_call
    .global _test_empty_asm
    .global _test_asm_whitespace
    .global _main

    .segment "code"

; function _test_x_clobber
; SAC inline storage: 4 bytes
    _test_x_clobber__local_0: .word 0
    _test_x_clobber__local_2: .word 0
    _test_x_clobber__local_8: .word 0
    _test_x_clobber__local_10: .word 0
    _test_x_clobber__local_11: .word 0
    proc _test_x_clobber
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdio.h", 5
    .local @_l_x = 0
    .local @_l_y = 2
; .debug_var: __test_x_clobber @_l_x offset=0 size=2 type=int16 scope=local
; .debug_var: __test_x_clobber @_l_y offset=2 size=2 type=int16 scope=local

@entry:
    .loc "test_asm_clobber_integration.c", 17
    lda #100
    sta _test_x_clobber__local_0
    lda #0
    sta _test_x_clobber__local_0+1
    .loc "test_asm_clobber_integration.c", 18
    lda #200
    sta _test_x_clobber__local_2
    lda #0
    sta _test_x_clobber__local_2+1
    .loc "test_asm_clobber_integration.c", 20
    ldx #$42
    .loc "test_asm_clobber_integration.c", 23
    lda _test_x_clobber__local_0
    ldx _test_x_clobber__local_0+1
    cmp.16 .AX, #100
    bne @if_then0
@or_rhs3:
    lda _test_x_clobber__local_2
    ldx _test_x_clobber__local_2+1
    cmp.16 .AX, #200
    bne @if_then0
    bra @if_end2
@if_then0:
    .loc "test_asm_clobber_integration.c", 24
    ldax #__str_4
    sta _test_x_clobber__local_8
    stx _test_x_clobber__local_8+1
    lda _test_x_clobber__local_8
    ldx _test_x_clobber__local_8+1
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
    .loc "test_asm_clobber_integration.c", 25
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end2:
    .loc "test_asm_clobber_integration.c", 27
    ldax #__str_5
    sta _test_x_clobber__local_11
    stx _test_x_clobber__local_11+1
    lda _test_x_clobber__local_11
    ldx _test_x_clobber__local_11+1
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

; function _test_a_clobber
; SAC inline storage: 6 bytes
    _test_a_clobber__local_0: .word 0
    _test_a_clobber__local_2: .word 0
    _test_a_clobber__local_4: .word 0
    _test_a_clobber__local_12: .word 0
    _test_a_clobber__local_14: .word 0
    _test_a_clobber__local_15: .word 0
    proc _test_a_clobber
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdio.h", 20
    .local @_l_a = 0
    .local @_l_b = 2
    .local @_l_c = 4
; .debug_var: __test_a_clobber @_l_a offset=0 size=2 type=int16 scope=local
; .debug_var: __test_a_clobber @_l_b offset=2 size=2 type=int16 scope=local
; .debug_var: __test_a_clobber @_l_c offset=4 size=2 type=int16 scope=local

@entry:
    .loc "test_asm_clobber_integration.c", 32
    lda #5
    sta _test_a_clobber__local_0
    lda #0
    sta _test_a_clobber__local_0+1
    .loc "test_asm_clobber_integration.c", 33
    lda #10
    sta _test_a_clobber__local_2
    lda #0
    sta _test_a_clobber__local_2+1
    .loc "test_asm_clobber_integration.c", 34
    lda #15
    sta _test_a_clobber__local_4
    lda #0
    sta _test_a_clobber__local_4+1
    .loc "test_asm_clobber_integration.c", 36
    lda #$FF
    .loc "test_asm_clobber_integration.c", 38
    lda _test_a_clobber__local_0
    ldx _test_a_clobber__local_0+1
    cmp.16 .AX, #5
    bne @if_then6
@or_rhs10:
    lda _test_a_clobber__local_2
    ldx _test_a_clobber__local_2+1
    cmp.16 .AX, #10
    bne @if_then6
@or_rhs9:
    lda _test_a_clobber__local_4
    ldx _test_a_clobber__local_4+1
    cmp.16 .AX, #15
    bne @if_then6
    bra @if_end8
@if_then6:
    .loc "test_asm_clobber_integration.c", 39
    ldax #__str_11
    sta _test_a_clobber__local_12
    stx _test_a_clobber__local_12+1
    lda _test_a_clobber__local_12
    ldx _test_a_clobber__local_12+1
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
    .loc "test_asm_clobber_integration.c", 40
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
    .loc "test_asm_clobber_integration.c", 42
    ldax #__str_12
    sta _test_a_clobber__local_15
    stx _test_a_clobber__local_15+1
    lda _test_a_clobber__local_15
    ldx _test_a_clobber__local_15+1
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

; function _test_y_clobber
; SAC inline storage: 4 bytes
    _test_y_clobber__local_0: .word 0
    _test_y_clobber__local_2: .word 0
    _test_y_clobber__local_8: .word 0
    _test_y_clobber__local_10: .word 0
    _test_y_clobber__local_11: .word 0
    proc _test_y_clobber
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stddef.h", 9
    .local @_l_y_var = 0
    .local @_l_z_var = 2
; .debug_var: __test_y_clobber @_l_y_var offset=0 size=2 type=int16 scope=local
; .debug_var: __test_y_clobber @_l_z_var offset=2 size=2 type=int16 scope=local

@entry:
    .loc "test_asm_clobber_integration.c", 47
    lda #42
    sta _test_y_clobber__local_0
    lda #0
    sta _test_y_clobber__local_0+1
    .loc "test_asm_clobber_integration.c", 48
    lda #84
    sta _test_y_clobber__local_2
    lda #0
    sta _test_y_clobber__local_2+1
    .loc "test_asm_clobber_integration.c", 50
    ldy #$30
    .loc "test_asm_clobber_integration.c", 52
    lda _test_y_clobber__local_0
    ldx _test_y_clobber__local_0+1
    cmp.16 .AX, #42
    bne @if_then13
@or_rhs16:
    lda _test_y_clobber__local_2
    ldx _test_y_clobber__local_2+1
    cmp.16 .AX, #84
    bne @if_then13
    bra @if_end15
@if_then13:
    .loc "test_asm_clobber_integration.c", 53
    ldax #__str_17
    sta _test_y_clobber__local_8
    stx _test_y_clobber__local_8+1
    lda _test_y_clobber__local_8
    ldx _test_y_clobber__local_8+1
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
    .loc "test_asm_clobber_integration.c", 54
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
    .loc "test_asm_clobber_integration.c", 56
    ldax #__str_18
    sta _test_y_clobber__local_11
    stx _test_y_clobber__local_11+1
    lda _test_y_clobber__local_11
    ldx _test_y_clobber__local_11+1
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

; function _test_multi_register_clobber
; SAC inline storage: 8 bytes
    _test_multi_register_clobber__local_0: .word 0
    _test_multi_register_clobber__local_2: .word 0
    _test_multi_register_clobber__local_4: .word 0
    _test_multi_register_clobber__local_6: .word 0
    _test_multi_register_clobber__local_16: .word 0
    _test_multi_register_clobber__local_18: .word 0
    _test_multi_register_clobber__local_19: .word 0
    proc _test_multi_register_clobber
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdlib.h", 15
    .local @_l_a = 0
    .local @_l_b = 2
    .local @_l_c = 4
    .local @_l_d = 6
; .debug_var: __test_multi_register_clobber @_l_a offset=0 size=2 type=int16 scope=local
; .debug_var: __test_multi_register_clobber @_l_b offset=2 size=2 type=int16 scope=local
; .debug_var: __test_multi_register_clobber @_l_c offset=4 size=2 type=int16 scope=local
; .debug_var: __test_multi_register_clobber @_l_d offset=6 size=2 type=int16 scope=local

@entry:
    .loc "test_asm_clobber_integration.c", 61
    lda #1
    sta _test_multi_register_clobber__local_0
    lda #0
    sta _test_multi_register_clobber__local_0+1
    .loc "test_asm_clobber_integration.c", 62
    lda #2
    sta _test_multi_register_clobber__local_2
    lda #0
    sta _test_multi_register_clobber__local_2+1
    .loc "test_asm_clobber_integration.c", 63
    lda #3
    sta _test_multi_register_clobber__local_4
    lda #0
    sta _test_multi_register_clobber__local_4+1
    .loc "test_asm_clobber_integration.c", 64
    lda #4
    sta _test_multi_register_clobber__local_6
    lda #0
    sta _test_multi_register_clobber__local_6+1
    .loc "test_asm_clobber_integration.c", 66
    lda #$11; ldx #$22; ldy #$33
    .loc "test_asm_clobber_integration.c", 68
    lda _test_multi_register_clobber__local_0
    ldx _test_multi_register_clobber__local_0+1
    cmp.16 .AX, #1
    bne @if_then19
@or_rhs24:
    lda _test_multi_register_clobber__local_2
    ldx _test_multi_register_clobber__local_2+1
    cmp.16 .AX, #2
    bne @if_then19
@or_rhs23:
    lda _test_multi_register_clobber__local_4
    ldx _test_multi_register_clobber__local_4+1
    cmp.16 .AX, #3
    bne @if_then19
@or_rhs22:
    lda _test_multi_register_clobber__local_6
    ldx _test_multi_register_clobber__local_6+1
    cmp.16 .AX, #4
    bne @if_then19
    bra @if_end21
@if_then19:
    .loc "test_asm_clobber_integration.c", 69
    ldax #__str_25
    sta _test_multi_register_clobber__local_16
    stx _test_multi_register_clobber__local_16+1
    lda _test_multi_register_clobber__local_16
    ldx _test_multi_register_clobber__local_16+1
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
    .loc "test_asm_clobber_integration.c", 70
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end21:
    .loc "test_asm_clobber_integration.c", 72
    ldax #__str_26
    sta _test_multi_register_clobber__local_19
    stx _test_multi_register_clobber__local_19+1
    lda _test_multi_register_clobber__local_19
    ldx _test_multi_register_clobber__local_19+1
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

; function _test_asm_variable_access
; SAC inline storage: 4 bytes
    _test_asm_variable_access__local_0: .word 0
    _test_asm_variable_access__local_2: .word 0
    _test_asm_variable_access__local_10: .word 0
    _test_asm_variable_access__local_12: .word 0
    _test_asm_variable_access__local_13: .word 0
    proc _test_asm_variable_access
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdlib.h", 31
    .local @_l_counter = 0
    .local @_l_limit = 2
; .debug_var: __test_asm_variable_access @_l_counter offset=0 size=2 type=int16 scope=local
; .debug_var: __test_asm_variable_access @_l_limit offset=2 size=2 type=int16 scope=local

@entry:
    .loc "test_asm_clobber_integration.c", 77
    lda #0
    sta _test_asm_variable_access__local_0
    sta _test_asm_variable_access__local_0+1
    .loc "test_asm_clobber_integration.c", 78
    lda #5
    sta _test_asm_variable_access__local_2
    lda #0
    sta _test_asm_variable_access__local_2+1
    .loc "test_asm_clobber_integration.c", 81
    ldx #$00
    .loc "test_asm_clobber_integration.c", 83
    lda _test_asm_variable_access__local_0
    ldx _test_asm_variable_access__local_0+1
    sta $20
    stx $21
    lda $20
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta _test_asm_variable_access__local_0
    stx _test_asm_variable_access__local_0+1
    .loc "test_asm_clobber_integration.c", 85
    lda _test_asm_variable_access__local_0
    ldx _test_asm_variable_access__local_0+1
    cmp.16 .AX, #1
    bne @if_then27
@or_rhs30:
    lda _test_asm_variable_access__local_2
    ldx _test_asm_variable_access__local_2+1
    cmp.16 .AX, #5
    bne @if_then27
    bra @if_end29
@if_then27:
    .loc "test_asm_clobber_integration.c", 86
    ldax #__str_31
    sta _test_asm_variable_access__local_10
    stx _test_asm_variable_access__local_10+1
    lda _test_asm_variable_access__local_10
    ldx _test_asm_variable_access__local_10+1
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
    .loc "test_asm_clobber_integration.c", 87
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end29:
    .loc "test_asm_clobber_integration.c", 89
    ldax #__str_32
    sta _test_asm_variable_access__local_13
    stx _test_asm_variable_access__local_13+1
    lda _test_asm_variable_access__local_13
    ldx _test_asm_variable_access__local_13+1
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

; function _test_nested_asm
; SAC inline storage: 2 bytes
    _test_nested_asm__local_0: .word 0
    _test_nested_asm__local_8: .word 0
    _test_nested_asm__local_10: .word 0
    _test_nested_asm__local_11: .word 0
    proc _test_nested_asm
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdlib.h", 48
    .local @_l_value = 0
; .debug_var: __test_nested_asm @_l_value offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_asm_clobber_integration.c", 94
    lda #123
    sta _test_nested_asm__local_0
    lda #0
    sta _test_nested_asm__local_0+1
    .loc "test_asm_clobber_integration.c", 96
    lda #$01
    .loc "test_asm_clobber_integration.c", 97
    inc.16f __vr0
    .loc "test_asm_clobber_integration.c", 98
    ldx #$02
    .loc "test_asm_clobber_integration.c", 99
    lda #2
    ldx #0
    sta $20
    stx $21
    lda _test_nested_asm__local_0
    ldx _test_nested_asm__local_0+1
    lsl.16 .AX
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _test_nested_asm__local_0
    stx _test_nested_asm__local_0+1
    .loc "test_asm_clobber_integration.c", 101
    lda _test_nested_asm__local_0
    ldx _test_nested_asm__local_0+1
    cmp.16 .AX, #248
    bne @if_then33
    bra @if_end35
@if_then33:
    .loc "test_asm_clobber_integration.c", 102
    ldax #__str_36
    sta _test_nested_asm__local_8
    stx _test_nested_asm__local_8+1
    lda _test_nested_asm__local_8
    ldx _test_nested_asm__local_8+1
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
    .loc "test_asm_clobber_integration.c", 103
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
    .loc "test_asm_clobber_integration.c", 105
    ldax #__str_37
    sta _test_nested_asm__local_11
    stx _test_nested_asm__local_11+1
    lda _test_nested_asm__local_11
    ldx _test_nested_asm__local_11+1
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

; function _test_asm_in_loop
; SAC inline storage: 2 bytes
    _test_asm_in_loop__local_0: .word 0
    _test_asm_in_loop__local_9: .word 0
    _test_asm_in_loop__local_11: .word 0
    _test_asm_in_loop__local_12: .word 0
    proc _test_asm_in_loop
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_asm_clobber_integration.c", 19
    .local @_l_result = 0
; .debug_var: __test_asm_in_loop @_l_result offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_asm_clobber_integration.c", 110
    lda #0
    sta _test_asm_in_loop__local_0
    sta _test_asm_in_loop__local_0+1
    lda #$00
    lda #0
    sta _test_asm_in_loop__local_0
    sta _test_asm_in_loop__local_0+1
    lda #$00
    lda #1
    sta _test_asm_in_loop__local_0
    lda #0
    sta _test_asm_in_loop__local_0+1
    lda #$00
    lda #2
    sta _test_asm_in_loop__local_0
    lda #0
    sta _test_asm_in_loop__local_0+1
    lda #$00
    lda #3
    sta _test_asm_in_loop__local_0
    lda #0
    sta _test_asm_in_loop__local_0+1
    lda #$00
    lda #4
    sta _test_asm_in_loop__local_0
    lda #0
    sta _test_asm_in_loop__local_0+1
    .loc "test_asm_clobber_integration.c", 117
    lda _test_asm_in_loop__local_0
    ldx _test_asm_in_loop__local_0+1
    cmp.16 .AX, #10
    bne @if_then38
    bra @if_end40
@if_then38:
    .loc "test_asm_clobber_integration.c", 118
    ldax #__str_41
    sta _test_asm_in_loop__local_9
    stx _test_asm_in_loop__local_9+1
    lda _test_asm_in_loop__local_9
    ldx _test_asm_in_loop__local_9+1
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
    .loc "test_asm_clobber_integration.c", 119
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end40:
    .loc "test_asm_clobber_integration.c", 121
    ldax #__str_42
    sta _test_asm_in_loop__local_12
    stx _test_asm_in_loop__local_12+1
    lda _test_asm_in_loop__local_12
    ldx _test_asm_in_loop__local_12+1
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

; function _helper_function
; SAC zero-alloc leaf: no storage overhead
    proc _helper_function
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_asm_clobber_integration.c", 35

@entry:
@__return:
    rts
    .func_flags stack_call, static_alloc, zeroalloc, leaf
    .frame_size 0
    endproc

; function _test_asm_with_call
; SAC inline storage: 4 bytes
    _test_asm_with_call__local_0: .word 0
    _test_asm_with_call__local_2: .word 0
    _test_asm_with_call__local_9: .word 0
    _test_asm_with_call__local_11: .word 0
    _test_asm_with_call__local_12: .word 0
    proc _test_asm_with_call
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_asm_clobber_integration.c", 39
    .local @_l_x = 0
    .local @_l_y = 2
; .debug_var: __test_asm_with_call @_l_x offset=0 size=2 type=int16 scope=local
; .debug_var: __test_asm_with_call @_l_y offset=2 size=2 type=int16 scope=local

@entry:
    .loc "test_asm_clobber_integration.c", 130
    lda #50
    sta _test_asm_with_call__local_0
    lda #0
    sta _test_asm_with_call__local_0+1
    .loc "test_asm_clobber_integration.c", 132
    lda #$FF
@inline_end43:
    .loc "test_asm_clobber_integration.c", 134
    lda _test_asm_with_call__local_0
    ldx _test_asm_with_call__local_0+1
    add.16 .AX, #1
    sta $22
    stx $23
    sta _test_asm_with_call__local_2
    stx _test_asm_with_call__local_2+1
    .loc "test_asm_clobber_integration.c", 136
    lda _test_asm_with_call__local_0
    ldx _test_asm_with_call__local_0+1
    cmp.16 .AX, #50
    bne @if_then44
@or_rhs47:
    lda _test_asm_with_call__local_2
    ldx _test_asm_with_call__local_2+1
    cmp.16 .AX, #51
    bne @if_then44
    bra @if_end46
@if_then44:
    .loc "test_asm_clobber_integration.c", 137
    ldax #__str_48
    sta _test_asm_with_call__local_9
    stx _test_asm_with_call__local_9+1
    lda _test_asm_with_call__local_9
    ldx _test_asm_with_call__local_9+1
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
    .loc "test_asm_clobber_integration.c", 138
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end46:
    .loc "test_asm_clobber_integration.c", 140
    ldax #__str_49
    sta _test_asm_with_call__local_12
    stx _test_asm_with_call__local_12+1
    lda _test_asm_with_call__local_12
    ldx _test_asm_with_call__local_12+1
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

; function _test_empty_asm
; SAC inline storage: 2 bytes
    _test_empty_asm__local_0: .word 0
    _test_empty_asm__local_4: .word 0
    _test_empty_asm__local_6: .word 0
    _test_empty_asm__local_7: .word 0
    proc _test_empty_asm
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_asm_clobber_integration.c", 54
    .local @_l_x = 0
; .debug_var: __test_empty_asm @_l_x offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_asm_clobber_integration.c", 145
    lda #99
    sta _test_empty_asm__local_0
    lda #0
    sta _test_empty_asm__local_0+1
    .loc "test_asm_clobber_integration.c", 146
    
    .loc "test_asm_clobber_integration.c", 147
    lda _test_empty_asm__local_0
    ldx _test_empty_asm__local_0+1
    cmp.16 .AX, #99
    bne @if_then50
    bra @if_end52
@if_then50:
    .loc "test_asm_clobber_integration.c", 148
    ldax #__str_53
    sta _test_empty_asm__local_4
    stx _test_empty_asm__local_4+1
    lda _test_empty_asm__local_4
    ldx _test_empty_asm__local_4+1
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
    .loc "test_asm_clobber_integration.c", 149
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end52:
    .loc "test_asm_clobber_integration.c", 151
    ldax #__str_54
    sta _test_empty_asm__local_7
    stx _test_empty_asm__local_7+1
    lda _test_empty_asm__local_7
    ldx _test_empty_asm__local_7+1
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

; function _test_asm_whitespace
; SAC inline storage: 2 bytes
    _test_asm_whitespace__local_0: .word 0
    _test_asm_whitespace__local_4: .word 0
    _test_asm_whitespace__local_6: .word 0
    _test_asm_whitespace__local_7: .word 0
    proc _test_asm_whitespace
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_asm_clobber_integration.c", 65
    .local @_l_value = 0
; .debug_var: __test_asm_whitespace @_l_value offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_asm_clobber_integration.c", 156
    lda #77
    sta _test_asm_whitespace__local_0
    lda #0
    sta _test_asm_whitespace__local_0+1
    .loc "test_asm_clobber_integration.c", 158
       lda #$55   
    .loc "test_asm_clobber_integration.c", 159
    	ldy #$66	
    .loc "test_asm_clobber_integration.c", 161
    lda _test_asm_whitespace__local_0
    ldx _test_asm_whitespace__local_0+1
    cmp.16 .AX, #77
    bne @if_then55
    bra @if_end57
@if_then55:
    .loc "test_asm_clobber_integration.c", 162
    ldax #__str_58
    sta _test_asm_whitespace__local_4
    stx _test_asm_whitespace__local_4+1
    lda _test_asm_whitespace__local_4
    ldx _test_asm_whitespace__local_4+1
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
    .loc "test_asm_clobber_integration.c", 163
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
    .loc "test_asm_clobber_integration.c", 165
    ldax #__str_59
    sta _test_asm_whitespace__local_7
    stx _test_asm_whitespace__local_7+1
    lda _test_asm_whitespace__local_7
    ldx _test_asm_whitespace__local_7+1
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

; function _main
; SAC inline storage: 38 bytes
    _main__local_0: .word 0
    _main__local_10: .word 0
    _main__local_12: .word 0
    _main__local_13: .word 0
    _main__local_27: .word 0
    _main__local_29: .word 0
    _main__local_30: .word 0
    _main__local_32: .word 0
    _main__local_34: .word 0
    _main__local_40: .word 0
    _main__local_42: .word 0
    _main__local_43: .word 0
    _main__local_45: .word 0
    _main__local_47: .word 0
    _main__local_49: .word 0
    _main__local_51: .word 0
    _main__local_61: .word 0
    _main__local_63: .word 0
    _main__local_64: .word 0
    _main__local_66: .word 0
    _main__local_68: .word 0
    _main__local_76: .word 0
    _main__local_78: .word 0
    _main__local_79: .word 0
    _main__local_89: .word 0
    _main__local_91: .word 0
    _main__local_92: .word 0
    _main__local_94: .word 0
    _main__local_103: .word 0
    _main__local_105: .word 0
    _main__local_106: .word 0
    _main__local_110: .word 0
    _main__local_117: .word 0
    _main__local_119: .word 0
    _main__local_120: .word 0
    _main__local_122: .word 0
    _main__local_126: .word 0
    _main__local_128: .word 0
    _main__local_129: .word 0
    _main__local_131: .word 0
    _main__local_135: .word 0
    _main__local_137: .word 0
    _main__local_138: .word 0
    _main__local_140: .word 0
    _main__local_141: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_asm_clobber_integration.c", 78
    .local @_l_a = 14
    .local @_l_b = 16
    .local @_l_c = 18
    .local @_l_counter = 22
    .local @_l_d = 20
    .local @_l_limit = 24
    .local @_l_result = 28
    .local @_l_value = 36
    .local @_l_x = 34
    .local @_l_y = 32
    .local @_l_y_var = 10
    .local @_l_z_var = 12
; .debug_var: __main @_l_a offset=14 size=2 type=int16 scope=local
; .debug_var: __main @_l_b offset=16 size=2 type=int16 scope=local
; .debug_var: __main @_l_c offset=18 size=2 type=int16 scope=local
; .debug_var: __main @_l_counter offset=22 size=2 type=int16 scope=local
; .debug_var: __main @_l_d offset=20 size=2 type=int16 scope=local
; .debug_var: __main @_l_limit offset=24 size=2 type=int16 scope=local
; .debug_var: __main @_l_result offset=28 size=2 type=int16 scope=local
; .debug_var: __main @_l_value offset=36 size=2 type=int16 scope=local
; .debug_var: __main @_l_x offset=34 size=2 type=int16 scope=local
; .debug_var: __main @_l_y offset=32 size=2 type=int16 scope=local
; .debug_var: __main @_l_y_var offset=10 size=2 type=int16 scope=local
; .debug_var: __main @_l_z_var offset=12 size=2 type=int16 scope=local

@entry:
    .loc "test_asm_clobber_integration.c", 169
    ldax #__str_60
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
    .loc "test_asm_clobber_integration.c", 17
    lda #100
    sta $22
    ldx #0
    stx $23
    .loc "test_asm_clobber_integration.c", 18
    lda #200
    sta $24
    ldx #0
    stx $25
    .loc "test_asm_clobber_integration.c", 20
    ldx #$42
    .loc "test_asm_clobber_integration.c", 23
    lda $22
    ldx $23
    cmp.16 .AX, #100
    bne @if_then62
@or_rhs65:
    lda $24
    ldx $25
    cmp.16 .AX, #200
    bne @if_then62
    bra @if_end64
@if_then62:
    .loc "test_asm_clobber_integration.c", 24
    ldax #__str_66
    sta _main__local_10
    stx _main__local_10+1
    lda _main__local_10
    ldx _main__local_10+1
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
    .loc "test_asm_clobber_integration.c", 25
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end64:
    .loc "test_asm_clobber_integration.c", 27
    ldax #__str_67
    sta _main__local_13
    stx _main__local_13+1
    lda _main__local_13
    ldx _main__local_13+1
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
@inline_end61:
    .loc "test_asm_clobber_integration.c", 32
    lda #5
    sta $22
    ldx #0
    stx $23
    .loc "test_asm_clobber_integration.c", 33
    lda #10
    sta $24
    ldx #0
    stx $25
    .loc "test_asm_clobber_integration.c", 34
    lda #15
    sta $26
    ldx #0
    stx $27
    .loc "test_asm_clobber_integration.c", 36
    lda #$FF
    .loc "test_asm_clobber_integration.c", 38
    lda $22
    ldx $23
    cmp.16 .AX, #5
    bne @if_then69
@or_rhs73:
    lda $24
    ldx $25
    cmp.16 .AX, #10
    bne @if_then69
@or_rhs72:
    lda $26
    ldx $27
    cmp.16 .AX, #15
    bne @if_then69
    bra @if_end71
@if_then69:
    .loc "test_asm_clobber_integration.c", 39
    ldax #__str_74
    sta _main__local_27
    stx _main__local_27+1
    lda _main__local_27
    ldx _main__local_27+1
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
    .loc "test_asm_clobber_integration.c", 40
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end71:
    .loc "test_asm_clobber_integration.c", 42
    ldax #__str_75
    sta _main__local_30
    stx _main__local_30+1
    lda _main__local_30
    ldx _main__local_30+1
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
@inline_end68:
    .loc "test_asm_clobber_integration.c", 47
    lda #42
    sta _main__local_32
    lda #0
    sta _main__local_32+1
    .loc "test_asm_clobber_integration.c", 48
    lda #84
    sta _main__local_34
    lda #0
    sta _main__local_34+1
    .loc "test_asm_clobber_integration.c", 50
    ldy #$30
    .loc "test_asm_clobber_integration.c", 52
    lda _main__local_32
    ldx _main__local_32+1
    cmp.16 .AX, #42
    bne @if_then77
@or_rhs80:
    lda _main__local_34
    ldx _main__local_34+1
    cmp.16 .AX, #84
    bne @if_then77
    bra @if_end79
@if_then77:
    .loc "test_asm_clobber_integration.c", 53
    ldax #__str_81
    sta _main__local_40
    stx _main__local_40+1
    lda _main__local_40
    ldx _main__local_40+1
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
    .loc "test_asm_clobber_integration.c", 54
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end79:
    .loc "test_asm_clobber_integration.c", 56
    ldax #__str_82
    sta _main__local_43
    stx _main__local_43+1
    lda _main__local_43
    ldx _main__local_43+1
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
@inline_end76:
    .loc "test_asm_clobber_integration.c", 61
    lda #1
    sta _main__local_45
    lda #0
    sta _main__local_45+1
    .loc "test_asm_clobber_integration.c", 62
    lda #2
    sta _main__local_47
    lda #0
    sta _main__local_47+1
    .loc "test_asm_clobber_integration.c", 63
    lda #3
    sta _main__local_49
    lda #0
    sta _main__local_49+1
    .loc "test_asm_clobber_integration.c", 64
    lda #4
    sta _main__local_51
    lda #0
    sta _main__local_51+1
    .loc "test_asm_clobber_integration.c", 66
    lda #$11; ldx #$22; ldy #$33
    .loc "test_asm_clobber_integration.c", 68
    lda _main__local_45
    ldx _main__local_45+1
    cmp.16 .AX, #1
    bne @if_then84
@or_rhs89:
    lda _main__local_47
    ldx _main__local_47+1
    cmp.16 .AX, #2
    bne @if_then84
@or_rhs88:
    lda _main__local_49
    ldx _main__local_49+1
    cmp.16 .AX, #3
    bne @if_then84
@or_rhs87:
    lda _main__local_51
    ldx _main__local_51+1
    cmp.16 .AX, #4
    bne @if_then84
    bra @if_end86
@if_then84:
    .loc "test_asm_clobber_integration.c", 69
    ldax #__str_90
    sta _main__local_61
    stx _main__local_61+1
    lda _main__local_61
    ldx _main__local_61+1
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
    .loc "test_asm_clobber_integration.c", 70
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
    .loc "test_asm_clobber_integration.c", 72
    ldax #__str_91
    sta _main__local_64
    stx _main__local_64+1
    lda _main__local_64
    ldx _main__local_64+1
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
@inline_end83:
    .loc "test_asm_clobber_integration.c", 77
    lda #0
    sta _main__local_66
    sta _main__local_66+1
    .loc "test_asm_clobber_integration.c", 78
    lda #5
    sta _main__local_68
    lda #0
    sta _main__local_68+1
    .loc "test_asm_clobber_integration.c", 81
    ldx #$00
    .loc "test_asm_clobber_integration.c", 83
    lda _main__local_66
    ldx _main__local_66+1
    sta $20
    stx $21
    lda $20
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta _main__local_66
    stx _main__local_66+1
    .loc "test_asm_clobber_integration.c", 85
    lda _main__local_66
    ldx _main__local_66+1
    cmp.16 .AX, #1
    bne @if_then93
@or_rhs96:
    lda _main__local_68
    ldx _main__local_68+1
    cmp.16 .AX, #5
    bne @if_then93
    bra @if_end95
@if_then93:
    .loc "test_asm_clobber_integration.c", 86
    ldax #__str_97
    sta _main__local_76
    stx _main__local_76+1
    lda _main__local_76
    ldx _main__local_76+1
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
    .loc "test_asm_clobber_integration.c", 87
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end95:
    .loc "test_asm_clobber_integration.c", 89
    ldax #__str_98
    sta _main__local_79
    stx _main__local_79+1
    lda _main__local_79
    ldx _main__local_79+1
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
@inline_end92:
    .loc "test_asm_clobber_integration.c", 94
    lda #123
    sta $22
    ldx #0
    stx $23
    .loc "test_asm_clobber_integration.c", 96
    lda #$01
    .loc "test_asm_clobber_integration.c", 97
    inc $22
    bne *+4
    inc $23
    .loc "test_asm_clobber_integration.c", 98
    ldx #$02
    .loc "test_asm_clobber_integration.c", 99
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
    sta $22
    stx $23
    .loc "test_asm_clobber_integration.c", 101
    lda $22
    ldx $23
    cmp.16 .AX, #248
    bne @if_then100
    bra @if_end102
@if_then100:
    .loc "test_asm_clobber_integration.c", 102
    ldax #__str_103
    sta _main__local_89
    stx _main__local_89+1
    lda _main__local_89
    ldx _main__local_89+1
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
    .loc "test_asm_clobber_integration.c", 103
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end102:
    .loc "test_asm_clobber_integration.c", 105
    ldax #__str_104
    sta _main__local_92
    stx _main__local_92+1
    lda _main__local_92
    ldx _main__local_92+1
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
@inline_end99:
    .loc "test_asm_clobber_integration.c", 110
    lda #0
    sta _main__local_94
    sta _main__local_94+1
    lda #$00
    lda #0
    sta _main__local_94
    sta _main__local_94+1
    lda #$00
    lda #1
    sta _main__local_94
    lda #0
    sta _main__local_94+1
    lda #$00
    lda #2
    sta _main__local_94
    lda #0
    sta _main__local_94+1
    lda #$00
    lda #3
    sta _main__local_94
    lda #0
    sta _main__local_94+1
    lda #$00
    lda #4
    sta _main__local_94
    lda #0
    sta _main__local_94+1
    .loc "test_asm_clobber_integration.c", 117
    lda _main__local_94
    ldx _main__local_94+1
    cmp.16 .AX, #10
    bne @if_then106
    bra @if_end108
@if_then106:
    .loc "test_asm_clobber_integration.c", 118
    ldax #__str_109
    sta _main__local_103
    stx _main__local_103+1
    lda _main__local_103
    ldx _main__local_103+1
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
    .loc "test_asm_clobber_integration.c", 119
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end108:
    .loc "test_asm_clobber_integration.c", 121
    ldax #__str_110
    sta _main__local_106
    stx _main__local_106+1
    lda _main__local_106
    ldx _main__local_106+1
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
@inline_end105:
    .loc "test_asm_clobber_integration.c", 130
    lda #50
    sta $22
    ldx #0
    stx $23
    .loc "test_asm_clobber_integration.c", 132
    lda #$FF
@inline_end112:
    .loc "test_asm_clobber_integration.c", 134
    lda $22
    clc
    adc #1
    sta $24
    lda $23
    adc #0
    sta $25
    lda $24
    ldx $25
    sta _main__local_110
    stx _main__local_110+1
    .loc "test_asm_clobber_integration.c", 136
    lda $22
    ldx $23
    cmp.16 .AX, #50
    bne @if_then113
@or_rhs116:
    lda _main__local_110
    ldx _main__local_110+1
    cmp.16 .AX, #51
    bne @if_then113
    bra @if_end115
@if_then113:
    .loc "test_asm_clobber_integration.c", 137
    ldax #__str_117
    sta _main__local_117
    stx _main__local_117+1
    lda _main__local_117
    ldx _main__local_117+1
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
    .loc "test_asm_clobber_integration.c", 138
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end115:
    .loc "test_asm_clobber_integration.c", 140
    ldax #__str_118
    sta _main__local_120
    stx _main__local_120+1
    lda _main__local_120
    ldx _main__local_120+1
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
@inline_end111:
    .loc "test_asm_clobber_integration.c", 145
    lda #99
    sta _main__local_122
    lda #0
    sta _main__local_122+1
    .loc "test_asm_clobber_integration.c", 146
    
    .loc "test_asm_clobber_integration.c", 147
    lda _main__local_122
    ldx _main__local_122+1
    cmp.16 .AX, #99
    bne @if_then120
    bra @if_end122
@if_then120:
    .loc "test_asm_clobber_integration.c", 148
    ldax #__str_123
    sta _main__local_126
    stx _main__local_126+1
    lda _main__local_126
    ldx _main__local_126+1
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
    .loc "test_asm_clobber_integration.c", 149
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end122:
    .loc "test_asm_clobber_integration.c", 151
    ldax #__str_124
    sta _main__local_129
    stx _main__local_129+1
    lda _main__local_129
    ldx _main__local_129+1
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
@inline_end119:
    .loc "test_asm_clobber_integration.c", 156
    lda #77
    sta _main__local_131
    lda #0
    sta _main__local_131+1
    .loc "test_asm_clobber_integration.c", 158
       lda #$55   
    .loc "test_asm_clobber_integration.c", 159
    	ldy #$66	
    .loc "test_asm_clobber_integration.c", 161
    lda _main__local_131
    ldx _main__local_131+1
    cmp.16 .AX, #77
    bne @if_then126
    bra @if_end128
@if_then126:
    .loc "test_asm_clobber_integration.c", 162
    ldax #__str_129
    sta _main__local_135
    stx _main__local_135+1
    lda _main__local_135
    ldx _main__local_135+1
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
    .loc "test_asm_clobber_integration.c", 163
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end128:
    .loc "test_asm_clobber_integration.c", 165
    ldax #__str_130
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
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 38
    endproc


    .segment "data"
__str_4:
    .text "FAIL: x_clobber
"
    .byte 0
__str_5:
    .text "PASS: x_clobber
"
    .byte 0
__str_11:
    .text "FAIL: a_clobber
"
    .byte 0
__str_12:
    .text "PASS: a_clobber
"
    .byte 0
__str_17:
    .text "FAIL: y_clobber
"
    .byte 0
__str_18:
    .text "PASS: y_clobber
"
    .byte 0
__str_25:
    .text "FAIL: multi_register_clobber
"
    .byte 0
__str_26:
    .text "PASS: multi_register_clobber
"
    .byte 0
__str_31:
    .text "FAIL: asm_variable_access
"
    .byte 0
__str_32:
    .text "PASS: asm_variable_access
"
    .byte 0
__str_36:
    .text "FAIL: nested_asm
"
    .byte 0
__str_37:
    .text "PASS: nested_asm
"
    .byte 0
__str_41:
    .text "FAIL: asm_in_loop
"
    .byte 0
__str_42:
    .text "PASS: asm_in_loop
"
    .byte 0
__str_48:
    .text "FAIL: asm_with_call
"
    .byte 0
__str_49:
    .text "PASS: asm_with_call
"
    .byte 0
__str_53:
    .text "FAIL: empty_asm
"
    .byte 0
__str_54:
    .text "PASS: empty_asm
"
    .byte 0
__str_58:
    .text "FAIL: asm_whitespace
"
    .byte 0
__str_59:
    .text "PASS: asm_whitespace
"
    .byte 0
__str_60:
    .text "=== Inline Assembly Clobber Integration Tests ===
"
    .byte 0
__str_66:
    .text "FAIL: x_clobber
"
    .byte 0
__str_67:
    .text "PASS: x_clobber
"
    .byte 0
__str_74:
    .text "FAIL: a_clobber
"
    .byte 0
__str_75:
    .text "PASS: a_clobber
"
    .byte 0
__str_81:
    .text "FAIL: y_clobber
"
    .byte 0
__str_82:
    .text "PASS: y_clobber
"
    .byte 0
__str_90:
    .text "FAIL: multi_register_clobber
"
    .byte 0
__str_91:
    .text "PASS: multi_register_clobber
"
    .byte 0
__str_97:
    .text "FAIL: asm_variable_access
"
    .byte 0
__str_98:
    .text "PASS: asm_variable_access
"
    .byte 0
__str_103:
    .text "FAIL: nested_asm
"
    .byte 0
__str_104:
    .text "PASS: nested_asm
"
    .byte 0
__str_109:
    .text "FAIL: asm_in_loop
"
    .byte 0
__str_110:
    .text "PASS: asm_in_loop
"
    .byte 0
__str_117:
    .text "FAIL: asm_with_call
"
    .byte 0
__str_118:
    .text "PASS: asm_with_call
"
    .byte 0
__str_123:
    .text "FAIL: empty_asm
"
    .byte 0
__str_124:
    .text "PASS: empty_asm
"
    .byte 0
__str_129:
    .text "FAIL: asm_whitespace
"
    .byte 0
__str_130:
    .text "PASS: asm_whitespace
"
    .byte 0
__str_131:
    .text "
=== All Clobber Tracking Tests Passed ===
"
    .byte 0

__zp_save_buf:

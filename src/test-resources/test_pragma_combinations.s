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
    .extern _free
    .extern _malloc
    .extern _printf

    .global _test1_string
    .global _test2_string
    .global _test4_string
    .global _test6_string
    .global _early_string
    .global _late_string
    .global _test_single_pragma
    .global _test_pragma_reset
    .global _test_heap_with_encoding
    .global _test_multiple_sequential_pragmas
    .global _function_with_petscii_strings
    .global _function_with_ascii_strings
    .global _test_scope_changes
    .global _test_pragma_override
    .global _test_different_pragma_types
    .global _func1_with_screencode
    .global _func2_still_screencode
    .global _test_pragma_persistence
    .global _test_pragma_timing
    .global _test_local_vars_unaffected
    .global _main

    .segment "data"
    .byte 0
_test1_string:
; .debug_var: @global _test1_string offset=0 size=2 type=int8 scope=global
    .word __str_0
_test2_string:
; .debug_var: @global _test2_string offset=0 size=2 type=int8 scope=global
    .word __str_7
_heap_buffer:
; .debug_var: @global _heap_buffer offset=0 size=2 type=ptr scope=global
    .word 0
_test4_string:
; .debug_var: @global _test4_string offset=0 size=2 type=int8 scope=global
    .word __str_18
_global_data:
; .debug_var: @global _global_data offset=0 size=2 type=int16 scope=global
    .word 42
_test6_string:
; .debug_var: @global _test6_string offset=0 size=2 type=int8 scope=global
    .word __str_47
_bss_var1:
; .debug_var: @global _bss_var1 offset=0 size=2 type=int16 scope=global
    .word 0

    .segment "bss"
_bss_var2:
; .debug_var: @global _bss_var2 offset=0 size=2 type=int16 scope=global
    .res 2
    .segment "data"
_early_string:
; .debug_var: @global _early_string offset=0 size=2 type=int8 scope=global
    .word __str_82
_late_string:
; .debug_var: @global _late_string offset=0 size=2 type=int8 scope=global
    .word __str_83

    .segment "code"

; function _test_single_pragma
; SAC inline storage: 0 bytes
    _test_single_pragma__local_14: .word 0
    _test_single_pragma__local_16: .word 0
    _test_single_pragma__local_17: .word 0
    proc _test_single_pragma
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdio.h", 12

@entry:
    .loc "test_pragma_combinations.c", 26
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_test1_string
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
    ldx #0
    sta $24
    lda #116
    sta $20
    lda $24
    ldx #0
    sxt.8
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
    bne @if_then1
@or_rhs4:
    lda #4
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_test1_string
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
    ldx #0
    sta $24
    lda #49
    sta $20
    lda $24
    ldx #0
    sxt.8
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
    bne @if_then1
    bra @if_end3
@if_then1:
    .loc "test_pragma_combinations.c", 27
    ldax #__str_5
    sta _test_single_pragma__local_14
    stx _test_single_pragma__local_14+1
    lda _test_single_pragma__local_14
    ldx _test_single_pragma__local_14+1
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
    .loc "test_pragma_combinations.c", 28
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end3:
    .loc "test_pragma_combinations.c", 30
    ldax #__str_6
    sta _test_single_pragma__local_17
    stx _test_single_pragma__local_17+1
    lda _test_single_pragma__local_17
    ldx _test_single_pragma__local_17+1
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

; function _test_pragma_reset
; SAC inline storage: 0 bytes
    _test_pragma_reset__local_7: .word 0
    _test_pragma_reset__local_9: .word 0
    _test_pragma_reset__local_10: .word 0
    proc _test_pragma_reset
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stddef.h", 3

@entry:
    .loc "test_pragma_combinations.c", 43
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_test2_string
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
    ldx #0
    sta $24
    lda #116
    sta $20
    lda $24
    ldx #0
    sxt.8
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
    beq @if_then8
    bra @if_end10
@if_then8:
    .loc "test_pragma_combinations.c", 44
    ldax #__str_11
    sta _test_pragma_reset__local_7
    stx _test_pragma_reset__local_7+1
    lda _test_pragma_reset__local_7
    ldx _test_pragma_reset__local_7+1
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
    .loc "test_pragma_combinations.c", 45
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end10:
    .loc "test_pragma_combinations.c", 47
    ldax #__str_12
    sta _test_pragma_reset__local_10
    stx _test_pragma_reset__local_10+1
    lda _test_pragma_reset__local_10
    ldx _test_pragma_reset__local_10+1
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

; function _test_heap_with_encoding
; SAC inline storage: 0 bytes
    _test_heap_with_encoding__local_0: .word 0
    _test_heap_with_encoding__local_5: .word 0
    _test_heap_with_encoding__local_7: .word 0
    _test_heap_with_encoding__local_8: .word 0
    _test_heap_with_encoding__local_9: .word 0
    proc _test_heap_with_encoding
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdlib.h", 12

@entry:
    .loc "test_pragma_combinations.c", 60
    lda #64
    ldx #0
    push .ax
    jsr _malloc
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _heap_buffer
    stx _heap_buffer+1
    .loc "test_pragma_combinations.c", 61
    lda _heap_buffer
    ldx _heap_buffer+1
    sta $20
    stx $21
    lda $20
    ora $21
    beq @if_then13
    bra @if_end15
@if_then13:
    .loc "test_pragma_combinations.c", 62
    ldax #__str_16
    sta _test_heap_with_encoding__local_5
    stx _test_heap_with_encoding__local_5+1
    lda _test_heap_with_encoding__local_5
    ldx _test_heap_with_encoding__local_5+1
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
    .loc "test_pragma_combinations.c", 63
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
    .loc "test_pragma_combinations.c", 65
    lda _heap_buffer
    ldx _heap_buffer+1
    sta _test_heap_with_encoding__local_8
    stx _test_heap_with_encoding__local_8+1
    lda _test_heap_with_encoding__local_8
    ldx _test_heap_with_encoding__local_8+1
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    jsr _free
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    .loc "test_pragma_combinations.c", 66
    ldax #__str_17
    sta _test_heap_with_encoding__local_9
    stx _test_heap_with_encoding__local_9+1
    lda _test_heap_with_encoding__local_9
    ldx _test_heap_with_encoding__local_9+1
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

; function _test_multiple_sequential_pragmas
; SAC inline storage: 0 bytes
    _test_multiple_sequential_pragmas__local_3: .word 0
    _test_multiple_sequential_pragmas__local_5: .word 0
    _test_multiple_sequential_pragmas__local_6: .word 0
    proc _test_multiple_sequential_pragmas
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdlib.h", 33

@entry:
    .loc "test_pragma_combinations.c", 80
    lda _global_data
    ldx _global_data+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #42
    bne @if_then19
    bra @if_end21
@if_then19:
    .loc "test_pragma_combinations.c", 81
    ldax #__str_22
    sta _test_multiple_sequential_pragmas__local_3
    stx _test_multiple_sequential_pragmas__local_3+1
    lda _test_multiple_sequential_pragmas__local_3
    ldx _test_multiple_sequential_pragmas__local_3+1
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
    .loc "test_pragma_combinations.c", 82
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
    .loc "test_pragma_combinations.c", 84
    ldax #__str_23
    sta _test_multiple_sequential_pragmas__local_6
    stx _test_multiple_sequential_pragmas__local_6+1
    lda _test_multiple_sequential_pragmas__local_6
    ldx _test_multiple_sequential_pragmas__local_6+1
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

; function _function_with_petscii_strings
; SAC inline storage: 2 bytes
    _function_with_petscii_strings__local_0: .word 0
    _function_with_petscii_strings__local_4: .word 0
    _function_with_petscii_strings__local_6: .word 0
    proc _function_with_petscii_strings
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdlib.h", 47
    .local @_l_s = 0
; .debug_var: __function_with_petscii_strings @_l_s offset=0 size=2 type=ptr scope=local

@entry:
    .loc "test_pragma_combinations.c", 94
    ldax #__str_24
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _function_with_petscii_strings__local_0
    stx _function_with_petscii_strings__local_0+1
    .loc "test_pragma_combinations.c", 95
    lda _function_with_petscii_strings__local_0
    ldx _function_with_petscii_strings__local_0+1
    stx __zp_scratch
    ora __zp_scratch
    beq @if_then25
    bra @if_end27
@if_then25:
    .loc "test_pragma_combinations.c", 96
    ldax #__str_28
    sta _function_with_petscii_strings__local_4
    stx _function_with_petscii_strings__local_4+1
    lda _function_with_petscii_strings__local_4
    ldx _function_with_petscii_strings__local_4+1
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
    .loc "test_pragma_combinations.c", 97
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end27:
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _function_with_ascii_strings
; SAC inline storage: 2 bytes
    _function_with_ascii_strings__local_0: .word 0
    _function_with_ascii_strings__local_4: .word 0
    _function_with_ascii_strings__local_6: .word 0
    proc _function_with_ascii_strings
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdlib.h", 57
    .local @_l_s = 0
; .debug_var: __function_with_ascii_strings @_l_s offset=0 size=2 type=ptr scope=local

@entry:
    .loc "test_pragma_combinations.c", 104
    ldax #__str_29
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _function_with_ascii_strings__local_0
    stx _function_with_ascii_strings__local_0+1
    .loc "test_pragma_combinations.c", 105
    lda _function_with_ascii_strings__local_0
    ldx _function_with_ascii_strings__local_0+1
    stx __zp_scratch
    ora __zp_scratch
    beq @if_then30
    bra @if_end32
@if_then30:
    .loc "test_pragma_combinations.c", 106
    ldax #__str_33
    sta _function_with_ascii_strings__local_4
    stx _function_with_ascii_strings__local_4+1
    lda _function_with_ascii_strings__local_4
    ldx _function_with_ascii_strings__local_4+1
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
    .loc "test_pragma_combinations.c", 107
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end32:
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _test_scope_changes
; SAC inline storage: 4 bytes
    _test_scope_changes__local_4: .word 0
    _test_scope_changes__local_6: .word 0
    _test_scope_changes__local_7: .word 0
    _test_scope_changes__local_11: .word 0
    _test_scope_changes__local_13: .word 0
    _test_scope_changes__local_14: .word 0
    _test_scope_changes__local_15: .word 0
    proc _test_scope_changes
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_pragma_combinations.c", 21
    .local @_l_s = 2
; .debug_var: __test_scope_changes @_l_s offset=2 size=2 type=ptr scope=local

@entry:
    .loc "test_pragma_combinations.c", 94
    ldax #__str_35
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_pragma_combinations.c", 95
    lda $22
    ora $23
    beq @if_then36
    bra @if_end38
@if_then36:
    .loc "test_pragma_combinations.c", 96
    ldax #__str_39
    sta _test_scope_changes__local_4
    stx _test_scope_changes__local_4+1
    lda _test_scope_changes__local_4
    ldx _test_scope_changes__local_4+1
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
    .loc "test_pragma_combinations.c", 97
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end38:
@inline_end34:
    .loc "test_pragma_combinations.c", 104
    ldax #__str_41
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _test_scope_changes__local_7
    stx _test_scope_changes__local_7+1
    .loc "test_pragma_combinations.c", 105
    lda _test_scope_changes__local_7
    ldx _test_scope_changes__local_7+1
    stx __zp_scratch
    ora __zp_scratch
    beq @if_then42
    bra @if_end44
@if_then42:
    .loc "test_pragma_combinations.c", 106
    ldax #__str_45
    sta _test_scope_changes__local_11
    stx _test_scope_changes__local_11+1
    lda _test_scope_changes__local_11
    ldx _test_scope_changes__local_11+1
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
    .loc "test_pragma_combinations.c", 107
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end44:
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _test_pragma_override
; SAC inline storage: 0 bytes
    _test_pragma_override__local_2: .word 0
    _test_pragma_override__local_4: .word 0
    _test_pragma_override__local_5: .word 0
    proc _test_pragma_override
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_pragma_combinations.c", 36

@entry:
    .loc "test_pragma_combinations.c", 127
    ldax #_test6_string
    stx __zp_scratch
    ora __zp_scratch
    beq @if_then48
    bra @if_end50
@if_then48:
    .loc "test_pragma_combinations.c", 128
    ldax #__str_51
    sta _test_pragma_override__local_2
    stx _test_pragma_override__local_2+1
    lda _test_pragma_override__local_2
    ldx _test_pragma_override__local_2+1
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
    .loc "test_pragma_combinations.c", 129
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end50:
    .loc "test_pragma_combinations.c", 131
    ldax #__str_52
    sta _test_pragma_override__local_5
    stx _test_pragma_override__local_5+1
    lda _test_pragma_override__local_5
    ldx _test_pragma_override__local_5+1
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

; function _test_different_pragma_types
; SAC inline storage: 0 bytes
    _test_different_pragma_types__local_8: .word 0
    _test_different_pragma_types__local_10: .word 0
    _test_different_pragma_types__local_11: .word 0
    proc _test_different_pragma_types
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_pragma_combinations.c", 54

@entry:
    .loc "test_pragma_combinations.c", 145
    lda #100
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _bss_var1
    stx _bss_var1+1
    .loc "test_pragma_combinations.c", 146
    lda #200
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _bss_var2
    stx _bss_var2+1
    .loc "test_pragma_combinations.c", 148
    lda _bss_var1
    ldx _bss_var1+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #100
    bne @if_then53
@or_rhs56:
    lda _bss_var2
    ldx _bss_var2+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #200
    bne @if_then53
    bra @if_end55
@if_then53:
    .loc "test_pragma_combinations.c", 149
    ldax #__str_57
    sta _test_different_pragma_types__local_8
    stx _test_different_pragma_types__local_8+1
    lda _test_different_pragma_types__local_8
    ldx _test_different_pragma_types__local_8+1
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
    .loc "test_pragma_combinations.c", 150
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end55:
    .loc "test_pragma_combinations.c", 152
    ldax #__str_58
    sta _test_different_pragma_types__local_11
    stx _test_different_pragma_types__local_11+1
    lda _test_different_pragma_types__local_11
    ldx _test_different_pragma_types__local_11+1
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

; function _func1_with_screencode
; SAC inline storage: 2 bytes
    _func1_with_screencode__local_0: .word 0
    _func1_with_screencode__local_4: .word 0
    _func1_with_screencode__local_6: .word 0
    proc _func1_with_screencode
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_pragma_combinations.c", 71
    .local @_l_s = 0
; .debug_var: __func1_with_screencode @_l_s offset=0 size=2 type=ptr scope=local

@entry:
    .loc "test_pragma_combinations.c", 162
    ldax #__str_59
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _func1_with_screencode__local_0
    stx _func1_with_screencode__local_0+1
    .loc "test_pragma_combinations.c", 163
    lda _func1_with_screencode__local_0
    ldx _func1_with_screencode__local_0+1
    stx __zp_scratch
    ora __zp_scratch
    beq @if_then60
    bra @if_end62
@if_then60:
    .loc "test_pragma_combinations.c", 164
    ldax #__str_63
    sta _func1_with_screencode__local_4
    stx _func1_with_screencode__local_4+1
    lda _func1_with_screencode__local_4
    ldx _func1_with_screencode__local_4+1
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
    .loc "test_pragma_combinations.c", 165
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end62:
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _func2_still_screencode
; SAC inline storage: 2 bytes
    _func2_still_screencode__local_0: .word 0
    _func2_still_screencode__local_4: .word 0
    _func2_still_screencode__local_6: .word 0
    proc _func2_still_screencode
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_pragma_combinations.c", 80
    .local @_l_s = 0
; .debug_var: __func2_still_screencode @_l_s offset=0 size=2 type=ptr scope=local

@entry:
    .loc "test_pragma_combinations.c", 171
    ldax #__str_64
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _func2_still_screencode__local_0
    stx _func2_still_screencode__local_0+1
    .loc "test_pragma_combinations.c", 172
    lda _func2_still_screencode__local_0
    ldx _func2_still_screencode__local_0+1
    stx __zp_scratch
    ora __zp_scratch
    beq @if_then65
    bra @if_end67
@if_then65:
    .loc "test_pragma_combinations.c", 173
    ldax #__str_68
    sta _func2_still_screencode__local_4
    stx _func2_still_screencode__local_4+1
    lda _func2_still_screencode__local_4
    ldx _func2_still_screencode__local_4+1
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
    .loc "test_pragma_combinations.c", 174
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end67:
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _test_pragma_persistence
; SAC inline storage: 4 bytes
    _test_pragma_persistence__local_4: .word 0
    _test_pragma_persistence__local_6: .word 0
    _test_pragma_persistence__local_7: .word 0
    _test_pragma_persistence__local_11: .word 0
    _test_pragma_persistence__local_13: .word 0
    _test_pragma_persistence__local_14: .word 0
    _test_pragma_persistence__local_15: .word 0
    proc _test_pragma_persistence
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_pragma_combinations.c", 88
    .local @_l_s = 2
; .debug_var: __test_pragma_persistence @_l_s offset=2 size=2 type=ptr scope=local

@entry:
    .loc "test_pragma_combinations.c", 162
    ldax #__str_70
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_pragma_combinations.c", 163
    lda $22
    ora $23
    beq @if_then71
    bra @if_end73
@if_then71:
    .loc "test_pragma_combinations.c", 164
    ldax #__str_74
    sta _test_pragma_persistence__local_4
    stx _test_pragma_persistence__local_4+1
    lda _test_pragma_persistence__local_4
    ldx _test_pragma_persistence__local_4+1
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
    .loc "test_pragma_combinations.c", 165
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end73:
@inline_end69:
    .loc "test_pragma_combinations.c", 171
    ldax #__str_76
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _test_pragma_persistence__local_7
    stx _test_pragma_persistence__local_7+1
    .loc "test_pragma_combinations.c", 172
    lda _test_pragma_persistence__local_7
    ldx _test_pragma_persistence__local_7+1
    stx __zp_scratch
    ora __zp_scratch
    beq @if_then77
    bra @if_end79
@if_then77:
    .loc "test_pragma_combinations.c", 173
    ldax #__str_80
    sta _test_pragma_persistence__local_11
    stx _test_pragma_persistence__local_11+1
    lda _test_pragma_persistence__local_11
    ldx _test_pragma_persistence__local_11+1
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
    .loc "test_pragma_combinations.c", 174
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
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _test_pragma_timing
; SAC inline storage: 0 bytes
    _test_pragma_timing__local_14: .word 0
    _test_pragma_timing__local_16: .word 0
    _test_pragma_timing__local_17: .word 0
    proc _test_pragma_timing
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_pragma_combinations.c", 104

@entry:
    .loc "test_pragma_combinations.c", 196
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_early_string
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
    ldx #0
    sta $24
    lda #0
    sta $20
    lda $24
    ldx #0
    sxt.8
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
    beq @if_then84
@or_rhs87:
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_late_string
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
    ldx #0
    sta $24
    lda #0
    sta $20
    lda $24
    ldx #0
    sxt.8
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
    beq @if_then84
    bra @if_end86
@if_then84:
    .loc "test_pragma_combinations.c", 197
    ldax #__str_88
    sta _test_pragma_timing__local_14
    stx _test_pragma_timing__local_14+1
    lda _test_pragma_timing__local_14
    ldx _test_pragma_timing__local_14+1
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
    .loc "test_pragma_combinations.c", 198
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
    .loc "test_pragma_combinations.c", 200
    ldax #__str_89
    sta _test_pragma_timing__local_17
    stx _test_pragma_timing__local_17+1
    lda _test_pragma_timing__local_17
    ldx _test_pragma_timing__local_17+1
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

; function _test_local_vars_unaffected
; SAC inline storage: 6 bytes
    _test_local_vars_unaffected__local_0: .word 0
    _test_local_vars_unaffected__local_2: .word 0
    _test_local_vars_unaffected__local_4: .word 0
    _test_local_vars_unaffected__local_8: .word 0
    _test_local_vars_unaffected__local_10: .word 0
    _test_local_vars_unaffected__local_11: .word 0
    proc _test_local_vars_unaffected
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_pragma_combinations.c", 119
    .local @_l_local_a = 0
    .local @_l_local_b = 2
    .local @_l_local_c = 4
; .debug_var: __test_local_vars_unaffected @_l_local_a offset=0 size=2 type=int16 scope=local
; .debug_var: __test_local_vars_unaffected @_l_local_b offset=2 size=2 type=int16 scope=local
; .debug_var: __test_local_vars_unaffected @_l_local_c offset=4 size=2 type=int16 scope=local

@entry:
    .loc "test_pragma_combinations.c", 210
    lda #10
    sta _test_local_vars_unaffected__local_0
    lda #0
    sta _test_local_vars_unaffected__local_0+1
    .loc "test_pragma_combinations.c", 211
    lda #20
    sta _test_local_vars_unaffected__local_2
    lda #0
    sta _test_local_vars_unaffected__local_2+1
    .loc "test_pragma_combinations.c", 212
    lda _test_local_vars_unaffected__local_2
    ldx _test_local_vars_unaffected__local_2+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _test_local_vars_unaffected__local_0
    ldx _test_local_vars_unaffected__local_0+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    sta _test_local_vars_unaffected__local_4
    stx _test_local_vars_unaffected__local_4+1
    .loc "test_pragma_combinations.c", 214
    lda _test_local_vars_unaffected__local_4
    ldx _test_local_vars_unaffected__local_4+1
    cmp.16 .AX, #30
    bne @if_then90
    bra @if_end92
@if_then90:
    .loc "test_pragma_combinations.c", 215
    ldax #__str_93
    sta _test_local_vars_unaffected__local_8
    stx _test_local_vars_unaffected__local_8+1
    lda _test_local_vars_unaffected__local_8
    ldx _test_local_vars_unaffected__local_8+1
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
    .loc "test_pragma_combinations.c", 216
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end92:
    .loc "test_pragma_combinations.c", 218
    ldax #__str_94
    sta _test_local_vars_unaffected__local_11
    stx _test_local_vars_unaffected__local_11+1
    lda _test_local_vars_unaffected__local_11
    ldx _test_local_vars_unaffected__local_11+1
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
; SAC inline storage: 14 bytes
    _main__local_0: .word 0
    _main__local_16: .word 0
    _main__local_18: .word 0
    _main__local_19: .word 0
    _main__local_28: .word 0
    _main__local_30: .word 0
    _main__local_31: .word 0
    _main__local_33: .word 0
    _main__local_38: .word 0
    _main__local_40: .word 0
    _main__local_41: .word 0
    _main__local_42: .word 0
    _main__local_47: .word 0
    _main__local_49: .word 0
    _main__local_50: .word 0
    _main__local_56: .word 0
    _main__local_58: .word 0
    _main__local_63: .word 0
    _main__local_65: .word 0
    _main__local_66: .word 0
    _main__local_70: .word 0
    _main__local_72: .word 0
    _main__local_73: .word 0
    _main__local_83: .word 0
    _main__local_85: .word 0
    _main__local_86: .word 0
    _main__local_92: .word 0
    _main__local_94: .word 0
    _main__local_95: .word 0
    _main__local_99: .word 0
    _main__local_101: .word 0
    _main__local_102: .word 0
    _main__local_118: .word 0
    _main__local_120: .word 0
    _main__local_121: .word 0
    _main__local_123: .word 0
    _main__local_125: .word 0
    _main__local_127: .word 0
    _main__local_131: .word 0
    _main__local_133: .word 0
    _main__local_134: .word 0
    _main__local_136: .word 0
    _main__local_137: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_pragma_combinations.c", 135
    .local @_l_local_a = 8
    .local @_l_local_b = 10
    .local @_l_local_c = 12
    .local @_l_s = 6
; .debug_var: __main @_l_local_a offset=8 size=2 type=int16 scope=local
; .debug_var: __main @_l_local_b offset=10 size=2 type=int16 scope=local
; .debug_var: __main @_l_local_c offset=12 size=2 type=int16 scope=local
; .debug_var: __main @_l_s offset=6 size=2 type=ptr scope=local

@entry:
    .loc "test_pragma_combinations.c", 226
    ldax #__str_95
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
    .loc "test_pragma_combinations.c", 26
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_test1_string
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
    ldx #0
    sta $24
    lda #116
    sta $20
    lda $24
    ldx #0
    sxt.8
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
    bne @if_then97
@or_rhs100:
    lda #4
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_test1_string
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
    ldx #0
    sta $24
    lda #49
    sta $20
    lda $24
    ldx #0
    sxt.8
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
    bne @if_then97
    bra @if_end99
@if_then97:
    .loc "test_pragma_combinations.c", 27
    ldax #__str_101
    sta _main__local_16
    stx _main__local_16+1
    lda _main__local_16
    ldx _main__local_16+1
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
    .loc "test_pragma_combinations.c", 28
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end99:
    .loc "test_pragma_combinations.c", 30
    ldax #__str_102
    sta _main__local_19
    stx _main__local_19+1
    lda _main__local_19
    ldx _main__local_19+1
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
@inline_end96:
    .loc "test_pragma_combinations.c", 43
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_test2_string
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
    ldx #0
    sta $24
    lda #116
    sta $20
    lda $24
    ldx #0
    sxt.8
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
    beq @if_then104
    bra @if_end106
@if_then104:
    .loc "test_pragma_combinations.c", 44
    ldax #__str_107
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
    .loc "test_pragma_combinations.c", 45
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end106:
    .loc "test_pragma_combinations.c", 47
    ldax #__str_108
    sta _main__local_31
    stx _main__local_31+1
    lda _main__local_31
    ldx _main__local_31+1
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
@inline_end103:
    .loc "test_pragma_combinations.c", 60
    lda #64
    ldx #0
    push .ax
    jsr _malloc
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _heap_buffer
    stx _heap_buffer+1
    .loc "test_pragma_combinations.c", 61
    lda _heap_buffer
    ldx _heap_buffer+1
    sta $20
    stx $21
    lda $20
    ora $21
    beq @if_then110
    bra @if_end112
@if_then110:
    .loc "test_pragma_combinations.c", 62
    ldax #__str_113
    sta _main__local_38
    stx _main__local_38+1
    lda _main__local_38
    ldx _main__local_38+1
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
    .loc "test_pragma_combinations.c", 63
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end112:
    .loc "test_pragma_combinations.c", 65
    lda _heap_buffer
    ldx _heap_buffer+1
    sta _main__local_41
    stx _main__local_41+1
    lda _main__local_41
    ldx _main__local_41+1
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    jsr _free
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    .loc "test_pragma_combinations.c", 66
    ldax #__str_114
    sta _main__local_42
    stx _main__local_42+1
    lda _main__local_42
    ldx _main__local_42+1
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
@inline_end109:
    .loc "test_pragma_combinations.c", 80
    lda _global_data
    ldx _global_data+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #42
    bne @if_then116
    bra @if_end118
@if_then116:
    .loc "test_pragma_combinations.c", 81
    ldax #__str_119
    sta _main__local_47
    stx _main__local_47+1
    lda _main__local_47
    ldx _main__local_47+1
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
    .loc "test_pragma_combinations.c", 82
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end118:
    .loc "test_pragma_combinations.c", 84
    ldax #__str_120
    sta _main__local_50
    stx _main__local_50+1
    lda _main__local_50
    ldx _main__local_50+1
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
@inline_end115:
    .loc "test_pragma_combinations.c", 94
    ldax #__str_123
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_pragma_combinations.c", 95
    lda $22
    ora $23
    beq @if_then124
    bra @if_end126
@if_then124:
    .loc "test_pragma_combinations.c", 96
    ldax #__str_127
    sta _main__local_56
    stx _main__local_56+1
    lda _main__local_56
    ldx _main__local_56+1
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
    .loc "test_pragma_combinations.c", 97
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end126:
@inline_end122:
    .loc "test_pragma_combinations.c", 104
    ldax #__str_129
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_pragma_combinations.c", 105
    lda $22
    ora $23
    beq @if_then130
    bra @if_end132
@if_then130:
    .loc "test_pragma_combinations.c", 106
    ldax #__str_133
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
    .loc "test_pragma_combinations.c", 107
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end132:
@inline_end128:
    .loc "test_pragma_combinations.c", 114
    ldax #__str_134
    sta _main__local_66
    stx _main__local_66+1
    lda _main__local_66
    ldx _main__local_66+1
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
@inline_end121:
    .loc "test_pragma_combinations.c", 127
    ldax #_test6_string
    stx __zp_scratch
    ora __zp_scratch
    beq @if_then136
    bra @if_end138
@if_then136:
    .loc "test_pragma_combinations.c", 128
    ldax #__str_139
    sta _main__local_70
    stx _main__local_70+1
    lda _main__local_70
    ldx _main__local_70+1
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
    .loc "test_pragma_combinations.c", 129
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
    .loc "test_pragma_combinations.c", 131
    ldax #__str_140
    sta _main__local_73
    stx _main__local_73+1
    lda _main__local_73
    ldx _main__local_73+1
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
@inline_end135:
    .loc "test_pragma_combinations.c", 145
    lda #100
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _bss_var1
    stx _bss_var1+1
    .loc "test_pragma_combinations.c", 146
    lda #200
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _bss_var2
    stx _bss_var2+1
    .loc "test_pragma_combinations.c", 148
    lda _bss_var1
    ldx _bss_var1+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #100
    bne @if_then142
@or_rhs145:
    lda _bss_var2
    ldx _bss_var2+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #200
    bne @if_then142
    bra @if_end144
@if_then142:
    .loc "test_pragma_combinations.c", 149
    ldax #__str_146
    sta _main__local_83
    stx _main__local_83+1
    lda _main__local_83
    ldx _main__local_83+1
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
    .loc "test_pragma_combinations.c", 150
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end144:
    .loc "test_pragma_combinations.c", 152
    ldax #__str_147
    sta _main__local_86
    stx _main__local_86+1
    lda _main__local_86
    ldx _main__local_86+1
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
    .loc "test_pragma_combinations.c", 162
    ldax #__str_150
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_pragma_combinations.c", 163
    lda $22
    ora $23
    beq @if_then151
    bra @if_end153
@if_then151:
    .loc "test_pragma_combinations.c", 164
    ldax #__str_154
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
    .loc "test_pragma_combinations.c", 165
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end153:
@inline_end149:
    .loc "test_pragma_combinations.c", 171
    ldax #__str_156
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_95
    stx _main__local_95+1
    .loc "test_pragma_combinations.c", 172
    lda _main__local_95
    ldx _main__local_95+1
    stx __zp_scratch
    ora __zp_scratch
    beq @if_then157
    bra @if_end159
@if_then157:
    .loc "test_pragma_combinations.c", 173
    ldax #__str_160
    sta _main__local_99
    stx _main__local_99+1
    lda _main__local_99
    ldx _main__local_99+1
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
    .loc "test_pragma_combinations.c", 174
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end159:
@inline_end155:
    .loc "test_pragma_combinations.c", 181
    ldax #__str_161
    sta _main__local_102
    stx _main__local_102+1
    lda _main__local_102
    ldx _main__local_102+1
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
@inline_end148:
    .loc "test_pragma_combinations.c", 196
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_early_string
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
    ldx #0
    sta $24
    lda #0
    sta $20
    lda $24
    ldx #0
    sxt.8
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
    beq @if_then163
@or_rhs166:
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_late_string
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
    ldx #0
    sta $24
    lda #0
    sta $20
    lda $24
    ldx #0
    sxt.8
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
    beq @if_then163
    bra @if_end165
@if_then163:
    .loc "test_pragma_combinations.c", 197
    ldax #__str_167
    sta _main__local_118
    stx _main__local_118+1
    lda _main__local_118
    ldx _main__local_118+1
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
    .loc "test_pragma_combinations.c", 198
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end165:
    .loc "test_pragma_combinations.c", 200
    ldax #__str_168
    sta _main__local_121
    stx _main__local_121+1
    lda _main__local_121
    ldx _main__local_121+1
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
@inline_end162:
    .loc "test_pragma_combinations.c", 210
    lda #10
    sta _main__local_123
    lda #0
    sta _main__local_123+1
    .loc "test_pragma_combinations.c", 211
    lda #20
    sta _main__local_125
    lda #0
    sta _main__local_125+1
    .loc "test_pragma_combinations.c", 212
    lda _main__local_125
    ldx _main__local_125+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_123
    ldx _main__local_123+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    sta _main__local_127
    stx _main__local_127+1
    .loc "test_pragma_combinations.c", 214
    lda _main__local_127
    ldx _main__local_127+1
    cmp.16 .AX, #30
    bne @if_then170
    bra @if_end172
@if_then170:
    .loc "test_pragma_combinations.c", 215
    ldax #__str_173
    sta _main__local_131
    stx _main__local_131+1
    lda _main__local_131
    ldx _main__local_131+1
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
    .loc "test_pragma_combinations.c", 216
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end172:
    .loc "test_pragma_combinations.c", 218
    ldax #__str_174
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
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 14
    endproc


    .segment "data"
__str_0:
    .text "TEST1"
    .byte 0
__str_5:
    .ascii "FAIL: single_pragma
"
    .byte 0
__str_6:
    .ascii "PASS: single_pragma
"
    .byte 0
__str_7:
    .text "TEST2"
    .byte 0
__str_11:
    .text "FAIL: pragma_reset (likely still ASCII)
"
    .byte 0
__str_12:
    .text "PASS: pragma_reset
"
    .byte 0
__str_16:
    .text "FAIL: heap_with_encoding (malloc failed)
"
    .byte 0
__str_17:
    .text "PASS: heap_with_encoding
"
    .byte 0
__str_18:
    .screencode "TEST4"
    .byte 0
__str_22:
    .screencode "FAIL: multiple_sequential_pragmas
"
    .byte 0
__str_23:
    .screencode "PASS: multiple_sequential_pragmas
"
    .byte 0
__str_24:
    .text "PETSCII"
    .byte 0
__str_28:
    .text "FAIL: function_scope_petscii
"
    .byte 0
__str_29:
    .ascii "ASCII"
    .byte 0
__str_33:
    .ascii "FAIL: function_scope_ascii
"
    .byte 0
__str_35:
    .ascii "PETSCII"
    .byte 0
__str_39:
    .ascii "FAIL: function_scope_petscii
"
    .byte 0
__str_41:
    .ascii "ASCII"
    .byte 0
__str_45:
    .ascii "FAIL: function_scope_ascii
"
    .byte 0
__str_46:
    .ascii "PASS: scope_changes
"
    .byte 0
__str_47:
    .text "TEST6"
    .byte 0
__str_51:
    .ascii "FAIL: pragma_override
"
    .byte 0
__str_52:
    .ascii "PASS: pragma_override
"
    .byte 0
__str_57:
    .text "FAIL: different_pragma_types
"
    .byte 0
__str_58:
    .text "PASS: different_pragma_types
"
    .byte 0
__str_59:
    .screencode "func1"
    .byte 0
__str_63:
    .screencode "FAIL: pragma_persistence (func1)
"
    .byte 0
__str_64:
    .screencode "func2"
    .byte 0
__str_68:
    .screencode "FAIL: pragma_persistence (func2)
"
    .byte 0
__str_70:
    .screencode "func1"
    .byte 0
__str_74:
    .screencode "FAIL: pragma_persistence (func1)
"
    .byte 0
__str_76:
    .screencode "func2"
    .byte 0
__str_80:
    .screencode "FAIL: pragma_persistence (func2)
"
    .byte 0
__str_81:
    .screencode "PASS: pragma_persistence
"
    .byte 0
__str_82:
    .screencode "EARLY"
    .byte 0
__str_83:
    .text "LATE"
    .byte 0
__str_88:
    .ascii "FAIL: pragma_timing
"
    .byte 0
__str_89:
    .ascii "PASS: pragma_timing
"
    .byte 0
__str_93:
    .ascii "FAIL: local_vars_unaffected
"
    .byte 0
__str_94:
    .ascii "PASS: local_vars_unaffected
"
    .byte 0
__str_95:
    .ascii "=== Pragma Combinations Integration Tests ===
"
    .byte 0
__str_101:
    .ascii "FAIL: single_pragma
"
    .byte 0
__str_102:
    .ascii "PASS: single_pragma
"
    .byte 0
__str_107:
    .ascii "FAIL: pragma_reset (likely still ASCII)
"
    .byte 0
__str_108:
    .ascii "PASS: pragma_reset
"
    .byte 0
__str_113:
    .ascii "FAIL: heap_with_encoding (malloc failed)
"
    .byte 0
__str_114:
    .ascii "PASS: heap_with_encoding
"
    .byte 0
__str_119:
    .ascii "FAIL: multiple_sequential_pragmas
"
    .byte 0
__str_120:
    .ascii "PASS: multiple_sequential_pragmas
"
    .byte 0
__str_123:
    .ascii "PETSCII"
    .byte 0
__str_127:
    .ascii "FAIL: function_scope_petscii
"
    .byte 0
__str_129:
    .ascii "ASCII"
    .byte 0
__str_133:
    .ascii "FAIL: function_scope_ascii
"
    .byte 0
__str_134:
    .ascii "PASS: scope_changes
"
    .byte 0
__str_139:
    .ascii "FAIL: pragma_override
"
    .byte 0
__str_140:
    .ascii "PASS: pragma_override
"
    .byte 0
__str_146:
    .ascii "FAIL: different_pragma_types
"
    .byte 0
__str_147:
    .ascii "PASS: different_pragma_types
"
    .byte 0
__str_150:
    .ascii "func1"
    .byte 0
__str_154:
    .ascii "FAIL: pragma_persistence (func1)
"
    .byte 0
__str_156:
    .ascii "func2"
    .byte 0
__str_160:
    .ascii "FAIL: pragma_persistence (func2)
"
    .byte 0
__str_161:
    .ascii "PASS: pragma_persistence
"
    .byte 0
__str_167:
    .ascii "FAIL: pragma_timing
"
    .byte 0
__str_168:
    .ascii "PASS: pragma_timing
"
    .byte 0
__str_173:
    .ascii "FAIL: local_vars_unaffected
"
    .byte 0
__str_174:
    .ascii "PASS: local_vars_unaffected
"
    .byte 0
__str_175:
    .ascii "
=== All Pragma Combination Tests Passed ===
"
    .byte 0

__zp_save_buf:

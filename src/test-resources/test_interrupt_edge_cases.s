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

    .global _basic_interrupt_handler
    .global _test_basic_interrupt
    .global _state_modifying_interrupt
    .global _test_interrupt_modify_globals
    .global _interrupt_with_asm
    .global _test_interrupt_with_asm
    .global _count_interrupt
    .global _test_multiple_interrupts
    .global _preserving_interrupt
    .global _test_interrupt_state_preservation
    .global _nested_interrupt
    .global _test_nested_interrupt_state
    .global _func_called_interrupt
    .global _caller_with_interrupt
    .global _test_interrupt_from_function
    .global _interrupt_a
    .global _interrupt_b
    .global _test_multiple_handlers
    .global _register_ops_interrupt
    .global _test_interrupt_register_ops
    .global _clobbering_interrupt
    .global _test_interrupt_clobber_tracking
    .global _main

    .segment "data"
    .byte 0
_interrupt_counter:
; .debug_var: @global _interrupt_counter offset=0 size=2 type=int16 scope=global
    .word 0
_global_flag:
; .debug_var: @global _global_flag offset=0 size=2 type=int16 scope=global
    .word 0
_nested_counter:
; .debug_var: @global _nested_counter offset=0 size=2 type=int16 scope=global
    .word 0
_handler_a_called:
; .debug_var: @global _handler_a_called offset=0 size=2 type=int16 scope=global
    .word 0
_handler_b_called:
; .debug_var: @global _handler_b_called offset=0 size=2 type=int16 scope=global
    .word 0

    .segment "code"

; function _basic_interrupt_handler
    proc _basic_interrupt_handler
    pha
    phx
    phy
    phz
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdio.h", 15

@entry:
    .loc "test_interrupt_edge_cases.c", 29
    lda _interrupt_counter
    ldx _interrupt_counter+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    clc
    adc #1
    sta $20
    lda $23
    adc #0
    sta $21
    lda $20
    ldx $21
    sta _interrupt_counter
    stx _interrupt_counter+1
@__return:
    plz
    ply
    plx
    pla
    rti
    rts
    .func_flags stack_call, isr, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 0
    endproc

; function _test_basic_interrupt
; SAC inline storage: 0 bytes
    _test_basic_interrupt__local_4: .word 0
    _test_basic_interrupt__local_6: .word 0
    _test_basic_interrupt__local_7: .word 0
    proc _test_basic_interrupt
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdio.h", 19

@entry:
    .loc "test_interrupt_edge_cases.c", 33
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta _interrupt_counter
    stx _interrupt_counter+1
    .loc "test_interrupt_edge_cases.c", 36
    jsr _basic_interrupt_handler
    .loc "test_interrupt_edge_cases.c", 38
    lda _interrupt_counter
    ldx _interrupt_counter+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #1
    bne @if_then0
    bra @if_end2
@if_then0:
    .loc "test_interrupt_edge_cases.c", 39
    ldax #__str_3
    sta _test_basic_interrupt__local_4
    stx _test_basic_interrupt__local_4+1
    lda _test_basic_interrupt__local_4
    ldx _test_basic_interrupt__local_4+1
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
    .loc "test_interrupt_edge_cases.c", 40
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
    .loc "test_interrupt_edge_cases.c", 42
    ldax #__str_4
    sta _test_basic_interrupt__local_7
    stx _test_basic_interrupt__local_7+1
    lda _test_basic_interrupt__local_7
    ldx _test_basic_interrupt__local_7+1
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

; function _state_modifying_interrupt
    proc _state_modifying_interrupt
    pha
    phx
    phy
    phz
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stddef.h", 10

@entry:
    .loc "test_interrupt_edge_cases.c", 50
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _global_flag
    stx _global_flag+1
    .loc "test_interrupt_edge_cases.c", 51
    lda _interrupt_counter
    ldx _interrupt_counter+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    clc
    adc #1
    sta $20
    lda $23
    adc #0
    sta $21
    lda $20
    ldx $21
    sta _interrupt_counter
    stx _interrupt_counter+1
@__return:
    plz
    ply
    plx
    pla
    rti
    rts
    .func_flags stack_call, isr, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 0
    endproc

; function _test_interrupt_modify_globals
; SAC inline storage: 0 bytes
    _test_interrupt_modify_globals__local_8: .word 0
    _test_interrupt_modify_globals__local_10: .word 0
    _test_interrupt_modify_globals__local_11: .word 0
    proc _test_interrupt_modify_globals
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdlib.h", 7

@entry:
    .loc "test_interrupt_edge_cases.c", 55
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta _global_flag
    stx _global_flag+1
    .loc "test_interrupt_edge_cases.c", 56
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta _interrupt_counter
    stx _interrupt_counter+1
    .loc "test_interrupt_edge_cases.c", 58
    jsr _state_modifying_interrupt
    .loc "test_interrupt_edge_cases.c", 60
    lda _global_flag
    ldx _global_flag+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #1
    bne @if_then5
@or_rhs8:
    lda _interrupt_counter
    ldx _interrupt_counter+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #1
    bne @if_then5
    bra @if_end7
@if_then5:
    .loc "test_interrupt_edge_cases.c", 61
    ldax #__str_9
    sta _test_interrupt_modify_globals__local_8
    stx _test_interrupt_modify_globals__local_8+1
    lda _test_interrupt_modify_globals__local_8
    ldx _test_interrupt_modify_globals__local_8+1
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
    .loc "test_interrupt_edge_cases.c", 62
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end7:
    .loc "test_interrupt_edge_cases.c", 64
    ldax #__str_10
    sta _test_interrupt_modify_globals__local_11
    stx _test_interrupt_modify_globals__local_11+1
    lda _test_interrupt_modify_globals__local_11
    ldx _test_interrupt_modify_globals__local_11+1
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

; function _interrupt_with_asm
    proc _interrupt_with_asm
    pha
    phx
    phy
    phz
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdlib.h", 24

@entry:
    .loc "test_interrupt_edge_cases.c", 72
    lda #$42
    .loc "test_interrupt_edge_cases.c", 73
    lda _interrupt_counter
    ldx _interrupt_counter+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    clc
    adc #1
    sta $20
    lda $23
    adc #0
    sta $21
    lda $20
    ldx $21
    sta _interrupt_counter
    stx _interrupt_counter+1
@__return:
    plz
    ply
    plx
    pla
    rti
    rts
    .func_flags stack_call, isr, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 0
    endproc

; function _test_interrupt_with_asm
; SAC inline storage: 0 bytes
    _test_interrupt_with_asm__local_4: .word 0
    _test_interrupt_with_asm__local_6: .word 0
    _test_interrupt_with_asm__local_7: .word 0
    proc _test_interrupt_with_asm
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdlib.h", 29

@entry:
    .loc "test_interrupt_edge_cases.c", 77
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta _interrupt_counter
    stx _interrupt_counter+1
    .loc "test_interrupt_edge_cases.c", 79
    jsr _interrupt_with_asm
    .loc "test_interrupt_edge_cases.c", 81
    lda _interrupt_counter
    ldx _interrupt_counter+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #1
    bne @if_then11
    bra @if_end13
@if_then11:
    .loc "test_interrupt_edge_cases.c", 82
    ldax #__str_14
    sta _test_interrupt_with_asm__local_4
    stx _test_interrupt_with_asm__local_4+1
    lda _test_interrupt_with_asm__local_4
    ldx _test_interrupt_with_asm__local_4+1
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
    .loc "test_interrupt_edge_cases.c", 83
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end13:
    .loc "test_interrupt_edge_cases.c", 85
    ldax #__str_15
    sta _test_interrupt_with_asm__local_7
    stx _test_interrupt_with_asm__local_7+1
    lda _test_interrupt_with_asm__local_7
    ldx _test_interrupt_with_asm__local_7+1
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

; function _count_interrupt
    proc _count_interrupt
    pha
    phx
    phy
    phz
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdlib.h", 45

@entry:
    .loc "test_interrupt_edge_cases.c", 93
    lda _interrupt_counter
    ldx _interrupt_counter+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    clc
    adc #1
    sta $20
    lda $23
    adc #0
    sta $21
    lda $20
    ldx $21
    sta _interrupt_counter
    stx _interrupt_counter+1
@__return:
    plz
    ply
    plx
    pla
    rti
    rts
    .func_flags stack_call, isr, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 0
    endproc

; function _test_multiple_interrupts
; SAC inline storage: 0 bytes
    _test_multiple_interrupts__local_4: .word 0
    _test_multiple_interrupts__local_6: .word 0
    _test_multiple_interrupts__local_7: .word 0
    proc _test_multiple_interrupts
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdlib.h", 49

@entry:
    .loc "test_interrupt_edge_cases.c", 97
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta _interrupt_counter
    stx _interrupt_counter+1
    .loc "test_interrupt_edge_cases.c", 99
    jsr _count_interrupt
    .loc "test_interrupt_edge_cases.c", 100
    jsr _count_interrupt
    .loc "test_interrupt_edge_cases.c", 101
    jsr _count_interrupt
    .loc "test_interrupt_edge_cases.c", 103
    lda _interrupt_counter
    ldx _interrupt_counter+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #3
    bne @if_then16
    bra @if_end18
@if_then16:
    .loc "test_interrupt_edge_cases.c", 104
    ldax #__str_19
    sta _test_multiple_interrupts__local_4
    stx _test_multiple_interrupts__local_4+1
    lda _test_multiple_interrupts__local_4
    ldx _test_multiple_interrupts__local_4+1
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
    .loc "test_interrupt_edge_cases.c", 105
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end18:
    .loc "test_interrupt_edge_cases.c", 107
    ldax #__str_20
    sta _test_multiple_interrupts__local_7
    stx _test_multiple_interrupts__local_7+1
    lda _test_multiple_interrupts__local_7
    ldx _test_multiple_interrupts__local_7+1
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

; function _preserving_interrupt
    proc _preserving_interrupt
    pha
    phx
    phy
    phz
    .var _fp = 0
    .loc "test_interrupt_edge_cases.c", 24

@entry:
    .loc "test_interrupt_edge_cases.c", 115
    lda #99
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _global_flag
    stx _global_flag+1
@__return:
    plz
    ply
    plx
    pla
    rti
    rts
    .func_flags stack_call, isr, leaf
    .reg_clobbers A, X
    .flag_clobbers N, Z
    .frame_size 0
    endproc

; function _test_interrupt_state_preservation
; SAC inline storage: 4 bytes
    _test_interrupt_state_preservation__local_0: .word 0
    _test_interrupt_state_preservation__local_2: .word 0
    _test_interrupt_state_preservation__local_12: .word 0
    _test_interrupt_state_preservation__local_14: .word 0
    _test_interrupt_state_preservation__local_15: .word 0
    proc _test_interrupt_state_preservation
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_interrupt_edge_cases.c", 28
    .local @_l_local_a = 0
    .local @_l_local_b = 2
; .debug_var: __test_interrupt_state_preservation @_l_local_a offset=0 size=2 type=int16 scope=local
; .debug_var: __test_interrupt_state_preservation @_l_local_b offset=2 size=2 type=int16 scope=local

@entry:
    .loc "test_interrupt_edge_cases.c", 119
    lda #100
    sta _test_interrupt_state_preservation__local_0
    lda #0
    sta _test_interrupt_state_preservation__local_0+1
    .loc "test_interrupt_edge_cases.c", 120
    lda #200
    sta _test_interrupt_state_preservation__local_2
    lda #0
    sta _test_interrupt_state_preservation__local_2+1
    .loc "test_interrupt_edge_cases.c", 121
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta _global_flag
    stx _global_flag+1
    .loc "test_interrupt_edge_cases.c", 123
    jsr _preserving_interrupt
    .loc "test_interrupt_edge_cases.c", 126
    lda _test_interrupt_state_preservation__local_0
    ldx _test_interrupt_state_preservation__local_0+1
    cmp.16 .AX, #100
    bne @if_then21
@or_rhs25:
    lda _test_interrupt_state_preservation__local_2
    ldx _test_interrupt_state_preservation__local_2+1
    cmp.16 .AX, #200
    bne @if_then21
@or_rhs24:
    lda _global_flag
    ldx _global_flag+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #99
    bne @if_then21
    bra @if_end23
@if_then21:
    .loc "test_interrupt_edge_cases.c", 127
    ldax #__str_26
    sta _test_interrupt_state_preservation__local_12
    stx _test_interrupt_state_preservation__local_12+1
    lda _test_interrupt_state_preservation__local_12
    ldx _test_interrupt_state_preservation__local_12+1
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
    .loc "test_interrupt_edge_cases.c", 128
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end23:
    .loc "test_interrupt_edge_cases.c", 130
    ldax #__str_27
    sta _test_interrupt_state_preservation__local_15
    stx _test_interrupt_state_preservation__local_15+1
    lda _test_interrupt_state_preservation__local_15
    ldx _test_interrupt_state_preservation__local_15+1
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

; function _nested_interrupt
    proc _nested_interrupt
    pha
    phx
    phy
    phz
    .var _fp = 0
    .loc "test_interrupt_edge_cases.c", 49

@entry:
    .loc "test_interrupt_edge_cases.c", 140
    lda _nested_counter
    ldx _nested_counter+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    clc
    adc #1
    sta $20
    lda $23
    adc #0
    sta $21
    lda $20
    ldx $21
    sta _nested_counter
    stx _nested_counter+1
    .loc "test_interrupt_edge_cases.c", 141
    lda _nested_counter
    ldx _nested_counter+1
    sta $20
    stx $21
    lda #10
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    mul.s16 .AX, $22
    sta $24
    stx $25
    lda $24
    ldx $25
    sta _global_flag
    stx _global_flag+1
@__return:
    plz
    ply
    plx
    pla
    rti
    rts
    .func_flags stack_call, isr, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 0
    endproc

; function _test_nested_interrupt_state
; SAC inline storage: 0 bytes
    _test_nested_interrupt_state__local_8: .word 0
    _test_nested_interrupt_state__local_10: .word 0
    _test_nested_interrupt_state__local_17: .word 0
    _test_nested_interrupt_state__local_19: .word 0
    _test_nested_interrupt_state__local_20: .word 0
    proc _test_nested_interrupt_state
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_interrupt_edge_cases.c", 54

@entry:
    .loc "test_interrupt_edge_cases.c", 145
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta _nested_counter
    stx _nested_counter+1
    .loc "test_interrupt_edge_cases.c", 146
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta _global_flag
    stx _global_flag+1
    .loc "test_interrupt_edge_cases.c", 148
    jsr _nested_interrupt
    .loc "test_interrupt_edge_cases.c", 149
    lda _nested_counter
    ldx _nested_counter+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #1
    bne @if_then28
@or_rhs31:
    lda _global_flag
    ldx _global_flag+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #10
    bne @if_then28
    bra @if_end30
@if_then28:
    .loc "test_interrupt_edge_cases.c", 150
    ldax #__str_32
    sta _test_nested_interrupt_state__local_8
    stx _test_nested_interrupt_state__local_8+1
    lda _test_nested_interrupt_state__local_8
    ldx _test_nested_interrupt_state__local_8+1
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
    .loc "test_interrupt_edge_cases.c", 151
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end30:
    .loc "test_interrupt_edge_cases.c", 154
    jsr _nested_interrupt
    .loc "test_interrupt_edge_cases.c", 155
    lda _nested_counter
    ldx _nested_counter+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #2
    bne @if_then33
@or_rhs36:
    lda _global_flag
    ldx _global_flag+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #20
    bne @if_then33
    bra @if_end35
@if_then33:
    .loc "test_interrupt_edge_cases.c", 156
    ldax #__str_37
    sta _test_nested_interrupt_state__local_17
    stx _test_nested_interrupt_state__local_17+1
    lda _test_nested_interrupt_state__local_17
    ldx _test_nested_interrupt_state__local_17+1
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
    .loc "test_interrupt_edge_cases.c", 157
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
    .loc "test_interrupt_edge_cases.c", 160
    ldax #__str_38
    sta _test_nested_interrupt_state__local_20
    stx _test_nested_interrupt_state__local_20+1
    lda _test_nested_interrupt_state__local_20
    ldx _test_nested_interrupt_state__local_20+1
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

; function _func_called_interrupt
    proc _func_called_interrupt
    pha
    phx
    phy
    phz
    .var _fp = 0
    .loc "test_interrupt_edge_cases.c", 77

@entry:
    .loc "test_interrupt_edge_cases.c", 168
    lda _interrupt_counter
    ldx _interrupt_counter+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    clc
    adc #1
    sta $20
    lda $23
    adc #0
    sta $21
    lda $20
    ldx $21
    sta _interrupt_counter
    stx _interrupt_counter+1
@__return:
    plz
    ply
    plx
    pla
    rti
    rts
    .func_flags stack_call, isr, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 0
    endproc

; function _caller_with_interrupt
; SAC inline storage: 0 bytes
    proc _caller_with_interrupt
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_interrupt_edge_cases.c", 81

@entry:
    .loc "test_interrupt_edge_cases.c", 172
    jsr _func_called_interrupt
    .loc "test_interrupt_edge_cases.c", 173
    lda _interrupt_counter
    ldx _interrupt_counter+1
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 0
    endproc

; function _test_interrupt_from_function
; SAC inline storage: 2 bytes
    _test_interrupt_from_function__local_1: .word 0
    _test_interrupt_from_function__local_10: .word 0
    _test_interrupt_from_function__local_12: .word 0
    _test_interrupt_from_function__local_13: .word 0
    proc _test_interrupt_from_function
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_interrupt_edge_cases.c", 86
    .local @_l_result = 0
; .debug_var: __test_interrupt_from_function @_l_result offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_interrupt_edge_cases.c", 177
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta _interrupt_counter
    stx _interrupt_counter+1
    .loc "test_interrupt_edge_cases.c", 172
    jsr _func_called_interrupt
    .loc "test_interrupt_edge_cases.c", 173
    lda _interrupt_counter
    ldx _interrupt_counter+1
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
@inline_end39:
    .loc "test_interrupt_edge_cases.c", 178
    lda $20
    ldx $21
    sta _test_interrupt_from_function__local_1
    stx _test_interrupt_from_function__local_1+1
    .loc "test_interrupt_edge_cases.c", 180
    lda _test_interrupt_from_function__local_1
    ldx _test_interrupt_from_function__local_1+1
    cmp.16 .AX, #1
    bne @if_then41
@or_rhs44:
    lda _interrupt_counter
    ldx _interrupt_counter+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #1
    bne @if_then41
    bra @if_end43
@if_then41:
    .loc "test_interrupt_edge_cases.c", 181
    ldax #__str_45
    sta _test_interrupt_from_function__local_10
    stx _test_interrupt_from_function__local_10+1
    lda _test_interrupt_from_function__local_10
    ldx _test_interrupt_from_function__local_10+1
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
    .loc "test_interrupt_edge_cases.c", 182
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end43:
    .loc "test_interrupt_edge_cases.c", 184
    ldax #__str_46
    sta _test_interrupt_from_function__local_13
    stx _test_interrupt_from_function__local_13+1
    lda _test_interrupt_from_function__local_13
    ldx _test_interrupt_from_function__local_13+1
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

; function _interrupt_a
    proc _interrupt_a
    pha
    phx
    phy
    phz
    .var _fp = 0
    .loc "test_interrupt_edge_cases.c", 104

@entry:
    .loc "test_interrupt_edge_cases.c", 195
    lda _handler_a_called
    ldx _handler_a_called+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    clc
    adc #1
    sta $20
    lda $23
    adc #0
    sta $21
    lda $20
    ldx $21
    sta _handler_a_called
    stx _handler_a_called+1
@__return:
    plz
    ply
    plx
    pla
    rti
    rts
    .func_flags stack_call, isr, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 0
    endproc

; function _interrupt_b
    proc _interrupt_b
    pha
    phx
    phy
    phz
    .var _fp = 0
    .loc "test_interrupt_edge_cases.c", 108

@entry:
    .loc "test_interrupt_edge_cases.c", 199
    lda _handler_b_called
    ldx _handler_b_called+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    clc
    adc #1
    sta $20
    lda $23
    adc #0
    sta $21
    lda $20
    ldx $21
    sta _handler_b_called
    stx _handler_b_called+1
@__return:
    plz
    ply
    plx
    pla
    rti
    rts
    .func_flags stack_call, isr, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 0
    endproc

; function _test_multiple_handlers
; SAC inline storage: 0 bytes
    _test_multiple_handlers__local_8: .word 0
    _test_multiple_handlers__local_10: .word 0
    _test_multiple_handlers__local_11: .word 0
    proc _test_multiple_handlers
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_interrupt_edge_cases.c", 112

@entry:
    .loc "test_interrupt_edge_cases.c", 203
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta _handler_a_called
    stx _handler_a_called+1
    .loc "test_interrupt_edge_cases.c", 204
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta _handler_b_called
    stx _handler_b_called+1
    .loc "test_interrupt_edge_cases.c", 206
    jsr _interrupt_a
    .loc "test_interrupt_edge_cases.c", 207
    jsr _interrupt_b
    .loc "test_interrupt_edge_cases.c", 208
    jsr _interrupt_a
    .loc "test_interrupt_edge_cases.c", 210
    lda _handler_a_called
    ldx _handler_a_called+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #2
    bne @if_then47
@or_rhs50:
    lda _handler_b_called
    ldx _handler_b_called+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #1
    bne @if_then47
    bra @if_end49
@if_then47:
    .loc "test_interrupt_edge_cases.c", 211
    ldax #__str_51
    sta _test_multiple_handlers__local_8
    stx _test_multiple_handlers__local_8+1
    lda _test_multiple_handlers__local_8
    ldx _test_multiple_handlers__local_8+1
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
    .loc "test_interrupt_edge_cases.c", 212
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end49:
    .loc "test_interrupt_edge_cases.c", 214
    ldax #__str_52
    sta _test_multiple_handlers__local_11
    stx _test_multiple_handlers__local_11+1
    lda _test_multiple_handlers__local_11
    ldx _test_multiple_handlers__local_11+1
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

; function _register_ops_interrupt
    proc _register_ops_interrupt
    pha
    phx
    phy
    phz
    .var _fp = 0
    .loc "test_interrupt_edge_cases.c", 131

@entry:
    .loc "test_interrupt_edge_cases.c", 222
    ldx #$55
    .loc "test_interrupt_edge_cases.c", 223
    ldy #$66
    .loc "test_interrupt_edge_cases.c", 224
    lda _interrupt_counter
    ldx _interrupt_counter+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    clc
    adc #1
    sta $20
    lda $23
    adc #0
    sta $21
    lda $20
    ldx $21
    sta _interrupt_counter
    stx _interrupt_counter+1
@__return:
    plz
    ply
    plx
    pla
    rti
    rts
    .func_flags stack_call, isr, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 0
    endproc

; function _test_interrupt_register_ops
; SAC inline storage: 0 bytes
    _test_interrupt_register_ops__local_4: .word 0
    _test_interrupt_register_ops__local_6: .word 0
    _test_interrupt_register_ops__local_7: .word 0
    proc _test_interrupt_register_ops
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_interrupt_edge_cases.c", 137

@entry:
    .loc "test_interrupt_edge_cases.c", 228
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta _interrupt_counter
    stx _interrupt_counter+1
    .loc "test_interrupt_edge_cases.c", 230
    jsr _register_ops_interrupt
    .loc "test_interrupt_edge_cases.c", 232
    lda _interrupt_counter
    ldx _interrupt_counter+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #1
    bne @if_then53
    bra @if_end55
@if_then53:
    .loc "test_interrupt_edge_cases.c", 233
    ldax #__str_56
    sta _test_interrupt_register_ops__local_4
    stx _test_interrupt_register_ops__local_4+1
    lda _test_interrupt_register_ops__local_4
    ldx _test_interrupt_register_ops__local_4+1
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
    .loc "test_interrupt_edge_cases.c", 234
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
    .loc "test_interrupt_edge_cases.c", 236
    ldax #__str_57
    sta _test_interrupt_register_ops__local_7
    stx _test_interrupt_register_ops__local_7+1
    lda _test_interrupt_register_ops__local_7
    ldx _test_interrupt_register_ops__local_7+1
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

; function _clobbering_interrupt
    proc _clobbering_interrupt
    pha
    phx
    phy
    phz
    .var _fp = 0
    .loc "test_interrupt_edge_cases.c", 153

@entry:
    .loc "test_interrupt_edge_cases.c", 244
    lda #$FF; ldx #$00; ldy #$00
    .loc "test_interrupt_edge_cases.c", 245
    lda _interrupt_counter
    ldx _interrupt_counter+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    clc
    adc #1
    sta $20
    lda $23
    adc #0
    sta $21
    lda $20
    ldx $21
    sta _interrupt_counter
    stx _interrupt_counter+1
@__return:
    plz
    ply
    plx
    pla
    rti
    rts
    .func_flags stack_call, isr, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 0
    endproc

; function _test_interrupt_clobber_tracking
; SAC inline storage: 4 bytes
    _test_interrupt_clobber_tracking__local_0: .word 0
    _test_interrupt_clobber_tracking__local_2: .word 0
    _test_interrupt_clobber_tracking__local_12: .word 0
    _test_interrupt_clobber_tracking__local_14: .word 0
    _test_interrupt_clobber_tracking__local_15: .word 0
    proc _test_interrupt_clobber_tracking
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_interrupt_edge_cases.c", 158
    .local @_l_x = 0
    .local @_l_y = 2
; .debug_var: __test_interrupt_clobber_tracking @_l_x offset=0 size=2 type=int16 scope=local
; .debug_var: __test_interrupt_clobber_tracking @_l_y offset=2 size=2 type=int16 scope=local

@entry:
    .loc "test_interrupt_edge_cases.c", 249
    lda #50
    sta _test_interrupt_clobber_tracking__local_0
    lda #0
    sta _test_interrupt_clobber_tracking__local_0+1
    .loc "test_interrupt_edge_cases.c", 250
    lda #75
    sta _test_interrupt_clobber_tracking__local_2
    lda #0
    sta _test_interrupt_clobber_tracking__local_2+1
    .loc "test_interrupt_edge_cases.c", 251
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta _interrupt_counter
    stx _interrupt_counter+1
    .loc "test_interrupt_edge_cases.c", 253
    jsr _clobbering_interrupt
    .loc "test_interrupt_edge_cases.c", 256
    lda _test_interrupt_clobber_tracking__local_0
    ldx _test_interrupt_clobber_tracking__local_0+1
    cmp.16 .AX, #50
    bne @if_then58
@or_rhs62:
    lda _test_interrupt_clobber_tracking__local_2
    ldx _test_interrupt_clobber_tracking__local_2+1
    cmp.16 .AX, #75
    bne @if_then58
@or_rhs61:
    lda _interrupt_counter
    ldx _interrupt_counter+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #1
    bne @if_then58
    bra @if_end60
@if_then58:
    .loc "test_interrupt_edge_cases.c", 257
    ldax #__str_63
    sta _test_interrupt_clobber_tracking__local_12
    stx _test_interrupt_clobber_tracking__local_12+1
    lda _test_interrupt_clobber_tracking__local_12
    ldx _test_interrupt_clobber_tracking__local_12+1
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
    .loc "test_interrupt_edge_cases.c", 258
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end60:
    .loc "test_interrupt_edge_cases.c", 260
    ldax #__str_64
    sta _test_interrupt_clobber_tracking__local_15
    stx _test_interrupt_clobber_tracking__local_15+1
    lda _test_interrupt_clobber_tracking__local_15
    ldx _test_interrupt_clobber_tracking__local_15+1
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

; function _main
; SAC inline storage: 10 bytes
    _main__local_0: .word 0
    _main__local_6: .word 0
    _main__local_8: .word 0
    _main__local_9: .word 0
    _main__local_19: .word 0
    _main__local_21: .word 0
    _main__local_22: .word 0
    _main__local_28: .word 0
    _main__local_30: .word 0
    _main__local_31: .word 0
    _main__local_37: .word 0
    _main__local_39: .word 0
    _main__local_40: .word 0
    _main__local_42: .word 0
    _main__local_44: .word 0
    _main__local_54: .word 0
    _main__local_56: .word 0
    _main__local_57: .word 0
    _main__local_67: .word 0
    _main__local_69: .word 0
    _main__local_76: .word 0
    _main__local_78: .word 0
    _main__local_79: .word 0
    _main__local_82: .word 0
    _main__local_90: .word 0
    _main__local_92: .word 0
    _main__local_93: .word 0
    _main__local_103: .word 0
    _main__local_105: .word 0
    _main__local_106: .word 0
    _main__local_112: .word 0
    _main__local_114: .word 0
    _main__local_115: .word 0
    _main__local_117: .word 0
    _main__local_119: .word 0
    _main__local_129: .word 0
    _main__local_131: .word 0
    _main__local_132: .word 0
    _main__local_134: .word 0
    _main__local_135: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_interrupt_edge_cases.c", 177
    .local @_l_local_a = 0
    .local @_l_local_b = 2
    .local @_l_result = 4
    .local @_l_x = 6
    .local @_l_y = 8
; .debug_var: __main @_l_local_a offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_local_b offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_result offset=4 size=2 type=int16 scope=local
; .debug_var: __main @_l_x offset=6 size=2 type=int16 scope=local
; .debug_var: __main @_l_y offset=8 size=2 type=int16 scope=local

@entry:
    .loc "test_interrupt_edge_cases.c", 268
    ldax #__str_65
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
    .loc "test_interrupt_edge_cases.c", 33
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta _interrupt_counter
    stx _interrupt_counter+1
    .loc "test_interrupt_edge_cases.c", 36
    jsr _basic_interrupt_handler
    .loc "test_interrupt_edge_cases.c", 38
    lda _interrupt_counter
    ldx _interrupt_counter+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #1
    bne @if_then67
    bra @if_end69
@if_then67:
    .loc "test_interrupt_edge_cases.c", 39
    ldax #__str_70
    sta _main__local_6
    stx _main__local_6+1
    lda _main__local_6
    ldx _main__local_6+1
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
    .loc "test_interrupt_edge_cases.c", 40
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end69:
    .loc "test_interrupt_edge_cases.c", 42
    ldax #__str_71
    sta _main__local_9
    stx _main__local_9+1
    lda _main__local_9
    ldx _main__local_9+1
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
@inline_end66:
    .loc "test_interrupt_edge_cases.c", 55
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta _global_flag
    stx _global_flag+1
    .loc "test_interrupt_edge_cases.c", 56
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta _interrupt_counter
    stx _interrupt_counter+1
    .loc "test_interrupt_edge_cases.c", 58
    jsr _state_modifying_interrupt
    .loc "test_interrupt_edge_cases.c", 60
    lda _global_flag
    ldx _global_flag+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #1
    bne @if_then73
@or_rhs76:
    lda _interrupt_counter
    ldx _interrupt_counter+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #1
    bne @if_then73
    bra @if_end75
@if_then73:
    .loc "test_interrupt_edge_cases.c", 61
    ldax #__str_77
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
    .loc "test_interrupt_edge_cases.c", 62
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
    .loc "test_interrupt_edge_cases.c", 64
    ldax #__str_78
    sta _main__local_22
    stx _main__local_22+1
    lda _main__local_22
    ldx _main__local_22+1
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
@inline_end72:
    .loc "test_interrupt_edge_cases.c", 77
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta _interrupt_counter
    stx _interrupt_counter+1
    .loc "test_interrupt_edge_cases.c", 79
    jsr _interrupt_with_asm
    .loc "test_interrupt_edge_cases.c", 81
    lda _interrupt_counter
    ldx _interrupt_counter+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #1
    bne @if_then80
    bra @if_end82
@if_then80:
    .loc "test_interrupt_edge_cases.c", 82
    ldax #__str_83
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
    .loc "test_interrupt_edge_cases.c", 83
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end82:
    .loc "test_interrupt_edge_cases.c", 85
    ldax #__str_84
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
@inline_end79:
    .loc "test_interrupt_edge_cases.c", 97
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta _interrupt_counter
    stx _interrupt_counter+1
    .loc "test_interrupt_edge_cases.c", 99
    jsr _count_interrupt
    .loc "test_interrupt_edge_cases.c", 100
    jsr _count_interrupt
    .loc "test_interrupt_edge_cases.c", 101
    jsr _count_interrupt
    .loc "test_interrupt_edge_cases.c", 103
    lda _interrupt_counter
    ldx _interrupt_counter+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #3
    bne @if_then86
    bra @if_end88
@if_then86:
    .loc "test_interrupt_edge_cases.c", 104
    ldax #__str_89
    sta _main__local_37
    stx _main__local_37+1
    lda _main__local_37
    ldx _main__local_37+1
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
    .loc "test_interrupt_edge_cases.c", 105
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end88:
    .loc "test_interrupt_edge_cases.c", 107
    ldax #__str_90
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
@inline_end85:
    .loc "test_interrupt_edge_cases.c", 119
    lda #100
    sta _main__local_42
    lda #0
    sta _main__local_42+1
    .loc "test_interrupt_edge_cases.c", 120
    lda #200
    sta _main__local_44
    lda #0
    sta _main__local_44+1
    .loc "test_interrupt_edge_cases.c", 121
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta _global_flag
    stx _global_flag+1
    .loc "test_interrupt_edge_cases.c", 123
    jsr _preserving_interrupt
    .loc "test_interrupt_edge_cases.c", 126
    lda _main__local_42
    ldx _main__local_42+1
    cmp.16 .AX, #100
    bne @if_then92
@or_rhs96:
    lda _main__local_44
    ldx _main__local_44+1
    cmp.16 .AX, #200
    bne @if_then92
@or_rhs95:
    lda _global_flag
    ldx _global_flag+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #99
    bne @if_then92
    bra @if_end94
@if_then92:
    .loc "test_interrupt_edge_cases.c", 127
    ldax #__str_97
    sta _main__local_54
    stx _main__local_54+1
    lda _main__local_54
    ldx _main__local_54+1
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
    .loc "test_interrupt_edge_cases.c", 128
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
    .loc "test_interrupt_edge_cases.c", 130
    ldax #__str_98
    sta _main__local_57
    stx _main__local_57+1
    lda _main__local_57
    ldx _main__local_57+1
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
@inline_end91:
    .loc "test_interrupt_edge_cases.c", 145
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta _nested_counter
    stx _nested_counter+1
    .loc "test_interrupt_edge_cases.c", 146
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta _global_flag
    stx _global_flag+1
    .loc "test_interrupt_edge_cases.c", 148
    jsr _nested_interrupt
    .loc "test_interrupt_edge_cases.c", 149
    lda _nested_counter
    ldx _nested_counter+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #1
    bne @if_then100
@or_rhs103:
    lda _global_flag
    ldx _global_flag+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #10
    bne @if_then100
    bra @if_end102
@if_then100:
    .loc "test_interrupt_edge_cases.c", 150
    ldax #__str_104
    sta _main__local_67
    stx _main__local_67+1
    lda _main__local_67
    ldx _main__local_67+1
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
    .loc "test_interrupt_edge_cases.c", 151
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
    .loc "test_interrupt_edge_cases.c", 154
    jsr _nested_interrupt
    .loc "test_interrupt_edge_cases.c", 155
    lda _nested_counter
    ldx _nested_counter+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #2
    bne @if_then105
@or_rhs108:
    lda _global_flag
    ldx _global_flag+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #20
    bne @if_then105
    bra @if_end107
@if_then105:
    .loc "test_interrupt_edge_cases.c", 156
    ldax #__str_109
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
    .loc "test_interrupt_edge_cases.c", 157
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end107:
    .loc "test_interrupt_edge_cases.c", 160
    ldax #__str_110
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
@inline_end99:
    .loc "test_interrupt_edge_cases.c", 177
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta _interrupt_counter
    stx _interrupt_counter+1
    .loc "test_interrupt_edge_cases.c", 172
    jsr _func_called_interrupt
    .loc "test_interrupt_edge_cases.c", 173
    lda _interrupt_counter
    ldx _interrupt_counter+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end112:
    .loc "test_interrupt_edge_cases.c", 178
    lda $22
    ldx $23
    sta _main__local_82
    stx _main__local_82+1
    .loc "test_interrupt_edge_cases.c", 180
    lda _main__local_82
    ldx _main__local_82+1
    cmp.16 .AX, #1
    bne @if_then114
@or_rhs117:
    lda _interrupt_counter
    ldx _interrupt_counter+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #1
    bne @if_then114
    bra @if_end116
@if_then114:
    .loc "test_interrupt_edge_cases.c", 181
    ldax #__str_118
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
    .loc "test_interrupt_edge_cases.c", 182
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end116:
    .loc "test_interrupt_edge_cases.c", 184
    ldax #__str_119
    sta _main__local_93
    stx _main__local_93+1
    lda _main__local_93
    ldx _main__local_93+1
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
    .loc "test_interrupt_edge_cases.c", 203
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta _handler_a_called
    stx _handler_a_called+1
    .loc "test_interrupt_edge_cases.c", 204
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta _handler_b_called
    stx _handler_b_called+1
    .loc "test_interrupt_edge_cases.c", 206
    jsr _interrupt_a
    .loc "test_interrupt_edge_cases.c", 207
    jsr _interrupt_b
    .loc "test_interrupt_edge_cases.c", 208
    jsr _interrupt_a
    .loc "test_interrupt_edge_cases.c", 210
    lda _handler_a_called
    ldx _handler_a_called+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #2
    bne @if_then121
@or_rhs124:
    lda _handler_b_called
    ldx _handler_b_called+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #1
    bne @if_then121
    bra @if_end123
@if_then121:
    .loc "test_interrupt_edge_cases.c", 211
    ldax #__str_125
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
    .loc "test_interrupt_edge_cases.c", 212
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end123:
    .loc "test_interrupt_edge_cases.c", 214
    ldax #__str_126
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
@inline_end120:
    .loc "test_interrupt_edge_cases.c", 228
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta _interrupt_counter
    stx _interrupt_counter+1
    .loc "test_interrupt_edge_cases.c", 230
    jsr _register_ops_interrupt
    .loc "test_interrupt_edge_cases.c", 232
    lda _interrupt_counter
    ldx _interrupt_counter+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #1
    bne @if_then128
    bra @if_end130
@if_then128:
    .loc "test_interrupt_edge_cases.c", 233
    ldax #__str_131
    sta _main__local_112
    stx _main__local_112+1
    lda _main__local_112
    ldx _main__local_112+1
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
    .loc "test_interrupt_edge_cases.c", 234
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end130:
    .loc "test_interrupt_edge_cases.c", 236
    ldax #__str_132
    sta _main__local_115
    stx _main__local_115+1
    lda _main__local_115
    ldx _main__local_115+1
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
@inline_end127:
    .loc "test_interrupt_edge_cases.c", 249
    lda #50
    sta _main__local_117
    lda #0
    sta _main__local_117+1
    .loc "test_interrupt_edge_cases.c", 250
    lda #75
    sta _main__local_119
    lda #0
    sta _main__local_119+1
    .loc "test_interrupt_edge_cases.c", 251
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta _interrupt_counter
    stx _interrupt_counter+1
    .loc "test_interrupt_edge_cases.c", 253
    jsr _clobbering_interrupt
    .loc "test_interrupt_edge_cases.c", 256
    lda _main__local_117
    ldx _main__local_117+1
    cmp.16 .AX, #50
    bne @if_then134
@or_rhs138:
    lda _main__local_119
    ldx _main__local_119+1
    cmp.16 .AX, #75
    bne @if_then134
@or_rhs137:
    lda _interrupt_counter
    ldx _interrupt_counter+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #1
    bne @if_then134
    bra @if_end136
@if_then134:
    .loc "test_interrupt_edge_cases.c", 257
    ldax #__str_139
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
    .loc "test_interrupt_edge_cases.c", 258
    lda #1
    ldx #0
    push .ax
    jsr _exit
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end136:
    .loc "test_interrupt_edge_cases.c", 260
    ldax #__str_140
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
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 10
    endproc


    .segment "data"
__str_3:
    .text "FAIL: basic_interrupt
"
    .byte 0
__str_4:
    .text "PASS: basic_interrupt
"
    .byte 0
__str_9:
    .text "FAIL: interrupt_modify_globals
"
    .byte 0
__str_10:
    .text "PASS: interrupt_modify_globals
"
    .byte 0
__str_14:
    .text "FAIL: interrupt_with_asm
"
    .byte 0
__str_15:
    .text "PASS: interrupt_with_asm
"
    .byte 0
__str_19:
    .text "FAIL: multiple_interrupts
"
    .byte 0
__str_20:
    .text "PASS: multiple_interrupts
"
    .byte 0
__str_26:
    .text "FAIL: interrupt_state_preservation
"
    .byte 0
__str_27:
    .text "PASS: interrupt_state_preservation
"
    .byte 0
__str_32:
    .text "FAIL: nested_interrupt_state (first)
"
    .byte 0
__str_37:
    .text "FAIL: nested_interrupt_state (second)
"
    .byte 0
__str_38:
    .text "PASS: nested_interrupt_state
"
    .byte 0
__str_45:
    .text "FAIL: interrupt_from_function
"
    .byte 0
__str_46:
    .text "PASS: interrupt_from_function
"
    .byte 0
__str_51:
    .text "FAIL: multiple_handlers
"
    .byte 0
__str_52:
    .text "PASS: multiple_handlers
"
    .byte 0
__str_56:
    .text "FAIL: interrupt_register_ops
"
    .byte 0
__str_57:
    .text "PASS: interrupt_register_ops
"
    .byte 0
__str_63:
    .text "FAIL: interrupt_clobber_tracking
"
    .byte 0
__str_64:
    .text "PASS: interrupt_clobber_tracking
"
    .byte 0
__str_65:
    .text "=== Interrupt Handler Edge Cases Tests ===
"
    .byte 0
__str_70:
    .text "FAIL: basic_interrupt
"
    .byte 0
__str_71:
    .text "PASS: basic_interrupt
"
    .byte 0
__str_77:
    .text "FAIL: interrupt_modify_globals
"
    .byte 0
__str_78:
    .text "PASS: interrupt_modify_globals
"
    .byte 0
__str_83:
    .text "FAIL: interrupt_with_asm
"
    .byte 0
__str_84:
    .text "PASS: interrupt_with_asm
"
    .byte 0
__str_89:
    .text "FAIL: multiple_interrupts
"
    .byte 0
__str_90:
    .text "PASS: multiple_interrupts
"
    .byte 0
__str_97:
    .text "FAIL: interrupt_state_preservation
"
    .byte 0
__str_98:
    .text "PASS: interrupt_state_preservation
"
    .byte 0
__str_104:
    .text "FAIL: nested_interrupt_state (first)
"
    .byte 0
__str_109:
    .text "FAIL: nested_interrupt_state (second)
"
    .byte 0
__str_110:
    .text "PASS: nested_interrupt_state
"
    .byte 0
__str_118:
    .text "FAIL: interrupt_from_function
"
    .byte 0
__str_119:
    .text "PASS: interrupt_from_function
"
    .byte 0
__str_125:
    .text "FAIL: multiple_handlers
"
    .byte 0
__str_126:
    .text "PASS: multiple_handlers
"
    .byte 0
__str_131:
    .text "FAIL: interrupt_register_ops
"
    .byte 0
__str_132:
    .text "PASS: interrupt_register_ops
"
    .byte 0
__str_139:
    .text "FAIL: interrupt_clobber_tracking
"
    .byte 0
__str_140:
    .text "PASS: interrupt_clobber_tracking
"
    .byte 0
__str_141:
    .text "
=== All Interrupt Handler Tests Passed ===
"
    .byte 0

__zp_save_buf:

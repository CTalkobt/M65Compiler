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

    .global _results
    .global _test_switch
    .global _main

    .segment "data"
    .byte 0
_results:
; .debug_var: @global _results offset=0 size=2 type=ptr scope=global
    .word 0

    .segment "code"

; function _test_switch
; SAC inline storage: 4 bytes
    .global _test_switch__param_val
    _test_switch__param_val: .word 0
    _test_switch__local_0: .word 0
    _test_switch__local_1: .word 0
    proc _test_switch, W#@_p_val
    .sac
    .var _fp = 0
    .loc "test_mmemu_control.c", 4
    .local @_l_result = 2
; .debug_var: __test_switch @_l_result offset=2 size=2 type=int16 scope=local
    .var @_p_val = 2
; .debug_var: __test_switch @_p_val offset=2 size=2 type=int16 scope=parameter

@entry:
    .loc "test_mmemu_control.c", 5
    lda #0
    sta _test_switch__local_1
    sta _test_switch__local_1+1
    .loc "test_mmemu_control.c", 6
    lda _test_switch__param_val
    ldx _test_switch__param_val+1
    cmp.16 .AX, #1
    beq @case1
@case_skip5:
    lda _test_switch__param_val
    ldx _test_switch__param_val+1
    cmp.16 .AX, #2
    beq @case2
@case_skip6:
    lda _test_switch__param_val
    ldx _test_switch__param_val+1
    cmp.16 .AX, #3
    beq @case3
    bra @default4
@case1:
    .loc "test_mmemu_control.c", 8
    lda #10
    sta _test_switch__local_1
    lda #0
    sta _test_switch__local_1+1
    bra @switch_end0
@case2:
    .loc "test_mmemu_control.c", 11
    lda #20
    sta _test_switch__local_1
    lda #0
    sta _test_switch__local_1+1
@case3:
    .loc "test_mmemu_control.c", 14
    lda _test_switch__local_1
    ldx _test_switch__local_1+1
    add.16 .AX, #5
    sta $22
    stx $23
    sta _test_switch__local_1
    stx _test_switch__local_1+1
    bra @switch_end0
@default4:
    .loc "test_mmemu_control.c", 17
    lda #100
    sta _test_switch__local_1
    lda #0
    sta _test_switch__local_1+1
@switch_end0:
    .loc "test_mmemu_control.c", 20
    lda _test_switch__local_1
    ldx _test_switch__local_1+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _main
; SAC inline storage: 10 bytes
    _main__local_0: .word 0
    _main__local_1: .word 0
    _main__local_87: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_mmemu_control.c", 23
    .local @_l_i = 0
    .local @_l_result = 8
    .local @_l_sum = 2
; .debug_var: __main @_l_i offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_result offset=8 size=2 type=int16 scope=local
; .debug_var: __main @_l_sum offset=2 size=2 type=int16 scope=local

@entry:
    .loc "test_mmemu_control.c", 27
    lda #0
    ldx #64
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _results
    stx _results+1
    .loc "test_mmemu_control.c", 31
    lda #1
    sta $20
    lda _results
    ldx _results+1
    sta $22
    stx $23
    lda #1
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx #0
    pha
    lda $24
    ldx $25
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
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "test_mmemu_control.c", 37
    lda #1
    sta $20
    lda _results
    ldx _results+1
    sta $22
    stx $23
    lda #2
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx #0
    pha
    lda $24
    ldx $25
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
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "test_mmemu_control.c", 43
    lda #0
    sta _main__local_0
    sta _main__local_0+1
    .loc "test_mmemu_control.c", 44
    lda #0
    sta _main__local_1
    sta _main__local_1+1
@while_cond8:
    .loc "test_mmemu_control.c", 45
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, #5
    bcc @while_body9
    bra @while_end10
@while_body9:
    .loc "test_mmemu_control.c", 46
    lda _main__local_0
    ldx _main__local_0+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_1
    ldx _main__local_1+1
    add.16 .AX, __zp_scratch2
    sta $24
    stx $25
    sta _main__local_1
    stx _main__local_1+1
    .loc "test_mmemu_control.c", 47
    inc.16f __vr0
    bra @while_cond8
@while_end10:
    lda _main__local_1
    ldx _main__local_1+1
    sta $20
    .loc "test_mmemu_control.c", 49
    lda _results
    ldx _results+1
    sta $22
    stx $23
    lda #3
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx #0
    pha
    lda $24
    ldx $25
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
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "test_mmemu_control.c", 52
    lda #0
    sta _main__local_0
    sta _main__local_0+1
    .loc "test_mmemu_control.c", 53
    lda #0
    sta _main__local_1
    sta _main__local_1+1
@do_body11:
    .loc "test_mmemu_control.c", 55
    lda _main__local_0
    ldx _main__local_0+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_1
    ldx _main__local_1+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    sta _main__local_1
    stx _main__local_1+1
    .loc "test_mmemu_control.c", 56
    inc.16f __vr0
@do_cond12:
    .loc "test_mmemu_control.c", 57
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, #5
    bcc @do_body11
@do_end13:
    lda _main__local_1
    ldx _main__local_1+1
    sta $20
    .loc "test_mmemu_control.c", 58
    lda _results
    ldx _results+1
    sta $22
    stx $23
    lda #4
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx #0
    pha
    lda $24
    ldx $25
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
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "test_mmemu_control.c", 61
    lda #0
    sta _main__local_1
    sta _main__local_1+1
    .loc "test_mmemu_control.c", 62
    lda #0
    sta _main__local_0
    sta _main__local_0+1
@for_cond14:
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, #10
    bcc @for_body15
    bra @for_end17
@for_body15:
    .loc "test_mmemu_control.c", 63
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, #3
    bcc @for_inc16
@if_end20:
    .loc "test_mmemu_control.c", 64
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, #6
    beq @if_end23
    bcs @for_end17
    bra @if_end23
@if_end23:
    .loc "test_mmemu_control.c", 65
    lda _main__local_0
    ldx _main__local_0+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_1
    ldx _main__local_1+1
    add.16 .AX, __zp_scratch2
    sta $2C
    stx $2D
    sta _main__local_1
    stx _main__local_1+1
@for_inc16:
    .loc "test_mmemu_control.c", 62
    inc.16f __vr0
    bra @for_cond14
@for_end17:
    lda _main__local_1
    ldx _main__local_1+1
    sta $20
    .loc "test_mmemu_control.c", 67
    lda _results
    ldx _results+1
    sta $22
    stx $23
    lda #5
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx #0
    pha
    lda $24
    ldx $25
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
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "test_mmemu_control.c", 70
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_mmemu_control.c", 5
    lda #0
    sta $26
    sta $27
    .loc "test_mmemu_control.c", 6
    lda $20
    ldx $21
    cmp.16 .AX, #1
    beq @case26
@case_skip30:
    lda $22
    ldx $23
    cmp.16 .AX, #2
    beq @case27
@case_skip31:
    lda $22
    ldx $23
    cmp.16 .AX, #3
    beq @case28
    bra @default29
@case26:
    .loc "test_mmemu_control.c", 8
    lda #10
    sta $26
    ldx #0
    stx $27
    bra @switch_end25
@case27:
    .loc "test_mmemu_control.c", 11
    lda #20
    sta $26
    ldx #0
    stx $27
@case28:
    .loc "test_mmemu_control.c", 14
    lda $26
    clc
    adc #5
    sta $26
    lda $27
    adc #0
    sta $27
    bra @switch_end25
@default29:
    .loc "test_mmemu_control.c", 17
    lda #100
    sta $26
    ldx #0
    stx $27
@switch_end25:
    lda $26
    ldx $27
    sta $20
    stx $21
    .loc "test_mmemu_control.c", 20
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end24:
    lda $22
    ldx $23
    sta $20
    .loc "test_mmemu_control.c", 70
    lda _results
    ldx _results+1
    sta $22
    stx $23
    lda #6
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx #0
    pha
    lda $24
    ldx $25
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
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "test_mmemu_control.c", 71
    lda #2
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_mmemu_control.c", 5
    lda #0
    sta $26
    sta $27
    .loc "test_mmemu_control.c", 6
    lda $20
    ldx $21
    cmp.16 .AX, #1
    beq @case36
@case_skip40:
    lda $22
    ldx $23
    cmp.16 .AX, #2
    beq @case37
@case_skip41:
    lda $22
    ldx $23
    cmp.16 .AX, #3
    beq @case38
    bra @default39
@case36:
    .loc "test_mmemu_control.c", 8
    lda #10
    sta $26
    ldx #0
    stx $27
    bra @switch_end35
@case37:
    .loc "test_mmemu_control.c", 11
    lda #20
    sta $26
    ldx #0
    stx $27
@case38:
    .loc "test_mmemu_control.c", 14
    lda $26
    clc
    adc #5
    sta $26
    lda $27
    adc #0
    sta $27
    bra @switch_end35
@default39:
    .loc "test_mmemu_control.c", 17
    lda #100
    sta $26
    ldx #0
    stx $27
@switch_end35:
    lda $26
    ldx $27
    sta $20
    stx $21
    .loc "test_mmemu_control.c", 20
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end34:
    lda $22
    ldx $23
    sta $20
    .loc "test_mmemu_control.c", 71
    lda _results
    ldx _results+1
    sta $22
    stx $23
    lda #7
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx #0
    pha
    lda $24
    ldx $25
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
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "test_mmemu_control.c", 72
    lda #4
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_mmemu_control.c", 5
    lda #0
    sta _main__local_87
    sta _main__local_87+1
    .loc "test_mmemu_control.c", 6
    lda $20
    ldx $21
    cmp.16 .AX, #1
    beq @case46
@case_skip50:
    lda $22
    ldx $23
    cmp.16 .AX, #2
    beq @case47
@case_skip51:
    lda $22
    ldx $23
    cmp.16 .AX, #3
    beq @case48
    bra @default49
@case46:
    .loc "test_mmemu_control.c", 8
    lda #10
    sta _main__local_87
    lda #0
    sta _main__local_87+1
    bra @switch_end45
@case47:
    .loc "test_mmemu_control.c", 11
    lda #20
    sta _main__local_87
    lda #0
    sta _main__local_87+1
@case48:
    .loc "test_mmemu_control.c", 14
    lda _main__local_87
    ldx _main__local_87+1
    add.16 .AX, #5
    sta $22
    stx $23
    sta _main__local_87
    stx _main__local_87+1
    bra @switch_end45
@default49:
    .loc "test_mmemu_control.c", 17
    lda #100
    sta _main__local_87
    lda #0
    sta _main__local_87+1
@switch_end45:
    lda _main__local_87
    ldx _main__local_87+1
    sta $20
    stx $21
    .loc "test_mmemu_control.c", 20
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end44:
    lda $22
    ldx $23
    sta $20
    .loc "test_mmemu_control.c", 72
    lda _results
    ldx _results+1
    sta $22
    stx $23
    lda #8
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx #0
    pha
    lda $24
    ldx $25
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
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "test_mmemu_control.c", 75
    lda #17
    sta $20
    lda _results
    ldx _results+1
    sta $22
    stx $23
    lda #9
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx #0
    pha
    lda $24
    ldx $25
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
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "test_mmemu_control.c", 76
    lda #34
    sta $20
    lda _results
    ldx _results+1
    sta $22
    stx $23
    lda #10
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx #0
    pha
    lda $24
    ldx $25
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
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "test_mmemu_control.c", 79
    lda #170
    sta $20
    lda _results
    ldx _results+1
    sta $22
    stx $23
    lda #0
    sta $24
    sta $25
    lda $20
    ldx #0
    pha
    lda $24
    ldx $25
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
    pla
    ldy #0
    sta (__zp_scratch),y
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 10
    endproc


__zp_save_buf:

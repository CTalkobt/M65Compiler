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

    .global _test_switch_continue
    .global _main

    .segment "code"

; function _test_switch_continue
; SAC inline storage: 4 bytes
    .global _test_switch_continue__param_val
    _test_switch_continue__param_val: .word 0
    _test_switch_continue__local_0: .word 0
    _test_switch_continue__local_1: .word 0
    proc _test_switch_continue, W#@_p_val
    .sac
    .var _fp = 0
    .loc "test_switch_continue.c", 1
    .local @_l_result = 2
; .debug_var: __test_switch_continue @_l_result offset=2 size=2 type=int16 scope=local
    .var @_p_val = 2
; .debug_var: __test_switch_continue @_p_val offset=2 size=2 type=int16 scope=parameter

@entry:
    .loc "test_switch_continue.c", 2
    lda #0
    sta _test_switch_continue__local_1
    sta _test_switch_continue__local_1+1
    .loc "test_switch_continue.c", 3
    lda _test_switch_continue__param_val
    ldx _test_switch_continue__param_val+1
    cmp.16 .AX, #1
    beq @case1
@case_skip5:
    lda _test_switch_continue__param_val
    ldx _test_switch_continue__param_val+1
    cmp.16 .AX, #2
    beq @case2
@case_skip6:
    lda _test_switch_continue__param_val
    ldx _test_switch_continue__param_val+1
    cmp.16 .AX, #3
    beq @case3
    bra @default4
@case1:
    .loc "test_switch_continue.c", 5
    inc.16f __vr1
@case2:
    .loc "test_switch_continue.c", 8
    lda _test_switch_continue__local_1
    ldx _test_switch_continue__local_1+1
    add.16 .AX, #10
    sta $22
    stx $23
    sta _test_switch_continue__local_1
    stx _test_switch_continue__local_1+1
    .loc "test_switch_continue.c", 9
    lda _test_switch_continue__param_val
    ldx _test_switch_continue__param_val+1
    cmp.16 .AX, #1
    beq @switch_end0
@if_end10:
@case3:
    .loc "test_switch_continue.c", 12
    lda #50
    sta _test_switch_continue__local_1
    lda #0
    sta _test_switch_continue__local_1+1
@default4:
    .loc "test_switch_continue.c", 17
    lda _test_switch_continue__local_1
    ldx _test_switch_continue__local_1+1
    add.16 .AX, #100
    sta $22
    stx $23
    sta _test_switch_continue__local_1
    stx _test_switch_continue__local_1+1
@switch_end0:
    .loc "test_switch_continue.c", 20
    lda _test_switch_continue__local_1
    ldx _test_switch_continue__local_1+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _main
; SAC inline storage: 8 bytes
    _main__local_60: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_switch_continue.c", 23
    .local @_l_result = 6
; .debug_var: __main @_l_result offset=6 size=2 type=int16 scope=local

@entry:
    .loc "test_switch_continue.c", 24
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_switch_continue.c", 2
    lda #0
    sta $26
    sta $27
    .loc "test_switch_continue.c", 3
    lda $20
    ldx $21
    cmp.16 .AX, #1
    beq @case16
@case_skip20:
    lda $22
    ldx $23
    cmp.16 .AX, #2
    beq @case17
@case_skip21:
    lda $22
    ldx $23
    cmp.16 .AX, #3
    beq @case18
    bra @default19
@case16:
    .loc "test_switch_continue.c", 5
    inc $26
    bne *+4
    inc $27
@case17:
    .loc "test_switch_continue.c", 8
    lda $26
    clc
    adc #10
    sta $26
    lda $27
    adc #0
    sta $27
    .loc "test_switch_continue.c", 9
    lda $22
    ldx $23
    cmp.16 .AX, #1
    beq @switch_end15
@if_end25:
@case18:
    .loc "test_switch_continue.c", 12
    lda #50
    sta $26
    ldx #0
    stx $27
@default19:
    .loc "test_switch_continue.c", 17
    lda $26
    clc
    adc #100
    sta $26
    lda $27
    adc #0
    sta $27
@switch_end15:
    .loc "test_switch_continue.c", 20
    lda $26
    ldx $27
    sta $20
    stx $21
@inline_end14:
    .loc "test_switch_continue.c", 24
    lda $20
    ldx $21
    cmp.16 .AX, #11
    bne @if_then11
    bra @if_end13
@if_then11:
    lda #1
    ldx #0
    bra @__return
@if_end13:
    .loc "test_switch_continue.c", 25
    lda #2
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_switch_continue.c", 2
    lda #0
    sta $26
    sta $27
    .loc "test_switch_continue.c", 3
    lda $20
    ldx $21
    cmp.16 .AX, #1
    beq @case32
@case_skip36:
    lda $22
    ldx $23
    cmp.16 .AX, #2
    beq @case33
@case_skip37:
    lda $22
    ldx $23
    cmp.16 .AX, #3
    beq @case34
    bra @default35
@case32:
    .loc "test_switch_continue.c", 5
    inc $26
    bne *+4
    inc $27
@case33:
    .loc "test_switch_continue.c", 8
    lda $26
    clc
    adc #10
    sta $26
    lda $27
    adc #0
    sta $27
    .loc "test_switch_continue.c", 9
    lda $22
    ldx $23
    cmp.16 .AX, #1
    beq @switch_end31
@if_end41:
@case34:
    .loc "test_switch_continue.c", 12
    lda #50
    sta $26
    ldx #0
    stx $27
@default35:
    .loc "test_switch_continue.c", 17
    lda $26
    clc
    adc #100
    sta $26
    lda $27
    adc #0
    sta $27
@switch_end31:
    .loc "test_switch_continue.c", 20
    lda $26
    ldx $27
    sta $20
    stx $21
@inline_end30:
    .loc "test_switch_continue.c", 25
    lda $20
    ldx $21
    cmp.16 .AX, #110
    bne @if_then27
    bra @if_end29
@if_then27:
    lda #2
    ldx #0
    bra @__return
@if_end29:
    .loc "test_switch_continue.c", 26
    lda #3
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_switch_continue.c", 2
    lda #0
    sta $26
    sta $27
    .loc "test_switch_continue.c", 3
    lda $20
    ldx $21
    cmp.16 .AX, #1
    beq @case48
@case_skip52:
    lda $22
    ldx $23
    cmp.16 .AX, #2
    beq @case49
@case_skip53:
    lda $22
    ldx $23
    cmp.16 .AX, #3
    beq @case50
    bra @default51
@case48:
    .loc "test_switch_continue.c", 5
    inc $26
    bne *+4
    inc $27
@case49:
    .loc "test_switch_continue.c", 8
    lda $26
    clc
    adc #10
    sta $26
    lda $27
    adc #0
    sta $27
    .loc "test_switch_continue.c", 9
    lda $22
    ldx $23
    cmp.16 .AX, #1
    beq @switch_end47
@if_end57:
@case50:
    .loc "test_switch_continue.c", 12
    lda #50
    sta $26
    ldx #0
    stx $27
@default51:
    .loc "test_switch_continue.c", 17
    lda $26
    clc
    adc #100
    sta $26
    lda $27
    adc #0
    sta $27
@switch_end47:
    .loc "test_switch_continue.c", 20
    lda $26
    ldx $27
    sta $20
    stx $21
@inline_end46:
    .loc "test_switch_continue.c", 26
    lda $20
    ldx $21
    cmp.16 .AX, #161
    bne @if_then43
    bra @if_end45
@if_then43:
    lda #3
    ldx #0
    bra @__return
@if_end45:
    .loc "test_switch_continue.c", 27
    lda #4
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_switch_continue.c", 2
    lda #0
    sta _main__local_60
    sta _main__local_60+1
    .loc "test_switch_continue.c", 3
    lda $20
    ldx $21
    cmp.16 .AX, #1
    beq @case64
@case_skip68:
    lda $22
    ldx $23
    cmp.16 .AX, #2
    beq @case65
@case_skip69:
    lda $22
    ldx $23
    cmp.16 .AX, #3
    beq @case66
    bra @default67
@case64:
    .loc "test_switch_continue.c", 5
    inc.16f __vr60
@case65:
    .loc "test_switch_continue.c", 8
    lda _main__local_60
    ldx _main__local_60+1
    add.16 .AX, #10
    sta $24
    stx $25
    sta _main__local_60
    stx _main__local_60+1
    .loc "test_switch_continue.c", 9
    lda $22
    ldx $23
    cmp.16 .AX, #1
    beq @switch_end63
@if_end73:
@case66:
    .loc "test_switch_continue.c", 12
    lda #50
    sta _main__local_60
    lda #0
    sta _main__local_60+1
@default67:
    .loc "test_switch_continue.c", 17
    lda _main__local_60
    ldx _main__local_60+1
    add.16 .AX, #100
    sta $22
    stx $23
    sta _main__local_60
    stx _main__local_60+1
@switch_end63:
    .loc "test_switch_continue.c", 20
    lda _main__local_60
    ldx _main__local_60+1
    sta $20
    stx $21
@inline_end62:
    .loc "test_switch_continue.c", 27
    lda $20
    ldx $21
    cmp.16 .AX, #100
    bne @if_then59
    bra @if_end61
@if_then59:
    lda #4
    ldx #0
    bra @__return
@if_end61:
    .loc "test_switch_continue.c", 28
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 8
    endproc


__zp_save_buf:

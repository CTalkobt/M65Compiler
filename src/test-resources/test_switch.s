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

    .global _test_switch
    .global _main

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
    .loc "test_switch.c", 1
    .local @_l_result = 2
; .debug_var: __test_switch @_l_result offset=2 size=2 type=int16 scope=local
    .var @_p_val = 2
; .debug_var: __test_switch @_p_val offset=2 size=2 type=int16 scope=parameter

@entry:
    .loc "test_switch.c", 2
    lda #0
    sta _test_switch__local_1
    sta _test_switch__local_1+1
    .loc "test_switch.c", 3
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
    .loc "test_switch.c", 5
    lda #10
    sta _test_switch__local_1
    lda #0
    sta _test_switch__local_1+1
    bra @switch_end0
@case2:
    .loc "test_switch.c", 8
    lda #20
    sta _test_switch__local_1
    lda #0
    sta _test_switch__local_1+1
@case3:
    .loc "test_switch.c", 11
    lda _test_switch__local_1
    ldx _test_switch__local_1+1
    add.16 .AX, #5
    sta $22
    stx $23
    sta _test_switch__local_1
    stx _test_switch__local_1+1
    bra @switch_end0
@default4:
    .loc "test_switch.c", 14
    lda #100
    sta _test_switch__local_1
    lda #0
    sta _test_switch__local_1+1
@switch_end0:
    .loc "test_switch.c", 17
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
; SAC inline storage: 8 bytes
    _main__local_48: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_switch.c", 20
    .local @_l_result = 6
; .debug_var: __main @_l_result offset=6 size=2 type=int16 scope=local

@entry:
    .loc "test_switch.c", 21
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_switch.c", 2
    lda #0
    sta $26
    sta $27
    .loc "test_switch.c", 3
    lda $20
    ldx $21
    cmp.16 .AX, #1
    beq @case13
@case_skip17:
    lda $22
    ldx $23
    cmp.16 .AX, #2
    beq @case14
@case_skip18:
    lda $22
    ldx $23
    cmp.16 .AX, #3
    beq @case15
    bra @default16
@case13:
    .loc "test_switch.c", 5
    lda #10
    sta $26
    ldx #0
    stx $27
    bra @switch_end12
@case14:
    .loc "test_switch.c", 8
    lda #20
    sta $26
    ldx #0
    stx $27
@case15:
    .loc "test_switch.c", 11
    lda $26
    clc
    adc #5
    sta $26
    lda $27
    adc #0
    sta $27
    bra @switch_end12
@default16:
    .loc "test_switch.c", 14
    lda #100
    sta $26
    ldx #0
    stx $27
@switch_end12:
    .loc "test_switch.c", 17
    lda $26
    ldx $27
    sta $20
    stx $21
@inline_end11:
    .loc "test_switch.c", 21
    lda $20
    ldx $21
    cmp.16 .AX, #10
    bne @if_then8
    bra @if_end10
@if_then8:
    lda #1
    ldx #0
    bra @__return
@if_end10:
    .loc "test_switch.c", 22
    lda #2
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_switch.c", 2
    lda #0
    sta $26
    sta $27
    .loc "test_switch.c", 3
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
    .loc "test_switch.c", 5
    lda #10
    sta $26
    ldx #0
    stx $27
    bra @switch_end25
@case27:
    .loc "test_switch.c", 8
    lda #20
    sta $26
    ldx #0
    stx $27
@case28:
    .loc "test_switch.c", 11
    lda $26
    clc
    adc #5
    sta $26
    lda $27
    adc #0
    sta $27
    bra @switch_end25
@default29:
    .loc "test_switch.c", 14
    lda #100
    sta $26
    ldx #0
    stx $27
@switch_end25:
    .loc "test_switch.c", 17
    lda $26
    ldx $27
    sta $20
    stx $21
@inline_end24:
    .loc "test_switch.c", 22
    lda $20
    ldx $21
    cmp.16 .AX, #25
    bne @if_then21
    bra @if_end23
@if_then21:
    lda #2
    ldx #0
    bra @__return
@if_end23:
    .loc "test_switch.c", 23
    lda #3
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_switch.c", 2
    lda #0
    sta $26
    sta $27
    .loc "test_switch.c", 3
    lda $20
    ldx $21
    cmp.16 .AX, #1
    beq @case39
@case_skip43:
    lda $22
    ldx $23
    cmp.16 .AX, #2
    beq @case40
@case_skip44:
    lda $22
    ldx $23
    cmp.16 .AX, #3
    beq @case41
    bra @default42
@case39:
    .loc "test_switch.c", 5
    lda #10
    sta $26
    ldx #0
    stx $27
    bra @switch_end38
@case40:
    .loc "test_switch.c", 8
    lda #20
    sta $26
    ldx #0
    stx $27
@case41:
    .loc "test_switch.c", 11
    lda $26
    clc
    adc #5
    sta $26
    lda $27
    adc #0
    sta $27
    bra @switch_end38
@default42:
    .loc "test_switch.c", 14
    lda #100
    sta $26
    ldx #0
    stx $27
@switch_end38:
    .loc "test_switch.c", 17
    lda $26
    ldx $27
    sta $20
    stx $21
@inline_end37:
    .loc "test_switch.c", 23
    lda $20
    ldx $21
    cmp.16 .AX, #5
    bne @if_then34
    bra @if_end36
@if_then34:
    lda #3
    ldx #0
    bra @__return
@if_end36:
    .loc "test_switch.c", 24
    lda #4
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_switch.c", 2
    lda #0
    sta _main__local_48
    sta _main__local_48+1
    .loc "test_switch.c", 3
    lda $20
    ldx $21
    cmp.16 .AX, #1
    beq @case52
@case_skip56:
    lda $22
    ldx $23
    cmp.16 .AX, #2
    beq @case53
@case_skip57:
    lda $22
    ldx $23
    cmp.16 .AX, #3
    beq @case54
    bra @default55
@case52:
    .loc "test_switch.c", 5
    lda #10
    sta _main__local_48
    lda #0
    sta _main__local_48+1
    bra @switch_end51
@case53:
    .loc "test_switch.c", 8
    lda #20
    sta _main__local_48
    lda #0
    sta _main__local_48+1
@case54:
    .loc "test_switch.c", 11
    lda _main__local_48
    ldx _main__local_48+1
    add.16 .AX, #5
    sta $22
    stx $23
    sta _main__local_48
    stx _main__local_48+1
    bra @switch_end51
@default55:
    .loc "test_switch.c", 14
    lda #100
    sta _main__local_48
    lda #0
    sta _main__local_48+1
@switch_end51:
    .loc "test_switch.c", 17
    lda _main__local_48
    ldx _main__local_48+1
    sta $20
    stx $21
@inline_end50:
    .loc "test_switch.c", 24
    lda $20
    ldx $21
    cmp.16 .AX, #100
    bne @if_then47
    bra @if_end49
@if_then47:
    lda #4
    ldx #0
    bra @__return
@if_end49:
    .loc "test_switch.c", 25
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

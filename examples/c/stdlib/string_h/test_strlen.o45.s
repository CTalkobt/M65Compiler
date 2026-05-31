    .o45
    .extern __sp_base
    __zp_scratch = $08
    __zp_scratch2 = $0A
    __zp_scratch3 = $0C
    __zp_scratch4 = $0E

    .extern _strlen

    .global _main

    .segment "code"

; function _main
    proc _main
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/string.h", 3
; frame: 8 bytes (frame-allocated vRegs only)
    phw #0
    phw #0
    phw #0
    phw #0
    .local __vr0 = 0
    .local __vr4 = 2
    .local __vr8 = 4
    .local __vr12 = 6

@entry:
    .loc "test_strlen.c", 8
    ldax #__str_3
    stax.fp __vr0
    ldax.fp __vr0
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _strlen
    .var _fp = _fp - 2
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #5
    bne @if_then0
    bra @if_end2
@if_then0:
    lda #1
    ldx #0
    bra @__return
@if_end2:
    .loc "test_strlen.c", 9
    ldax #__str_7
    stax.fp __vr4
    ldax.fp __vr4
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _strlen
    .var _fp = _fp - 2
    sta $20
    stx $21
    lda $20
    ora $21
    bne @if_then4
    bra @if_end6
@if_then4:
    lda #2
    ldx #0
    bra @__return
@if_end6:
    .loc "test_strlen.c", 10
    ldax #__str_11
    stax.fp __vr8
    ldax.fp __vr8
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _strlen
    .var _fp = _fp - 2
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #1
    bne @if_then8
    bra @if_end10
@if_then8:
    lda #3
    ldx #0
    bra @__return
@if_end10:
    .loc "test_strlen.c", 11
    ldax #__str_15
    stax.fp __vr12
    ldax.fp __vr12
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _strlen
    .var _fp = _fp - 2
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #10
    bne @if_then12
    bra @if_end14
@if_then12:
    lda #4
    ldx #0
    bra @__return
@if_end14:
    .loc "test_strlen.c", 12
    lda #0
    ldx #0
@__return:
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    .func_flags stack_call
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc


    .segment "data"
__str_3:
    .text "hello"
    .byte 0
__str_7:
    .text ""
    .byte 0
__str_11:
    .text "a"
    .byte 0
__str_15:
    .text "abcdefghij"
    .byte 0

__zp_save_buf:

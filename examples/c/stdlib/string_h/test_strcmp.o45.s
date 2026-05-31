    .o45
    .extern __sp_base
    __zp_scratch = $08
    __zp_scratch2 = $0A
    __zp_scratch3 = $0C
    __zp_scratch4 = $0E

    .extern _strcmp

    .global _main

    .segment "code"

; function _main
    proc _main
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/string.h", 3
; frame: 24 bytes (frame-allocated vRegs only)
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    .local __vr0 = 0
    .local __vr1 = 2
    .local __vr5 = 4
    .local __vr6 = 6
    .local __vr10 = 8
    .local __vr11 = 10
    .local __vr15 = 12
    .local __vr16 = 14
    .local __vr20 = 16
    .local __vr21 = 18
    .local __vr25 = 20
    .local __vr26 = 22

@entry:
    .loc "test_strcmp.c", 9
    ldax #__str_3
    stax.fp __vr0
    ldax #__str_4
    stax.fp __vr1
    ldax.fp __vr1
    sta $28
    stx $29
    ldax.fp __vr0
    sta $2C
    stx $2D
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    lda $2C
    ldx $2D
    push .ax
    .var _fp = _fp + 2
    jsr _strcmp
    .var _fp = _fp - 4
    sta $20
    stx $21
    lda $20
    ora $21
    bne @if_then0
    bra @if_end2
@if_then0:
    lda #1
    ldx #0
    bra @__return
@if_end2:
    .loc "test_strcmp.c", 10
    ldax #__str_8
    stax.fp __vr5
    ldax #__str_9
    stax.fp __vr6
    ldax.fp __vr6
    sta $28
    stx $29
    ldax.fp __vr5
    sta $2C
    stx $2D
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    lda $2C
    ldx $2D
    push .ax
    .var _fp = _fp + 2
    jsr _strcmp
    .var _fp = _fp - 4
    sta $20
    stx $21
    lda $20
    ora $21
    bne @if_then5
    bra @if_end7
@if_then5:
    lda #2
    ldx #0
    bra @__return
@if_end7:
    .loc "test_strcmp.c", 13
    ldax #__str_13
    stax.fp __vr10
    ldax #__str_14
    stax.fp __vr11
    ldax.fp __vr11
    sta $28
    stx $29
    ldax.fp __vr10
    sta $2C
    stx $2D
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    lda $2C
    ldx $2D
    push .ax
    .var _fp = _fp + 2
    jsr _strcmp
    .var _fp = _fp - 4
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #0
    bcs @if_then10
    bra @if_end12
@if_then10:
    lda #3
    ldx #0
    bra @__return
@if_end12:
    .loc "test_strcmp.c", 14
    ldax #__str_18
    stax.fp __vr15
    ldax #__str_19
    stax.fp __vr16
    ldax.fp __vr16
    sta $28
    stx $29
    ldax.fp __vr15
    sta $2C
    stx $2D
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    lda $2C
    ldx $2D
    push .ax
    .var _fp = _fp + 2
    jsr _strcmp
    .var _fp = _fp - 4
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #0
    bcc @if_then15
    beq @if_then15
    bra @if_end17
@if_then15:
    lda #4
    ldx #0
    bra @__return
@if_end17:
    .loc "test_strcmp.c", 17
    ldax #__str_23
    stax.fp __vr20
    ldax #__str_24
    stax.fp __vr21
    ldax.fp __vr21
    sta $28
    stx $29
    ldax.fp __vr20
    sta $2C
    stx $2D
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    lda $2C
    ldx $2D
    push .ax
    .var _fp = _fp + 2
    jsr _strcmp
    .var _fp = _fp - 4
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #0
    bcs @if_then20
    bra @if_end22
@if_then20:
    lda #5
    ldx #0
    bra @__return
@if_end22:
    .loc "test_strcmp.c", 18
    ldax #__str_28
    stax.fp __vr25
    ldax #__str_29
    stax.fp __vr26
    ldax.fp __vr26
    sta $28
    stx $29
    ldax.fp __vr25
    sta $2C
    stx $2D
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    lda $2C
    ldx $2D
    push .ax
    .var _fp = _fp + 2
    jsr _strcmp
    .var _fp = _fp - 4
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #0
    bcc @if_then25
    beq @if_then25
    bra @if_end27
@if_then25:
    lda #6
    ldx #0
    bra @__return
@if_end27:
    .loc "test_strcmp.c", 20
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
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
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
    .text "abc"
    .byte 0
__str_4:
    .text "abc"
    .byte 0
__str_8:
    .text ""
    .byte 0
__str_9:
    .text ""
    .byte 0
__str_13:
    .text "abc"
    .byte 0
__str_14:
    .text "abd"
    .byte 0
__str_18:
    .text "abd"
    .byte 0
__str_19:
    .text "abc"
    .byte 0
__str_23:
    .text "ab"
    .byte 0
__str_24:
    .text "abc"
    .byte 0
__str_28:
    .text "abc"
    .byte 0
__str_29:
    .text "ab"
    .byte 0

__zp_save_buf:

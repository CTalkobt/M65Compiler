    .o45
    .extern __sp_base
    __zp_scratch = $08
    __zp_scratch2 = $0A
    __zp_scratch3 = $0C
    __zp_scratch4 = $0E

    .extern _atoi

    .global _main

    .segment "code"

; function _main
    proc _main
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdlib.h", 3
; frame: 18 bytes (frame-allocated vRegs only)
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
    .local __vr4 = 2
    .local __vr8 = 4
    .local __vr12 = 6
    .local __vr16 = 8
    .local __vr20 = 10
    .local __vr24 = 12
    .local __vr28 = 14
    .local __vr32 = 16

@entry:
    .loc "test_atoi.c", 9
    ldax #__str_3
    stax.fp __vr0
    ldax.fp __vr0
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _atoi
    .var _fp = _fp - 2
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
    .loc "test_atoi.c", 10
    ldax #__str_7
    stax.fp __vr4
    ldax.fp __vr4
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _atoi
    .var _fp = _fp - 2
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #1
    bne @if_then4
    bra @if_end6
@if_then4:
    lda #2
    ldx #0
    bra @__return
@if_end6:
    .loc "test_atoi.c", 11
    ldax #__str_11
    stax.fp __vr8
    ldax.fp __vr8
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _atoi
    .var _fp = _fp - 2
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #123
    bne @if_then8
    bra @if_end10
@if_then8:
    lda #3
    ldx #0
    bra @__return
@if_end10:
    .loc "test_atoi.c", 12
    ldax #__str_15
    stax.fp __vr12
    ldax.fp __vr12
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _atoi
    .var _fp = _fp - 2
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #65535
    bne @if_then12
    bra @if_end14
@if_then12:
    lda #4
    ldx #0
    bra @__return
@if_end14:
    .loc "test_atoi.c", 15
    ldax #__str_19
    stax.fp __vr16
    ldax.fp __vr16
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _atoi
    .var _fp = _fp - 2
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #42
    bne @if_then16
    bra @if_end18
@if_then16:
    lda #5
    ldx #0
    bra @__return
@if_end18:
    .loc "test_atoi.c", 18
    ldax #__str_23
    stax.fp __vr20
    ldax.fp __vr20
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _atoi
    .var _fp = _fp - 2
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #7
    bne @if_then20
    bra @if_end22
@if_then20:
    lda #6
    ldx #0
    bra @__return
@if_end22:
    .loc "test_atoi.c", 21
    ldax #__str_27
    stax.fp __vr24
    ldax.fp __vr24
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _atoi
    .var _fp = _fp - 2
    sta $20
    stx $21
    lda $20
    ora $21
    bne @if_then24
    bra @if_end26
@if_then24:
    lda #7
    ldx #0
    bra @__return
@if_end26:
    .loc "test_atoi.c", 22
    ldax #__str_31
    stax.fp __vr28
    ldax.fp __vr28
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _atoi
    .var _fp = _fp - 2
    sta $20
    stx $21
    lda $20
    ora $21
    bne @if_then28
    bra @if_end30
@if_then28:
    lda #8
    ldx #0
    bra @__return
@if_end30:
    .loc "test_atoi.c", 25
    ldax #__str_35
    stax.fp __vr32
    ldax.fp __vr32
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _atoi
    .var _fp = _fp - 2
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #99
    bne @if_then32
    bra @if_end34
@if_then32:
    lda #9
    ldx #0
    bra @__return
@if_end34:
    .loc "test_atoi.c", 27
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
    .func_flags stack_call
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc


    .segment "data"
__str_3:
    .text "0"
    .byte 0
__str_7:
    .text "1"
    .byte 0
__str_11:
    .text "123"
    .byte 0
__str_15:
    .text "65535"
    .byte 0
__str_19:
    .text "  42"
    .byte 0
__str_23:
    .text "+7"
    .byte 0
__str_27:
    .text ""
    .byte 0
__str_31:
    .text "abc"
    .byte 0
__str_35:
    .text "99xyz"
    .byte 0

__zp_save_buf:

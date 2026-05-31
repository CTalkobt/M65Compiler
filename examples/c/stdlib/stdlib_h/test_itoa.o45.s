    .o45
    .extern __sp_base
    __zp_scratch = $08
    __zp_scratch2 = $0A
    __zp_scratch3 = $0C
    __zp_scratch4 = $0E

    .extern _itoa
    .extern _strcmp

    .global _buf
    .global _main

    .segment "bss"
_buf:
    .res 20

    .segment "code"

; function _main
    proc _main
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stddef.h", 1
; frame: 16 bytes (frame-allocated vRegs only)
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    .local __vr3 = 0
    .local __vr10 = 2
    .local __vr17 = 4
    .local __vr24 = 6
    .local __vr31 = 8
    .local __vr38 = 10
    .local __vr45 = 12
    .local __vr52 = 14

@entry:
    .loc "test_itoa.c", 12
    phw #10
    .var _fp = _fp + 2
    phw #_buf
    .var _fp = _fp + 2
    phw #0
    .var _fp = _fp + 2
    jsr _itoa
    .var _fp = _fp - 6
    .loc "test_itoa.c", 13
    ldax #__str_3
    stax.fp __vr3
    ldax.fp __vr3
    sta $28
    stx $29
    ldax #_buf
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
    .loc "test_itoa.c", 15
    phw #10
    .var _fp = _fp + 2
    phw #_buf
    .var _fp = _fp + 2
    phw #1
    .var _fp = _fp + 2
    jsr _itoa
    .var _fp = _fp - 6
    .loc "test_itoa.c", 16
    ldax #__str_7
    stax.fp __vr10
    ldax.fp __vr10
    sta $28
    stx $29
    ldax #_buf
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
    bne @if_then4
    bra @if_end6
@if_then4:
    lda #2
    ldx #0
    bra @__return
@if_end6:
    .loc "test_itoa.c", 18
    phw #10
    .var _fp = _fp + 2
    phw #_buf
    .var _fp = _fp + 2
    phw #123
    .var _fp = _fp + 2
    jsr _itoa
    .var _fp = _fp - 6
    .loc "test_itoa.c", 19
    ldax #__str_11
    stax.fp __vr17
    ldax.fp __vr17
    sta $28
    stx $29
    ldax #_buf
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
    bne @if_then8
    bra @if_end10
@if_then8:
    lda #3
    ldx #0
    bra @__return
@if_end10:
    .loc "test_itoa.c", 21
    phw #10
    .var _fp = _fp + 2
    phw #_buf
    .var _fp = _fp + 2
    phw #65535
    .var _fp = _fp + 2
    jsr _itoa
    .var _fp = _fp - 6
    .loc "test_itoa.c", 22
    ldax #__str_15
    stax.fp __vr24
    ldax.fp __vr24
    sta $28
    stx $29
    ldax #_buf
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
    bne @if_then12
    bra @if_end14
@if_then12:
    lda #4
    ldx #0
    bra @__return
@if_end14:
    .loc "test_itoa.c", 25
    phw #16
    .var _fp = _fp + 2
    phw #_buf
    .var _fp = _fp + 2
    phw #255
    .var _fp = _fp + 2
    jsr _itoa
    .var _fp = _fp - 6
    .loc "test_itoa.c", 26
    ldax #__str_19
    stax.fp __vr31
    ldax.fp __vr31
    sta $28
    stx $29
    ldax #_buf
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
    bne @if_then16
    bra @if_end18
@if_then16:
    lda #5
    ldx #0
    bra @__return
@if_end18:
    .loc "test_itoa.c", 28
    phw #16
    .var _fp = _fp + 2
    phw #_buf
    .var _fp = _fp + 2
    phw #4096
    .var _fp = _fp + 2
    jsr _itoa
    .var _fp = _fp - 6
    .loc "test_itoa.c", 29
    ldax #__str_23
    stax.fp __vr38
    ldax.fp __vr38
    sta $28
    stx $29
    ldax #_buf
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
    bne @if_then20
    bra @if_end22
@if_then20:
    lda #6
    ldx #0
    bra @__return
@if_end22:
    .loc "test_itoa.c", 32
    phw #2
    .var _fp = _fp + 2
    phw #_buf
    .var _fp = _fp + 2
    phw #10
    .var _fp = _fp + 2
    jsr _itoa
    .var _fp = _fp - 6
    .loc "test_itoa.c", 33
    ldax #__str_27
    stax.fp __vr45
    ldax.fp __vr45
    sta $28
    stx $29
    ldax #_buf
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
    bne @if_then24
    bra @if_end26
@if_then24:
    lda #7
    ldx #0
    bra @__return
@if_end26:
    .loc "test_itoa.c", 36
    phw #8
    .var _fp = _fp + 2
    phw #_buf
    .var _fp = _fp + 2
    phw #7
    .var _fp = _fp + 2
    jsr _itoa
    .var _fp = _fp - 6
    .loc "test_itoa.c", 37
    ldax #__str_31
    stax.fp __vr52
    ldax.fp __vr52
    sta $28
    stx $29
    ldax #_buf
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
    bne @if_then28
    bra @if_end30
@if_then28:
    lda #8
    ldx #0
    bra @__return
@if_end30:
    .loc "test_itoa.c", 39
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
    .text "ff"
    .byte 0
__str_23:
    .text "1000"
    .byte 0
__str_27:
    .text "1010"
    .byte 0
__str_31:
    .text "7"
    .byte 0

__zp_save_buf:

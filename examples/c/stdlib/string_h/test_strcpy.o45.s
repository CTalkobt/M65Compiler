    .o45
    .extern __sp_base
    __zp_scratch = $08
    __zp_scratch2 = $0A
    __zp_scratch3 = $0C
    __zp_scratch4 = $0E

    .extern _strcmp
    .extern _strcpy
    .extern _strlen

    .global _buf
    .global _main

    .segment "bss"
_buf:
    .res 16

    .segment "code"

; function _main
    proc _main
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/string.h", 5
; frame: 6 bytes (frame-allocated vRegs only)
    phw #0
    phw #0
    phw #0
    .local __vr0 = 0
    .local __vr2 = 2
    .local __vr9 = 4

@entry:
    .loc "test_strcpy.c", 11
    ldax #__str_0
    stax.fp __vr0
    ldax.fp __vr0
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
    jsr _strcpy
    .var _fp = _fp - 4
    .loc "test_strcpy.c", 12
    ldax #__str_4
    stax.fp __vr2
    ldax.fp __vr2
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
    bne @if_then1
    bra @if_end3
@if_then1:
    lda #1
    ldx #0
    bra @__return
@if_end3:
    .loc "test_strcpy.c", 13
    phw #_buf
    .var _fp = _fp + 2
    jsr _strlen
    .var _fp = _fp - 2
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #5
    bne @if_then5
    bra @if_end7
@if_then5:
    lda #2
    ldx #0
    bra @__return
@if_end7:
    .loc "test_strcpy.c", 16
    ldax #__str_8
    stax.fp __vr9
    ldax.fp __vr9
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
    jsr _strcpy
    .var _fp = _fp - 4
    .loc "test_strcpy.c", 17
    phw #_buf
    .var _fp = _fp + 2
    jsr _strlen
    .var _fp = _fp - 2
    sta $20
    stx $21
    lda $20
    ora $21
    bne @if_then9
    bra @if_end11
@if_then9:
    lda #3
    ldx #0
    bra @__return
@if_end11:
    .loc "test_strcpy.c", 19
    lda #0
    ldx #0
@__return:
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
__str_0:
    .text "hello"
    .byte 0
__str_4:
    .text "hello"
    .byte 0
__str_8:
    .text ""
    .byte 0

__zp_save_buf:

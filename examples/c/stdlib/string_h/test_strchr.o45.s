    .o45
    .extern __sp_base
    __zp_scratch = $08
    __zp_scratch2 = $0A
    __zp_scratch3 = $0C
    __zp_scratch4 = $0E

    .extern _strchr
    .extern _strrchr

    .global _msg
    .global _main

    .segment "bss"
_msg:
    .res 8

    .segment "code"

; function _main
    proc _main
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/string.h", 5
; frame: 2 bytes (frame-allocated vRegs only)
    phw #0
    .local __vr18 = 0
    .local @_l_p = 0

@entry:
    .loc "test_strchr.c", 10
    lda #65
    sta $20
    lda #0
    sta $22
    sta $23
    lda #<_msg
    ldx #>_msg
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $22
    ldx $23
    clc
    adc __zp_scratch2
    sta $24
    txa
    adc __zp_scratch2+1
    sta $25
    lda $20
    ldy #0
    sta ($24),y
    .loc "test_strchr.c", 11
    lda #66
    sta $20
    lda #1
    ldx #0
    sta $22
    stx $23
    lda #<_msg
    ldx #>_msg
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $22
    ldx $23
    clc
    adc __zp_scratch2
    sta $24
    txa
    adc __zp_scratch2+1
    sta $25
    lda $20
    ldy #0
    sta ($24),y
    .loc "test_strchr.c", 12
    lda #67
    sta $20
    lda #2
    ldx #0
    sta $22
    stx $23
    lda #<_msg
    ldx #>_msg
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $22
    ldx $23
    clc
    adc __zp_scratch2
    sta $24
    txa
    adc __zp_scratch2+1
    sta $25
    lda $20
    ldy #0
    sta ($24),y
    .loc "test_strchr.c", 13
    lda #66
    sta $20
    lda #3
    ldx #0
    sta $22
    stx $23
    lda #<_msg
    ldx #>_msg
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $22
    ldx $23
    clc
    adc __zp_scratch2
    sta $24
    txa
    adc __zp_scratch2+1
    sta $25
    lda $20
    ldy #0
    sta ($24),y
    .loc "test_strchr.c", 14
    lda #68
    sta $20
    lda #4
    ldx #0
    sta $22
    stx $23
    lda #<_msg
    ldx #>_msg
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $22
    ldx $23
    clc
    adc __zp_scratch2
    sta $24
    txa
    adc __zp_scratch2+1
    sta $25
    lda $20
    ldy #0
    sta ($24),y
    .loc "test_strchr.c", 15
    lda #0
    sta $20
    lda #5
    ldx #0
    sta $22
    stx $23
    lda #<_msg
    ldx #>_msg
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $22
    ldx $23
    clc
    adc __zp_scratch2
    sta $24
    txa
    adc __zp_scratch2+1
    sta $25
    lda $20
    ldy #0
    sta ($24),y
    .loc "test_strchr.c", 18
    phw #66
    .var _fp = _fp + 2
    phw #_msg
    .var _fp = _fp + 2
    jsr _strchr
    .var _fp = _fp - 4
    sta $20
    stx $21
    lda $20
    ldx $21
    stax.fp __vr18
    .loc "test_strchr.c", 19
    ldax.fp __vr18
    stx __zp_scratch
    ora __zp_scratch
    beq @if_then0
    bra @if_end2
@if_then0:
    lda #1
    ldx #0
    bra @__return
@if_end2:
    .loc "test_strchr.c", 20
    ldax #_msg
    add.16 .AX, #1
    sta $22
    stx $23
    ldax.fp __vr18
    cmp.16 .AX, $22
    bne @if_then3
    bra @if_end5
@if_then3:
    lda #2
    ldx #0
    bra @__return
@if_end5:
    .loc "test_strchr.c", 23
    phw #90
    .var _fp = _fp + 2
    phw #_msg
    .var _fp = _fp + 2
    jsr _strchr
    .var _fp = _fp - 4
    sta $20
    stx $21
    lda $20
    ora $21
    bne @if_then6
    bra @if_end8
@if_then6:
    lda #3
    ldx #0
    bra @__return
@if_end8:
    .loc "test_strchr.c", 26
    phw #66
    .var _fp = _fp + 2
    phw #_msg
    .var _fp = _fp + 2
    jsr _strrchr
    .var _fp = _fp - 4
    sta $20
    stx $21
    lda $20
    ldx $21
    stax.fp __vr18
    .loc "test_strchr.c", 27
    ldax.fp __vr18
    stx __zp_scratch
    ora __zp_scratch
    beq @if_then9
    bra @if_end11
@if_then9:
    lda #4
    ldx #0
    bra @__return
@if_end11:
    .loc "test_strchr.c", 28
    ldax #_msg
    add.16 .AX, #3
    sta $22
    stx $23
    ldax.fp __vr18
    cmp.16 .AX, $22
    bne @if_then12
    bra @if_end14
@if_then12:
    lda #5
    ldx #0
    bra @__return
@if_end14:
    .loc "test_strchr.c", 30
    lda #0
    ldx #0
@__return:
    plz
    plz
    .func_flags stack_call
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc


__zp_save_buf:

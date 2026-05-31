    .o45
    .extern __sp_base
    __zp_scratch = $08
    __zp_scratch2 = $0A
    __zp_scratch3 = $0C
    __zp_scratch4 = $0E

    .extern _memcmp
    .extern _memcpy
    .extern _memset

    .global _src
    .global _dst
    .global _main

    .segment "bss"
_src:
    .res 8
_dst:
    .res 8

    .segment "code"

; function _main
    proc _main
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/string.h", 6

@entry:
    .loc "test_memops.c", 12
    phw #4
    .var _fp = _fp + 2
    phw #66
    .var _fp = _fp + 2
    phw #_src
    .var _fp = _fp + 2
    jsr _memset
    .var _fp = _fp - 6
    .loc "test_memops.c", 13
    lda #0
    sta $20
    sta $21
    lda #<_src
    ldx #>_src
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx $21
    clc
    adc __zp_scratch2
    sta $22
    txa
    adc __zp_scratch2+1
    sta $23
    ldy #0
    lda ($22),y
    ldx #0
    sta $20
    lda $20
    ldx #0
    cmp #66
    bne @if_then0
    bra @if_end2
@if_then0:
    lda #1
    ldx #0
    bra @__return
@if_end2:
    .loc "test_memops.c", 14
    lda #3
    ldx #0
    sta $20
    stx $21
    lda #<_src
    ldx #>_src
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx $21
    clc
    adc __zp_scratch2
    sta $22
    txa
    adc __zp_scratch2+1
    sta $23
    ldy #0
    lda ($22),y
    ldx #0
    sta $20
    lda $20
    ldx #0
    cmp #66
    bne @if_then3
    bra @if_end5
@if_then3:
    lda #2
    ldx #0
    bra @__return
@if_end5:
    .loc "test_memops.c", 17
    phw #8
    .var _fp = _fp + 2
    phw #0
    .var _fp = _fp + 2
    phw #_dst
    .var _fp = _fp + 2
    jsr _memset
    .var _fp = _fp - 6
    .loc "test_memops.c", 18
    phw #4
    .var _fp = _fp + 2
    phw #_src
    .var _fp = _fp + 2
    phw #_dst
    .var _fp = _fp + 2
    jsr _memcpy
    .var _fp = _fp - 6
    .loc "test_memops.c", 19
    lda #0
    sta $20
    sta $21
    lda #<_dst
    ldx #>_dst
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx $21
    clc
    adc __zp_scratch2
    sta $22
    txa
    adc __zp_scratch2+1
    sta $23
    ldy #0
    lda ($22),y
    ldx #0
    sta $20
    lda $20
    ldx #0
    cmp #66
    bne @if_then6
    bra @if_end8
@if_then6:
    lda #3
    ldx #0
    bra @__return
@if_end8:
    .loc "test_memops.c", 20
    lda #3
    ldx #0
    sta $20
    stx $21
    lda #<_dst
    ldx #>_dst
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx $21
    clc
    adc __zp_scratch2
    sta $22
    txa
    adc __zp_scratch2+1
    sta $23
    ldy #0
    lda ($22),y
    ldx #0
    sta $20
    lda $20
    ldx #0
    cmp #66
    bne @if_then9
    bra @if_end11
@if_then9:
    lda #4
    ldx #0
    bra @__return
@if_end11:
    .loc "test_memops.c", 23
    phw #4
    .var _fp = _fp + 2
    phw #_dst
    .var _fp = _fp + 2
    phw #_src
    .var _fp = _fp + 2
    jsr _memcmp
    .var _fp = _fp - 6
    sta $20
    stx $21
    lda $20
    ora $21
    bne @if_then12
    bra @if_end14
@if_then12:
    lda #5
    ldx #0
    bra @__return
@if_end14:
    .loc "test_memops.c", 26
    lda #65
    sta $20
    lda #2
    ldx #0
    sta $22
    stx $23
    lda #<_dst
    ldx #>_dst
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
    .loc "test_memops.c", 27
    phw #4
    .var _fp = _fp + 2
    phw #_dst
    .var _fp = _fp + 2
    phw #_src
    .var _fp = _fp + 2
    jsr _memcmp
    .var _fp = _fp - 6
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #0
    bcc @if_then15
    beq @if_then15
    bra @if_end17
@if_then15:
    lda #6
    ldx #0
    bra @__return
@if_end17:
    .loc "test_memops.c", 29
    lda #0
    ldx #0
@__return:
    .func_flags stack_call
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc


__zp_save_buf:

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

    .global _main

    .segment "code"

; function _main
; SAC inline storage: 6 bytes
    _main__local_0: .word 0
    _main__local_44: .word 0
    _main__local_56: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_string_literal.c", 4
    .local @_l_len = 2
    .local @_l_s = 0
    .local @_l_t = 4
; .debug_var: __main @_l_len offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_s offset=0 size=2 type=ptr scope=local
; .debug_var: __main @_l_t offset=4 size=2 type=ptr scope=local

@entry:
    .loc "test_string_literal.c", 5
    ldax #__str_0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_0
    stx _main__local_0+1
    .loc "test_string_literal.c", 8
    lda #0
    sta $20
    sta $21
    lda _main__local_0
    ldx _main__local_0+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda __zp_scratch2
    ldx __zp_scratch2+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $24
    lda #72
    sta $20
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    cmp.16 .AX, $24
    bne @if_then1
    bra @if_end3
@if_then1:
    lda #1
    ldx #0
    bra @__return
@if_end3:
    .loc "test_string_literal.c", 9
    lda #1
    ldx #0
    sta $20
    stx $21
    lda _main__local_0
    ldx _main__local_0+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda __zp_scratch2
    ldx __zp_scratch2+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $24
    lda #69
    sta $20
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    cmp.16 .AX, $24
    bne @if_then4
    bra @if_end6
@if_then4:
    lda #2
    ldx #0
    bra @__return
@if_end6:
    .loc "test_string_literal.c", 10
    lda #2
    ldx #0
    sta $20
    stx $21
    lda _main__local_0
    ldx _main__local_0+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda __zp_scratch2
    ldx __zp_scratch2+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $24
    lda #76
    sta $20
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    cmp.16 .AX, $24
    bne @if_then7
    bra @if_end9
@if_then7:
    lda #3
    ldx #0
    bra @__return
@if_end9:
    .loc "test_string_literal.c", 11
    lda #3
    ldx #0
    sta $20
    stx $21
    lda _main__local_0
    ldx _main__local_0+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda __zp_scratch2
    ldx __zp_scratch2+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $24
    lda #76
    sta $20
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    cmp.16 .AX, $24
    bne @if_then10
    bra @if_end12
@if_then10:
    lda #4
    ldx #0
    bra @__return
@if_end12:
    .loc "test_string_literal.c", 12
    lda #4
    ldx #0
    sta $20
    stx $21
    lda _main__local_0
    ldx _main__local_0+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda __zp_scratch2
    ldx __zp_scratch2+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $24
    lda #79
    sta $20
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    cmp.16 .AX, $24
    bne @if_then13
    bra @if_end15
@if_then13:
    lda #5
    ldx #0
    bra @__return
@if_end15:
    .loc "test_string_literal.c", 13
    lda #5
    ldx #0
    sta $20
    stx $21
    lda _main__local_0
    ldx _main__local_0+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda __zp_scratch2
    ldx __zp_scratch2+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $24
    lda #0
    sta $20
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    cmp.16 .AX, $24
    bne @if_then16
    bra @if_end18
@if_then16:
    lda #6
    ldx #0
    bra @__return
@if_end18:
    .loc "test_string_literal.c", 16
    lda #0
    sta _main__local_44
    sta _main__local_44+1
@while_cond19_ph:
    .loc "test_string_literal.c", 17
    lda #0
    sta $20
    lda $20
    ldx #0
    ldx #0
    sta $22
    stx $23
@while_cond19:
    lda _main__local_0
    ldx _main__local_0+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_44
    ldx _main__local_44+1
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda __zp_scratch2
    ldx __zp_scratch2+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $24
    lda #0
    sta $26
    lda $24
    ldx #0
    ldx #0
    sta $28
    stx $29
    lda $28
    ldx $29
    cmp.16 .AX, $22
    bne @while_body20
    bra @while_end21
@while_body20:
    .loc "test_string_literal.c", 18
    inc.16f __vr44
    bra @while_cond19
@while_end21:
    .loc "test_string_literal.c", 20
    lda _main__local_44
    ldx _main__local_44+1
    cmp.16 .AX, #5
    bne @if_then22
    bra @if_end24
@if_then22:
    lda #7
    ldx #0
    bra @__return
@if_end24:
    .loc "test_string_literal.c", 23
    ldax #__str_25
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_56
    stx _main__local_56+1
    .loc "test_string_literal.c", 24
    lda #0
    sta $20
    sta $21
    lda _main__local_56
    ldx _main__local_56+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda __zp_scratch2
    ldx __zp_scratch2+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $24
    lda #65
    sta $20
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    cmp.16 .AX, $24
    bne @if_then26
    bra @if_end28
@if_then26:
    lda #8
    ldx #0
    bra @__return
@if_end28:
    .loc "test_string_literal.c", 25
    lda #1
    ldx #0
    sta $20
    stx $21
    lda _main__local_56
    ldx _main__local_56+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda __zp_scratch2
    ldx __zp_scratch2+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $24
    lda #66
    sta $20
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    cmp.16 .AX, $24
    bne @if_then29
    bra @if_end31
@if_then29:
    lda #9
    ldx #0
    bra @__return
@if_end31:
    .loc "test_string_literal.c", 26
    lda #2
    ldx #0
    sta $20
    stx $21
    lda _main__local_56
    ldx _main__local_56+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda __zp_scratch2
    ldx __zp_scratch2+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $24
    lda #0
    sta $20
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    cmp.16 .AX, $24
    bne @if_then32
    bra @if_end34
@if_then32:
    lda #10
    ldx #0
    bra @__return
@if_end34:
    .loc "test_string_literal.c", 29
    lda #0
    sta $20
    sta $21
    lda _main__local_0
    ldx _main__local_0+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda __zp_scratch2
    ldx __zp_scratch2+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $24
    lda #72
    sta $20
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    cmp.16 .AX, $24
    bne @if_then35
    bra @if_end37
@if_then35:
    lda #11
    ldx #0
    bra @__return
@if_end37:
    .loc "test_string_literal.c", 31
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc


    .segment "data"
__str_0:
    .text "HELLO"
    .byte 0
__str_25:
    .text "AB"
    .byte 0

__zp_save_buf:

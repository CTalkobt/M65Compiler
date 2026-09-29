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
; SAC inline storage: 12 bytes
    _main__local_0: .word 0
    _main__local_1: .word 0
    _main__local_10: .word 0
    _main__local_68: .word 0
    _main__local_77: .word 0
    _main__local_83: .word 0
    _main__local_89: .word 0
    _main__local_125: .word 0
    _main__local_128: .word 0
    _main__local_135: .word 0
    _main__local_142: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_repeat.c", 3
    .local @_l_buf = 4
    .local @_l_i = 0
    .local @_l_p = 2
    .local @_l_x = 0
    .local @_l_y = 0
; .debug_var: __main @_l_buf offset=4 size=2 type=int8 scope=local
; .debug_var: __main @_l_i offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_p offset=2 size=2 type=ptr scope=local
; .debug_var: __main @_l_x offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_y offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_repeat.c", 8
    lda #0
    sta _main__local_1
    sta _main__local_1+1
@for_cond0_ph:
    leax.local 4
    sta $20
    stx $21
@for_cond0:
    lda _main__local_1
    ldx _main__local_1+1
    cmp.16 .AX, #8
    bcc @for_body1
    bra @for_end3
@for_body1:
    lda #0
    sta $26
    lda $26
    ldx #0
    pha
    lda _main__local_1
    ldx _main__local_1+1
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
@for_inc2:
    lda _main__local_1
    ldx _main__local_1+1
    sta $2A
    stx $2B
    lda $2A
    clc
    adc #1
    sta $2C
    lda $2B
    adc #0
    sta $2D
    lda $2C
    ldx $2D
    sta _main__local_1
    stx _main__local_1+1
    bra @for_cond0
@for_end3:
    .loc "test_repeat.c", 11
    leax.local 4
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_10
    stx _main__local_10+1
    .loc "test_repeat.c", 13
    lda #42
    sta $20
    lda _main__local_10
    ldx _main__local_10+1
    sta $22
    stx $23
    lda $20
    ldy #0
    sta ($22),y
    .loc "test_repeat.c", 14
    lda _main__local_10
    ldx _main__local_10+1
    sta $20
    stx $21
    lda $20
    ldx $21
    add.16 .AX, #1
    sta $22
    stx $23
    sta _main__local_10
    stx _main__local_10+1
    .loc "test_repeat.c", 13
    lda #42
    sta $20
    lda _main__local_10
    ldx _main__local_10+1
    sta $22
    stx $23
    lda $20
    ldy #0
    sta ($22),y
    .loc "test_repeat.c", 14
    lda _main__local_10
    ldx _main__local_10+1
    sta $20
    stx $21
    lda $20
    ldx $21
    add.16 .AX, #1
    sta $22
    stx $23
    sta _main__local_10
    stx _main__local_10+1
    .loc "test_repeat.c", 13
    lda #42
    sta $20
    lda _main__local_10
    ldx _main__local_10+1
    sta $22
    stx $23
    lda $20
    ldy #0
    sta ($22),y
    .loc "test_repeat.c", 14
    lda _main__local_10
    ldx _main__local_10+1
    sta $20
    stx $21
    lda $20
    ldx $21
    add.16 .AX, #1
    sta $22
    stx $23
    sta _main__local_10
    stx _main__local_10+1
    .loc "test_repeat.c", 13
    lda #42
    sta $20
    lda _main__local_10
    ldx _main__local_10+1
    sta $22
    stx $23
    lda $20
    ldy #0
    sta ($22),y
    .loc "test_repeat.c", 14
    lda _main__local_10
    ldx _main__local_10+1
    sta $20
    stx $21
    lda $20
    ldx $21
    add.16 .AX, #1
    sta $22
    stx $23
    sta _main__local_10
    stx _main__local_10+1
    .loc "test_repeat.c", 16
    leax.local 4
    sta $20
    stx $21
    lda #0
    sta $22
    sta $23
    lda $22
    ldx $23
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
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
    sta $26
    lda #42
    sta $20
    lda $26
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
    lda #1
    ldx #0
    bra @__return
@if_end6:
    .loc "test_repeat.c", 17
    leax.local 4
    sta $20
    stx $21
    lda #1
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
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
    sta $26
    lda #42
    sta $20
    lda $26
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
    lda #2
    ldx #0
    bra @__return
@if_end9:
    .loc "test_repeat.c", 18
    leax.local 4
    sta $20
    stx $21
    lda #2
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
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
    sta $26
    lda #42
    sta $20
    lda $26
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
    lda #3
    ldx #0
    bra @__return
@if_end12:
    .loc "test_repeat.c", 19
    leax.local 4
    sta $20
    stx $21
    lda #3
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
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
    sta $26
    lda #42
    sta $20
    lda $26
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
    lda #4
    ldx #0
    bra @__return
@if_end15:
    .loc "test_repeat.c", 20
    leax.local 4
    sta $20
    stx $21
    lda #4
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
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
    sta $26
    lda #0
    sta $20
    lda $26
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
    lda #5
    ldx #0
    bra @__return
@if_end18:
    .loc "test_repeat.c", 23
    lda #1
    sta _main__local_68
    stx _main__local_68+1
    lda #1
    sta _main__local_68
    stx _main__local_68+1
    .loc "test_repeat.c", 24
    leax.local 4
    sta $20
    stx $21
    lda #1
    sta $22
    lda _main__local_68
    ldx _main__local_68+1
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $24
    sec
    sbc $26
    sta $22
    lda $25
    sbc $26+1
    sta $23
    lda _main__local_68
    ldx _main__local_68+1
    pha
    lda $22
    ldx $23
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "test_repeat.c", 23
    lda #2
    sta _main__local_68
    stx _main__local_68+1
    lda #2
    sta _main__local_68
    stx _main__local_68+1
    .loc "test_repeat.c", 24
    leax.local 4
    sta $20
    stx $21
    lda #1
    sta $22
    lda $22
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $24
    sec
    sbc $26
    sta $22
    lda $25
    sbc $26+1
    sta $23
    lda _main__local_68
    ldx _main__local_68+1
    pha
    lda $22
    ldx $23
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "test_repeat.c", 23
    lda #3
    sta _main__local_68
    stx _main__local_68+1
    lda #3
    sta _main__local_68
    stx _main__local_68+1
    .loc "test_repeat.c", 24
    leax.local 4
    sta $20
    stx $21
    lda #1
    sta $22
    lda $22
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $24
    sec
    sbc $26
    sta $22
    lda $25
    sbc $26+1
    sta $23
    lda _main__local_68
    ldx _main__local_68+1
    pha
    lda $22
    ldx $23
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "test_repeat.c", 23
    lda #4
    sta _main__local_68
    stx _main__local_68+1
    lda #4
    sta _main__local_68
    stx _main__local_68+1
    .loc "test_repeat.c", 24
    leax.local 4
    sta $20
    stx $21
    lda #1
    sta $22
    lda $22
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $24
    sec
    sbc $26
    sta $22
    lda $25
    sbc $26+1
    sta $23
    lda _main__local_68
    ldx _main__local_68+1
    pha
    lda $22
    ldx $23
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "test_repeat.c", 26
    leax.local 4
    sta $20
    stx $21
    lda #0
    sta $22
    sta $23
    lda $22
    ldx $23
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
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
    sta $26
    lda #1
    sta $20
    lda $26
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
    bne @if_then19
    bra @if_end21
@if_then19:
    lda #6
    ldx #0
    bra @__return
@if_end21:
    .loc "test_repeat.c", 27
    leax.local 4
    sta $20
    stx $21
    lda #1
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
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
    sta $26
    lda #2
    sta $20
    lda $26
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
    bne @if_then22
    bra @if_end24
@if_then22:
    lda #7
    ldx #0
    bra @__return
@if_end24:
    .loc "test_repeat.c", 28
    leax.local 4
    sta $20
    stx $21
    lda #2
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
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
    sta $26
    lda #3
    sta $20
    lda $26
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
    bne @if_then25
    bra @if_end27
@if_then25:
    lda #8
    ldx #0
    bra @__return
@if_end27:
    .loc "test_repeat.c", 29
    leax.local 4
    sta $20
    stx $21
    lda #3
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
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
    sta $26
    lda #4
    sta $20
    lda $26
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
    bne @if_then28
    bra @if_end30
@if_then28:
    lda #9
    ldx #0
    bra @__return
@if_end30:
    .loc "test_repeat.c", 32
    lda #1
    ldx #0
    sta _main__local_125
    stx _main__local_125+1
    lda #1
    ldx #0
    sta _main__local_125
    stx _main__local_125+1
    .loc "test_repeat.c", 33
    lda #10
    ldx #0
    sta $20
    stx $21
    lda _main__local_125
    ldx _main__local_125+1
    mul.16 .AX, $20
    sta $22
    stx $23
    leax.local 4
    sta $20
    stx $21
    lda _main__local_125
    ldx _main__local_125+1
    sub.16 .AX, #1
    sta $26
    stx $27
    lda $22
    ldx $23
    pha
    lda $26
    ldx $27
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "test_repeat.c", 32
    lda #2
    ldx #0
    sta _main__local_125
    stx _main__local_125+1
    lda #2
    ldx #0
    sta _main__local_125
    stx _main__local_125+1
    .loc "test_repeat.c", 33
    lda #10
    ldx #0
    sta $20
    stx $21
    lda _main__local_125
    ldx _main__local_125+1
    mul.16 .AX, $20
    sta $22
    stx $23
    leax.local 4
    sta $20
    stx $21
    lda _main__local_125
    ldx _main__local_125+1
    sub.16 .AX, #1
    sta $26
    stx $27
    lda $22
    ldx $23
    pha
    lda $26
    ldx $27
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "test_repeat.c", 32
    lda #3
    ldx #0
    sta _main__local_125
    stx _main__local_125+1
    lda #3
    ldx #0
    sta _main__local_125
    stx _main__local_125+1
    .loc "test_repeat.c", 33
    lda #10
    ldx #0
    sta $20
    stx $21
    lda _main__local_125
    ldx _main__local_125+1
    mul.16 .AX, $20
    sta $22
    stx $23
    leax.local 4
    sta $20
    stx $21
    lda _main__local_125
    ldx _main__local_125+1
    sub.16 .AX, #1
    sta $26
    stx $27
    lda $22
    ldx $23
    pha
    lda $26
    ldx $27
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "test_repeat.c", 35
    leax.local 4
    sta $20
    stx $21
    lda #0
    sta $22
    sta $23
    lda $22
    ldx $23
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
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
    sta $26
    lda #10
    sta $20
    lda $26
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
    bne @if_then31
    bra @if_end33
@if_then31:
    lda #10
    ldx #0
    bra @__return
@if_end33:
    .loc "test_repeat.c", 36
    leax.local 4
    sta $20
    stx $21
    lda #1
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
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
    sta $26
    lda #20
    sta $20
    lda $26
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
    bne @if_then34
    bra @if_end36
@if_then34:
    lda #11
    ldx #0
    bra @__return
@if_end36:
    .loc "test_repeat.c", 37
    leax.local 4
    sta $20
    stx $21
    lda #2
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
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
    sta $26
    lda #30
    sta $20
    lda $26
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
    bne @if_then37
    bra @if_end39
@if_then37:
    lda #12
    ldx #0
    bra @__return
@if_end39:
    .loc "test_repeat.c", 45
    lda #0
    sta $20
    leax.local 4
    sta $22
    stx $23
    lda #7
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx #0
    pha
    lda $24
    ldx $25
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $22
    ldx $22+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "test_repeat.c", 47
    lda #77
    sta $20
    leax.local 4
    sta $22
    stx $23
    lda #7
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx #0
    pha
    lda $24
    ldx $25
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $22
    ldx $22+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "test_repeat.c", 49
    leax.local 4
    sta $20
    stx $21
    lda #7
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
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
    sta $26
    lda #77
    sta $20
    lda $26
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
    bne @if_then40
    bra @if_end42
@if_then40:
    lda #13
    ldx #0
    bra @__return
@if_end42:
    .loc "test_repeat.c", 51
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 12
    endproc


__zp_save_buf:

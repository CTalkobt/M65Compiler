    .o45
    .extern __sp_base
    __zp_scratch = $08
    __zp_scratch2 = $0A
    __zp_scratch3 = $0C
    __zp_scratch4 = $0E

    .extern _div
    .extern _ldiv

    .global _main

    .segment "code"

; function _main
    proc _main
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdlib.h", 3
; frame: 80 bytes (frame-allocated vRegs only)
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
    phw #0
    phw #0
    phw #0
    phw #0
    .local __vr0 = 0
    .local __vr131 = 4
    .local __vr134 = 12
    .local __vr135 = 16
    .local __vr149 = 20
    .local __vr150 = 24
    .local __vr164 = 28
    .local __vr165 = 32
    .local __vr179 = 36
    .local __vr180 = 40
    .local __vr194 = 44
    .local __vr195 = 48
    .local __vr209 = 52
    .local __vr210 = 56
    .local __vr224 = 60
    .local __vr225 = 64
    .local __vr260 = 68
    .local __vr261 = 70
    .local __vr275 = 72
    .local __vr276 = 76
    .local @_l_lresult = 4
    .local @_l_result = 0

@entry:
    .loc "src/test-resources/stdlib/test_division.c", 9
    phw #3
    .var _fp = _fp + 2
    phw #10
    .var _fp = _fp + 2
    jsr _div
    .var _fp = _fp - 4
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    staxyz.fp __vr0
    .loc "src/test-resources/stdlib/test_division.c", 10
    leax.fp __vr0
    sta $20
    stx $21
    lda $20
    clc
    adc #0
    sta $22
    lda $21
    adc #0
    sta $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #3
    bne @if_then0
@or_rhs3:
    leax.fp __vr0
    sta $20
    stx $21
    lda $20
    clc
    adc #2
    sta $22
    lda $21
    adc #0
    sta $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #1
    bne @if_then0
    bra @if_end2
@if_then0:
    lda #1
    ldx #0
    bra @__return
@if_end2:
    .loc "src/test-resources/stdlib/test_division.c", 13
    phw #3
    .var _fp = _fp + 2
    phw #12
    .var _fp = _fp + 2
    jsr _div
    .var _fp = _fp - 4
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    staxyz.fp __vr0
    .loc "src/test-resources/stdlib/test_division.c", 14
    leax.fp __vr0
    sta $20
    stx $21
    lda $20
    clc
    adc #0
    sta $22
    lda $21
    adc #0
    sta $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #4
    bne @if_then4
@or_rhs7:
    leax.fp __vr0
    sta $20
    stx $21
    lda $20
    clc
    adc #2
    sta $22
    lda $21
    adc #0
    sta $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
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
    .loc "src/test-resources/stdlib/test_division.c", 17
    phw #1
    .var _fp = _fp + 2
    phw #5
    .var _fp = _fp + 2
    jsr _div
    .var _fp = _fp - 4
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    staxyz.fp __vr0
    .loc "src/test-resources/stdlib/test_division.c", 18
    leax.fp __vr0
    sta $20
    stx $21
    lda $20
    clc
    adc #0
    sta $22
    lda $21
    adc #0
    sta $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #5
    bne @if_then8
@or_rhs11:
    leax.fp __vr0
    sta $20
    stx $21
    lda $20
    clc
    adc #2
    sta $22
    lda $21
    adc #0
    sta $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
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
    .loc "src/test-resources/stdlib/test_division.c", 21
    phw #3
    .var _fp = _fp + 2
    phw #65526
    .var _fp = _fp + 2
    jsr _div
    .var _fp = _fp - 4
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    staxyz.fp __vr0
    .loc "src/test-resources/stdlib/test_division.c", 22
    leax.fp __vr0
    sta $20
    stx $21
    lda $20
    clc
    adc #0
    sta $22
    lda $21
    adc #0
    sta $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #-3
    bne @if_then12
@or_rhs15:
    leax.fp __vr0
    sta $20
    stx $21
    lda $20
    clc
    adc #2
    sta $22
    lda $21
    adc #0
    sta $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #-1
    bne @if_then12
    bra @if_end14
@if_then12:
    lda #4
    ldx #0
    bra @__return
@if_end14:
    .loc "src/test-resources/stdlib/test_division.c", 25
    phw #65533
    .var _fp = _fp + 2
    phw #10
    .var _fp = _fp + 2
    jsr _div
    .var _fp = _fp - 4
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    staxyz.fp __vr0
    .loc "src/test-resources/stdlib/test_division.c", 26
    leax.fp __vr0
    sta $20
    stx $21
    lda $20
    clc
    adc #0
    sta $22
    lda $21
    adc #0
    sta $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #-3
    bne @if_then16
@or_rhs19:
    leax.fp __vr0
    sta $20
    stx $21
    lda $20
    clc
    adc #2
    sta $22
    lda $21
    adc #0
    sta $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #1
    bne @if_then16
    bra @if_end18
@if_then16:
    lda #5
    ldx #0
    bra @__return
@if_end18:
    .loc "src/test-resources/stdlib/test_division.c", 29
    phw #65533
    .var _fp = _fp + 2
    phw #65526
    .var _fp = _fp + 2
    jsr _div
    .var _fp = _fp - 4
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    staxyz.fp __vr0
    .loc "src/test-resources/stdlib/test_division.c", 30
    leax.fp __vr0
    sta $20
    stx $21
    lda $20
    clc
    adc #0
    sta $22
    lda $21
    adc #0
    sta $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #3
    bne @if_then20
@or_rhs23:
    leax.fp __vr0
    sta $20
    stx $21
    lda $20
    clc
    adc #2
    sta $22
    lda $21
    adc #0
    sta $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #-1
    bne @if_then20
    bra @if_end22
@if_then20:
    lda #6
    ldx #0
    bra @__return
@if_end22:
    .loc "src/test-resources/stdlib/test_division.c", 33
    phw #5
    .var _fp = _fp + 2
    phw #0
    .var _fp = _fp + 2
    jsr _div
    .var _fp = _fp - 4
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    staxyz.fp __vr0
    .loc "src/test-resources/stdlib/test_division.c", 34
    leax.fp __vr0
    sta $20
    stx $21
    lda $20
    clc
    adc #0
    sta $22
    lda $21
    adc #0
    sta $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ora $21
    bne @if_then24
@or_rhs27:
    leax.fp __vr0
    sta $20
    stx $21
    lda $20
    clc
    adc #2
    sta $22
    lda $21
    adc #0
    sta $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
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
    .loc "src/test-resources/stdlib/test_division.c", 37
    phw #7
    .var _fp = _fp + 2
    phw #1000
    .var _fp = _fp + 2
    jsr _div
    .var _fp = _fp - 4
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    staxyz.fp __vr0
    .loc "src/test-resources/stdlib/test_division.c", 38
    leax.fp __vr0
    sta $20
    stx $21
    lda $20
    clc
    adc #0
    sta $22
    lda $21
    adc #0
    sta $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #142
    bne @if_then28
@or_rhs31:
    leax.fp __vr0
    sta $20
    stx $21
    lda $20
    clc
    adc #2
    sta $22
    lda $21
    adc #0
    sta $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #6
    bne @if_then28
    bra @if_end30
@if_then28:
    lda #8
    ldx #0
    bra @__return
@if_end30:
    .loc "src/test-resources/stdlib/test_division.c", 41
    phw #10
    .var _fp = _fp + 2
    phw #3
    .var _fp = _fp + 2
    jsr _div
    .var _fp = _fp - 4
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    staxyz.fp __vr0
    .loc "src/test-resources/stdlib/test_division.c", 42
    leax.fp __vr0
    sta $20
    stx $21
    lda $20
    clc
    adc #0
    sta $22
    lda $21
    adc #0
    sta $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ora $21
    bne @if_then32
@or_rhs35:
    leax.fp __vr0
    sta $20
    stx $21
    lda $20
    clc
    adc #2
    sta $22
    lda $21
    adc #0
    sta $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #3
    bne @if_then32
    bra @if_end34
@if_then32:
    lda #9
    ldx #0
    bra @__return
@if_end34:
    .loc "src/test-resources/stdlib/test_division.c", 45
    phw #5
    .var _fp = _fp + 2
    phw #5
    .var _fp = _fp + 2
    jsr _div
    .var _fp = _fp - 4
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    staxyz.fp __vr0
    .loc "src/test-resources/stdlib/test_division.c", 46
    leax.fp __vr0
    sta $20
    stx $21
    lda $20
    clc
    adc #0
    sta $22
    lda $21
    adc #0
    sta $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #1
    bne @if_then36
@or_rhs39:
    leax.fp __vr0
    sta $20
    stx $21
    lda $20
    clc
    adc #2
    sta $22
    lda $21
    adc #0
    sta $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ora $21
    bne @if_then36
    bra @if_end38
@if_then36:
    lda #10
    ldx #0
    bra @__return
@if_end38:
    .loc "src/test-resources/stdlib/test_division.c", 50
    lda #10
    ldx #0
    sta $20
    stx $21
    lda #3
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    ldy #0
    ldz #0
    staxyz.fp __vr134
    lda $22
    ldx $23
    ldy #0
    ldz #0
    staxyz.fp __vr135
    ldaxyz.fp __vr135
    sta $28
    stx $29
    sty $2A
    stz $2B
    ldaxyz.fp __vr134
    sta $2C
    stx $2D
    sty $2E
    stz $2F
    lda $28
    ldx $29
    ldy $2A
    ldz $2B
    push .axyz
    .var _fp = _fp + 4
    lda $2C
    ldx $2D
    ldy $2E
    ldz $2F
    push .axyz
    .var _fp = _fp + 4
    jsr _ldiv
    .var _fp = _fp - 8
    sta $20
    stx $21
    lda $20
    ldx $21
    stax.fp __vr131
    .loc "src/test-resources/stdlib/test_division.c", 51
    leax.fp __vr131
    sta $20
    stx $21
    lda $20
    clc
    adc #0
    sta $22
    lda $21
    adc #0
    sta $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #3
    bne @if_then40
@or_rhs43:
    leax.fp __vr131
    sta $20
    stx $21
    lda $20
    clc
    adc #4
    sta $22
    lda $21
    adc #0
    sta $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #1
    bne @if_then40
    bra @if_end42
@if_then40:
    lda #11
    ldx #0
    bra @__return
@if_end42:
    .loc "src/test-resources/stdlib/test_division.c", 54
    lda #100
    ldx #0
    sta $20
    stx $21
    lda #10
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    ldy #0
    ldz #0
    staxyz.fp __vr149
    lda $22
    ldx $23
    ldy #0
    ldz #0
    staxyz.fp __vr150
    ldaxyz.fp __vr150
    sta $28
    stx $29
    sty $2A
    stz $2B
    ldaxyz.fp __vr149
    sta $2C
    stx $2D
    sty $2E
    stz $2F
    lda $28
    ldx $29
    ldy $2A
    ldz $2B
    push .axyz
    .var _fp = _fp + 4
    lda $2C
    ldx $2D
    ldy $2E
    ldz $2F
    push .axyz
    .var _fp = _fp + 4
    jsr _ldiv
    .var _fp = _fp - 8
    sta $20
    stx $21
    lda $20
    ldx $21
    stax.fp __vr131
    .loc "src/test-resources/stdlib/test_division.c", 55
    leax.fp __vr131
    sta $20
    stx $21
    lda $20
    clc
    adc #0
    sta $22
    lda $21
    adc #0
    sta $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #10
    bne @if_then44
@or_rhs47:
    leax.fp __vr131
    sta $20
    stx $21
    lda $20
    clc
    adc #4
    sta $22
    lda $21
    adc #0
    sta $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ora $21
    bne @if_then44
    bra @if_end46
@if_then44:
    lda #12
    ldx #0
    bra @__return
@if_end46:
    .loc "src/test-resources/stdlib/test_division.c", 58
    lda #156
    ldx #255
    sta $20
    stx $21
    lda #7
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    ldy #0
    ldz #0
    staxyz.fp __vr164
    lda $22
    ldx $23
    ldy #0
    ldz #0
    staxyz.fp __vr165
    ldaxyz.fp __vr165
    sta $28
    stx $29
    sty $2A
    stz $2B
    ldaxyz.fp __vr164
    sta $2C
    stx $2D
    sty $2E
    stz $2F
    lda $28
    ldx $29
    ldy $2A
    ldz $2B
    push .axyz
    .var _fp = _fp + 4
    lda $2C
    ldx $2D
    ldy $2E
    ldz $2F
    push .axyz
    .var _fp = _fp + 4
    jsr _ldiv
    .var _fp = _fp - 8
    sta $20
    stx $21
    lda $20
    ldx $21
    stax.fp __vr131
    .loc "src/test-resources/stdlib/test_division.c", 59
    leax.fp __vr131
    sta $20
    stx $21
    lda $20
    clc
    adc #0
    sta $22
    lda $21
    adc #0
    sta $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #-14
    bne @if_then48
@or_rhs51:
    leax.fp __vr131
    sta $20
    stx $21
    lda $20
    clc
    adc #4
    sta $22
    lda $21
    adc #0
    sta $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #-2
    bne @if_then48
    bra @if_end50
@if_then48:
    lda #13
    ldx #0
    bra @__return
@if_end50:
    .loc "src/test-resources/stdlib/test_division.c", 62
    lda #100
    ldx #0
    sta $20
    stx $21
    lda #249
    ldx #255
    sta $22
    stx $23
    lda $20
    ldx $21
    ldy #0
    ldz #0
    staxyz.fp __vr179
    lda $22
    ldx $23
    ldy #0
    ldz #0
    staxyz.fp __vr180
    ldaxyz.fp __vr180
    sta $28
    stx $29
    sty $2A
    stz $2B
    ldaxyz.fp __vr179
    sta $2C
    stx $2D
    sty $2E
    stz $2F
    lda $28
    ldx $29
    ldy $2A
    ldz $2B
    push .axyz
    .var _fp = _fp + 4
    lda $2C
    ldx $2D
    ldy $2E
    ldz $2F
    push .axyz
    .var _fp = _fp + 4
    jsr _ldiv
    .var _fp = _fp - 8
    sta $20
    stx $21
    lda $20
    ldx $21
    stax.fp __vr131
    .loc "src/test-resources/stdlib/test_division.c", 63
    leax.fp __vr131
    sta $20
    stx $21
    lda $20
    clc
    adc #0
    sta $22
    lda $21
    adc #0
    sta $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #-14
    bne @if_then52
@or_rhs55:
    leax.fp __vr131
    sta $20
    stx $21
    lda $20
    clc
    adc #4
    sta $22
    lda $21
    adc #0
    sta $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #2
    bne @if_then52
    bra @if_end54
@if_then52:
    lda #14
    ldx #0
    bra @__return
@if_end54:
    .loc "src/test-resources/stdlib/test_division.c", 66
    lda #156
    ldx #255
    sta $20
    stx $21
    lda #249
    ldx #255
    sta $22
    stx $23
    lda $20
    ldx $21
    ldy #0
    ldz #0
    staxyz.fp __vr194
    lda $22
    ldx $23
    ldy #0
    ldz #0
    staxyz.fp __vr195
    ldaxyz.fp __vr195
    sta $28
    stx $29
    sty $2A
    stz $2B
    ldaxyz.fp __vr194
    sta $2C
    stx $2D
    sty $2E
    stz $2F
    lda $28
    ldx $29
    ldy $2A
    ldz $2B
    push .axyz
    .var _fp = _fp + 4
    lda $2C
    ldx $2D
    ldy $2E
    ldz $2F
    push .axyz
    .var _fp = _fp + 4
    jsr _ldiv
    .var _fp = _fp - 8
    sta $20
    stx $21
    lda $20
    ldx $21
    stax.fp __vr131
    .loc "src/test-resources/stdlib/test_division.c", 67
    leax.fp __vr131
    sta $20
    stx $21
    lda $20
    clc
    adc #0
    sta $22
    lda $21
    adc #0
    sta $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #14
    bne @if_then56
@or_rhs59:
    leax.fp __vr131
    sta $20
    stx $21
    lda $20
    clc
    adc #4
    sta $22
    lda $21
    adc #0
    sta $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #-2
    bne @if_then56
    bra @if_end58
@if_then56:
    lda #15
    ldx #0
    bra @__return
@if_end58:
    .loc "src/test-resources/stdlib/test_division.c", 70
    lda #0
    sta $20
    sta $21
    lda #50
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    ldy #0
    ldz #0
    staxyz.fp __vr209
    lda $22
    ldx $23
    ldy #0
    ldz #0
    staxyz.fp __vr210
    ldaxyz.fp __vr210
    sta $28
    stx $29
    sty $2A
    stz $2B
    ldaxyz.fp __vr209
    sta $2C
    stx $2D
    sty $2E
    stz $2F
    lda $28
    ldx $29
    ldy $2A
    ldz $2B
    push .axyz
    .var _fp = _fp + 4
    lda $2C
    ldx $2D
    ldy $2E
    ldz $2F
    push .axyz
    .var _fp = _fp + 4
    jsr _ldiv
    .var _fp = _fp - 8
    sta $20
    stx $21
    lda $20
    ldx $21
    stax.fp __vr131
    .loc "src/test-resources/stdlib/test_division.c", 71
    leax.fp __vr131
    sta $20
    stx $21
    lda $20
    clc
    adc #0
    sta $22
    lda $21
    adc #0
    sta $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ora $21
    bne @if_then60
@or_rhs63:
    leax.fp __vr131
    sta $20
    stx $21
    lda $20
    clc
    adc #4
    sta $22
    lda $21
    adc #0
    sta $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ora $21
    bne @if_then60
    bra @if_end62
@if_then60:
    lda #16
    ldx #0
    bra @__return
@if_end62:
    .loc "src/test-resources/stdlib/test_division.c", 74
    lda #15
    ldx #39
    sta $20
    stx $21
    lda #13
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    ldy #0
    ldz #0
    staxyz.fp __vr224
    lda $22
    ldx $23
    ldy #0
    ldz #0
    staxyz.fp __vr225
    ldaxyz.fp __vr225
    sta $28
    stx $29
    sty $2A
    stz $2B
    ldaxyz.fp __vr224
    sta $2C
    stx $2D
    sty $2E
    stz $2F
    lda $28
    ldx $29
    ldy $2A
    ldz $2B
    push .axyz
    .var _fp = _fp + 4
    lda $2C
    ldx $2D
    ldy $2E
    ldz $2F
    push .axyz
    .var _fp = _fp + 4
    jsr _ldiv
    .var _fp = _fp - 8
    sta $20
    stx $21
    lda $20
    ldx $21
    stax.fp __vr131
    .loc "src/test-resources/stdlib/test_division.c", 75
    leax.fp __vr131
    sta $20
    stx $21
    lda $20
    clc
    adc #0
    sta $22
    lda $21
    adc #0
    sta $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #769
    bne @if_then64
@or_rhs67:
    leax.fp __vr131
    sta $20
    stx $21
    lda $20
    clc
    adc #4
    sta $22
    lda $21
    adc #0
    sta $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #2
    bne @if_then64
    bra @if_end66
@if_then64:
    lda #17
    ldx #0
    bra @__return
@if_end66:
    .loc "src/test-resources/stdlib/test_division.c", 78
    phw #65534
    .var _fp = _fp + 2
    phw #7
    .var _fp = _fp + 2
    jsr _div
    .var _fp = _fp - 4
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    staxyz.fp __vr0
    .loc "src/test-resources/stdlib/test_division.c", 79
    leax.fp __vr0
    sta $20
    stx $21
    lda $20
    clc
    adc #2
    sta $22
    lda $21
    adc #0
    sta $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #1
    bne @if_then68
    bra @if_end70
@if_then68:
    lda #18
    ldx #0
    bra @__return
@if_end70:
    .loc "src/test-resources/stdlib/test_division.c", 82
    phw #3
    .var _fp = _fp + 2
    phw #20
    .var _fp = _fp + 2
    jsr _div
    .var _fp = _fp - 4
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    staxyz.fp __vr0
    .loc "src/test-resources/stdlib/test_division.c", 83
    leax.fp __vr0
    sta $20
    stx $21
    lda $20
    clc
    adc #0
    sta $22
    lda $21
    adc #0
    sta $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #6
    bne @if_then71
@or_rhs74:
    leax.fp __vr0
    sta $20
    stx $21
    lda $20
    clc
    adc #2
    sta $22
    lda $21
    adc #0
    sta $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #2
    bne @if_then71
    bra @if_end73
@if_then71:
    lda #19
    ldx #0
    bra @__return
@if_end73:
    .loc "src/test-resources/stdlib/test_division.c", 84
    leax.fp __vr0
    sta $20
    stx $21
    lda $20
    clc
    adc #0
    sta $22
    lda $21
    adc #0
    sta $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    stax.fp __vr260
    lda #2
    ldx #0
    stax.fp __vr261
    ldax.fp __vr261
    sta $28
    stx $29
    ldax.fp __vr260
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
    jsr _div
    .var _fp = _fp - 4
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    staxyz.fp __vr0
    .loc "src/test-resources/stdlib/test_division.c", 85
    leax.fp __vr0
    sta $20
    stx $21
    lda $20
    clc
    adc #0
    sta $22
    lda $21
    adc #0
    sta $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #3
    bne @if_then75
@or_rhs78:
    leax.fp __vr0
    sta $20
    stx $21
    lda $20
    clc
    adc #2
    sta $22
    lda $21
    adc #0
    sta $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ora $21
    bne @if_then75
    bra @if_end77
@if_then75:
    lda #20
    ldx #0
    bra @__return
@if_end77:
    .loc "src/test-resources/stdlib/test_division.c", 88
    lda #241
    ldx #255
    sta $20
    stx $21
    lda #4
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    ldy #0
    ldz #0
    staxyz.fp __vr275
    lda $22
    ldx $23
    ldy #0
    ldz #0
    staxyz.fp __vr276
    ldaxyz.fp __vr276
    sta $28
    stx $29
    sty $2A
    stz $2B
    ldaxyz.fp __vr275
    sta $2C
    stx $2D
    sty $2E
    stz $2F
    lda $28
    ldx $29
    ldy $2A
    ldz $2B
    push .axyz
    .var _fp = _fp + 4
    lda $2C
    ldx $2D
    ldy $2E
    ldz $2F
    push .axyz
    .var _fp = _fp + 4
    jsr _ldiv
    .var _fp = _fp - 8
    sta $20
    stx $21
    lda $20
    ldx $21
    stax.fp __vr131
    .loc "src/test-resources/stdlib/test_division.c", 89
    leax.fp __vr131
    sta $20
    stx $21
    lda $20
    clc
    adc #4
    sta $22
    lda $21
    adc #0
    sta $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #-3
    bne @if_then79
    bra @if_end81
@if_then79:
    lda #21
    ldx #0
    bra @__return
@if_end81:
    .loc "src/test-resources/stdlib/test_division.c", 91
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


__zp_save_buf:

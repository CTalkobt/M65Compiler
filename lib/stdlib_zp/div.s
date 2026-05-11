; div.s — Integer division with quotient and remainder (ZP calling convention)
;
; div_t div(int numer, int denom);
;   Returns div_t { int quot; int rem; } via hidden return pointer in $03/$04.
;   numer in $05/$06, denom in $07/$08.

.global _div

.segment "code"

proc _div
    .zp_uses $03, $04, $05, $06, $07, $08
    .zp_clobbers $03, $04, $05, $06, $07, $08
    .zp_release $03, $04, $05, $06, $07, $08
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z

    ; Save ZP temps
    lda $02
    pha
    lda $09
    pha

    stz $02                 ; numer_neg
    stz $09                 ; quot sign XOR

    ; Absolute value of numerator
    lda $06                 ; high byte
    bpl @numer_pos
    inc $02
    inc $09
    ; Negate
    lda $05
    eor #$FF
    clc
    adc #1
    sta $05
    lda $06
    eor #$FF
    adc #0
    sta $06
@numer_pos:
    lda $05
    sta $D760
    lda $06
    sta $D761
    stz $D762
    stz $D763

    ; Absolute value of denominator
    lda $08                 ; high byte
    bpl @denom_pos
    lda $09
    eor #1
    sta $09
    lda $07
    eor #$FF
    clc
    adc #1
    sta $07
    lda $08
    eor #$FF
    adc #0
    sta $08
@denom_pos:
    lda $07
    sta $D764
    lda $08
    sta $D765
    stz $D766
    stz $D767

@wait:
    lda $D7FE
    bmi @wait

    ; Quotient
    lda $D768
    ldx $D769
    ldy $09
    beq @quot_pos
    eor #$FF
    clc
    adc #1
    pha
    txa
    eor #$FF
    adc #0
    tax
    pla
@quot_pos:
    ldy #0
    sta ($03), y
    iny
    txa
    sta ($03), y

    ; Remainder
    lda $D770
    ldx $D771
    ldy $02
    beq @rem_pos
    eor #$FF
    clc
    adc #1
    pha
    txa
    eor #$FF
    adc #0
    tax
    pla
@rem_pos:
    ldy #2
    sta ($03), y
    iny
    txa
    sta ($03), y

    ; Return pointer in AX
    lda $03
    ldx $04

    ; Restore temps
    pla
    sta $09
    pla
    sta $02
    rts
    endproc

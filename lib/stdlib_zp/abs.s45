; abs.s — Integer absolute value (ZP calling convention)
;
; int abs(int value);
;   Returns the absolute value of a 16-bit signed integer.

.global _abs

.segment "code"

proc _abs
    .zp_uses $03, $04
    .zp_clobbers $03, $04
    .zp_release $03, $04
    .reg_clobbers A, X
    .flag_clobbers C, N, Z

    lda $04             ; check sign (high byte)
    bpl @positive
    ; Negate: complement and add 1
    lda $03
    eor #$FF
    clc
    adc #1
    pha
    lda $04
    eor #$FF
    adc #0
    tax
    pla
    rts
@positive:
    lda $03
    ldx $04
    rts
    endproc

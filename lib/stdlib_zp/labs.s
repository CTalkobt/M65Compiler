; labs.s — Long absolute value (ZP calling convention)
;
; long labs(long value);
;   Returns the absolute value of a 32-bit signed integer.

.global _labs

.segment "code"

proc _labs
    .zp_uses $03, $04, $05, $06
    .zp_clobbers $03, $04, $05, $06
    .zp_release $03, $04, $05, $06
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z

    lda $06                 ; byte3 (sign)
    bpl @positive
    ; Negate 32-bit
    sec
    lda #0
    sbc $03
    pha                     ; save byte0
    lda #0
    sbc $04
    tax                     ; byte1
    lda #0
    sbc $05
    tay                     ; byte2
    lda #0
    sbc $06
    taz                     ; byte3
    pla                     ; byte0 in A
    rts
@positive:
    lda $06
    taz                     ; byte3
    lda $03
    ldx $04
    ldy $05
    rts
    endproc

; labs.s — Long absolute value
;
; long labs(long value);
;   Returns the absolute value of a 32-bit signed integer.
;   Result in .AXYZ (A=byte0, X=byte1, Y=byte2, Z=byte3).

.global _labs
.extern __sp_base

.segment "code"

proc _labs, D#_p_value
    .var _fp = 0
    ; Check sign (byte3)
    lda _p_value+3, s
    bpl @positive
    ; Negate: complement and add 1
    lda _p_value, s
    eor #$FF
    clc
    adc #1
    pha                     ; save byte0
    lda _p_value+1, s
    eor #$FF
    adc #0
    tax                     ; byte1
    lda _p_value+2, s
    eor #$FF
    adc #0
    tay                     ; byte2
    lda _p_value+3, s
    eor #$FF
    adc #0
    taz                     ; byte3
    pla                     ; byte0 in A
    rtn #0
@positive:
    lda _p_value, s
    ldx _p_value+1, s
    ldy _p_value+2, s
    ldz _p_value+3, s
    rtn #0
    endproc

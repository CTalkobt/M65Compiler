; abs.s — Integer absolute value
;
; int abs(int value);
;   Returns the absolute value of a 16-bit signed integer.
;   Result in .AX (A=low, X=high).

.global _abs
.extern __sp_base

.segment "code"

proc _abs, W#_p_value
    .var _fp = 0
    ldax _p_value, sp
    ; Check sign bit (high byte in X)
    txa
    bpl @done           ; already positive, A/X already set
    ; Negate .AX: complement and add 1
    ldax _p_value, sp    ; reload
    eor #$FF            ; complement low byte
    clc
    adc #1
    pha
    txa
    eor #$FF
    adc #0
    tax
    pla
@done:
    rtn #0
    endproc

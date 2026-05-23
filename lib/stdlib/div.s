; div.s — Integer division with quotient and remainder
;
; div_t div(int numer, int denom);
;   Returns div_t { int quot; int rem; } via hidden return pointer.
;   Uses signed 16-bit division.
;
;   Hidden pointer to result struct is the last parameter pushed
;   (highest stack offset).

.global _div
.extern __sp_base
.extern __zp_scratch

.segment "code"

proc _div, W#_p___ret_ptr, W#_p_numer, W#_p_denom
    .var _fp = 0

    ; Track signs for result fixup
    ; $02 = numer_neg, $03 = result sign XOR
    lda $02
    pha
    lda $03
    pha

    stz $02                 ; numer_neg = 0
    stz $03                 ; sign = 0

    ; Load numerator
    ldax _p_numer+2, s
    ; Check if negative
    cpx #$80
    bcc @numer_pos
    ; Negate numerator
    eor #$FF
    clc
    adc #1
    pha
    txa
    eor #$FF
    adc #0
    tax
    pla
    inc $02                 ; numer was negative
    inc $03                 ; flip result sign
@numer_pos:
    ; Store abs(numer) in dividend
    sta $D760
    stx $D761
    stz $D762
    stz $D763

    ; Load denominator
    ldax _p_denom+2, s
    ; Check if negative
    cpx #$80
    bcc @denom_pos
    ; Negate denominator
    eor #$FF
    clc
    adc #1
    pha
    txa
    eor #$FF
    adc #0
    tax
    pla
    lda $03
    eor #1                  ; flip result sign
    sta $03
    ; Reload abs(denom) into AX
    ldax _p_denom+2, s
    eor #$FF
    clc
    adc #1
    pha
    txa
    eor #$FF
    adc #0
    tax
    pla
@denom_pos:
    sta $D764
    stx $D765
    stz $D766
    stz $D767

    ; Wait for hardware divider
@wait:
    bit $D70F
    bne @wait

    ; Quotient at $D768-$D769
    lda $D768
    ldx $D769

    ; Apply sign to quotient
    ldy $03
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
    ; Store quot via hidden return pointer
    ldy #0
    sta (_p___ret_ptr+2, sp), y
    iny
    txa
    sta (_p___ret_ptr+2, sp), y

    ; Remainder at $D76C-$D76D
    lda $D76C
    ldx $D76D

    ; Remainder sign follows numerator sign
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
    ; Store rem via hidden return pointer
    ldy #2
    sta (_p___ret_ptr+2, sp), y
    iny
    txa
    sta (_p___ret_ptr+2, sp), y

    ; Return pointer to struct in AX
    ldax _p___ret_ptr+2, s

    ; Restore ZP
    pla
    sta $03
    pla
    sta $02

    rtn #0
    endproc

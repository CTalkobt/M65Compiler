; ldiv.s — Long division with quotient and remainder
;
; ldiv_t ldiv(long numer, long denom);
;   Returns ldiv_t { long quot; long rem; } via hidden return pointer.
;   Uses MEGA65 hardware divider for unsigned 32-bit division,
;   with sign fixup for signed operands.
;   ZP usage: $02-$08 (saved/restored)

.global _ldiv
.extern __sp_base

.segment "code"

proc _ldiv, W#_p___ret_ptr, D#_p_numer, D#_p_denom
    .var _fp = 0

    ; Save ZP temps $02-$08
    lda $02
    pha
    lda $03
    pha
    lda $04
    pha
    lda $05
    pha
    lda $06
    pha
    lda $07
    pha
    lda $08
    pha

    stz $02                 ; numer_neg
    stz $03                 ; quot sign XOR

    ; Load numer into $04-$07
    lda _p_numer+7, s
    sta $04
    lda _p_numer+8, s
    sta $05
    lda _p_numer+9, s
    sta $06
    lda _p_numer+10, s
    sta $07

    ; --- abs(numer) ---
    lda $07                 ; byte3 (sign)
    bpl @numer_pos
    inc $02
    inc $03
    ; Negate
    lda $04
    eor #$FF
    clc
    adc #1
    sta $04
    lda $05
    eor #$FF
    adc #0
    sta $05
    lda $06
    eor #$FF
    adc #0
    sta $06
    lda $07
    eor #$FF
    adc #0
    sta $07
@numer_pos:
    lda $04
    sta $D760
    lda $05
    sta $D761
    lda $06
    sta $D762
    lda $07
    sta $D763

    ; Load denom into $04-$07
    lda _p_denom+7, s
    sta $04
    lda _p_denom+8, s
    sta $05
    lda _p_denom+9, s
    sta $06
    lda _p_denom+10, s
    sta $07

    ; --- abs(denom) ---
    lda $07
    bpl @denom_pos
    lda $03
    eor #1
    sta $03
    lda $04
    eor #$FF
    clc
    adc #1
    sta $04
    lda $05
    eor #$FF
    adc #0
    sta $05
    lda $06
    eor #$FF
    adc #0
    sta $06
    lda $07
    eor #$FF
    adc #0
    sta $07
@denom_pos:
    lda $04
    sta $D764
    lda $05
    sta $D765
    lda $06
    sta $D766
    lda $07
    sta $D767

    ; Wait for hardware divider
@wait:
    bit $D70F
    bne @wait

    ; Load ret_ptr into $04/$05 (adjusted for 7 pushed ZP bytes)
    ldax _p___ret_ptr+7, s
    sta $04
    stx $05

    ; --- Store quotient with sign fixup ---
    lda $03
    beq @quot_pos
    ; Negate quotient
    lda $D768
    eor #$FF
    clc
    adc #1
    ldy #0
    sta ($04), y
    lda $D769
    eor #$FF
    adc #0
    iny
    sta ($04), y
    lda $D76A
    eor #$FF
    adc #0
    iny
    sta ($04), y
    lda $D76B
    eor #$FF
    adc #0
    iny
    sta ($04), y
    bra @do_rem

@quot_pos:
    ldy #0
    lda $D768
    sta ($04), y
    iny
    lda $D769
    sta ($04), y
    iny
    lda $D76A
    sta ($04), y
    iny
    lda $D76B
    sta ($04), y

@do_rem:
    ; --- Store remainder with numerator sign ---
    lda $02
    beq @rem_pos
    ; Negate remainder
    lda $D770
    eor #$FF
    clc
    adc #1
    ldy #4
    sta ($04), y
    lda $D771
    eor #$FF
    adc #0
    iny
    sta ($04), y
    lda $D772
    eor #$FF
    adc #0
    iny
    sta ($04), y
    lda $D773
    eor #$FF
    adc #0
    iny
    sta ($04), y
    bra @done

@rem_pos:
    ldy #4
    lda $D770
    sta ($04), y
    iny
    lda $D771
    sta ($04), y
    iny
    lda $D772
    sta ($04), y
    iny
    lda $D773
    sta ($04), y

@done:
    ; Return pointer in AX
    lda $04
    ldx $05

    ; Restore ZP $02-$08
    ; (Note: PLZ does not touch A/X)
    plz
    stz $08
    plz
    stz $07
    plz
    stz $06
    plz
    stz $05
    plz
    stz $04
    plz
    stz $03
    plz
    stz $02

    rtn #0
    endproc

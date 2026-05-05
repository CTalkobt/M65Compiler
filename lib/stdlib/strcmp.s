; strcmp.s — Compare two strings
;
; int strcmp(char *s1, char *s2);
;   Returns 0 if equal, <0 if s1 < s2, >0 if s1 > s2.
;   Unsigned byte comparison.

.global _strcmp
.extern __sp_base

.segment "code"

proc _strcmp, W#_p_s1, W#_p_s2
    .var _fp = 0
    ; Save ZP $02-$04 to stack
    lda $02: pha
    lda $03: pha
    lda $04: pha

    ldax _p_s1+3, s
    stax $02
    ldy #0
@loop:
    lda ($02),y         ; load from s1
    sta $04             ; temporary scratch
    lda (_p_s2+3, sp), y   ; load from s2
    cmp $04
    bne @diff
    ; both equal — check if NUL
    cmp #0
    beq @equal
    iny
    bne @loop
    inc $03
    ; increment s2 pointer on stack for wrap
    clc
    lda _p_s2+3, s
    adc #0 ; y already 0 here
    sta _p_s2+3, s
    lda _p_s2+4, s
    adc #1
    sta _p_s2+4, s
    bra @loop
@equal:
    ldax #$0000
    bra @restore
@diff:
    ; A = s2 byte, $04 = s1 byte. We want s1 - s2.
    ; CMP calculates Accumulator - operand.
    ; If we do LDA s1_byte; CMP s2_byte, we get s1 - s2.
    sta $03 ; save s2 byte
    lda $04 ; load s1 byte
    cmp $03 ; s1 - s2
    bcs @greater
    ; s1 < s2
    ldax #$FFFF
    bra @restore
@greater:
    ldax #$0001
@restore:
    ; Return AX is preserved (PLZ doesn't touch A/X).
    plz
    stz $04
    plz
    stz $03
    plz
    stz $02
    rts
    endproc

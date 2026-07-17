; atoi.s — Convert string to integer
;
; int atoi(char *s);
;   Parses a decimal integer from a NUL-terminated string.
;   Skips leading whitespace ($20, $09-$0D).
;   Recognizes optional leading '-' for negative values.
;   Stops at the first non-digit character.
;   Returns the 16-bit integer value in .AX.

.global _atoi
.extern __sp_base

.segment "code"

proc _atoi, W#_p_s
    .var _fp = 0
    ; Save ZP $02-$05 to stack (compiler pool)
    lda $02
    pha
    lda $03
    pha
    lda $04
    pha
    lda $05
    pha

    ldy #0

    ; --- Skip leading whitespace ---
@skipws:
    lda (_p_s+4, sp), y    ; load from string via stack-relative indirect
    cmp #$20            ; space
    beq @skipnext
    cmp #$09
    bcc @checksign      ; < $09 = not whitespace
    cmp #$0e
    bcc @skipnext       ; $09-$0D = whitespace
    bra @checksign
@skipnext:
    iny
    bne @skipws
    bra @retzero        ; ran off — return 0

    ; --- Check for sign ---
@checksign:
    stz $04             ; $04 = sign flag (0=positive)
    cmp #$2d            ; '-'
    bne @checkplus
    inc $04             ; mark negative
    iny
    bra @startnum
@checkplus:
    cmp #$2b            ; '+'
    bne @startnum
    iny
@startnum:
    ; --- Initialize result = 0 ---
    lda #0
    sta $02             ; $02/$03 = result (lo/hi)
    sta $03

    ; --- Parse digits ---
@digit:
    lda (_p_s+4, sp), y
    sec
    sbc #$30            ; subtract '0'
    bcc @done           ; < '0'
    cmp #10
    bcs @done           ; > '9'
    sta $05             ; $05 = current digit

    ; result = result * 10: use hardware multiplier
    ; $D770 = multiplicand, $D774 = multiplier, product at $D778
    lda $02
    sta $d770
    lda $03
    sta $d771
    lda #0
    sta $d772
    sta $d773
    lda #10
    sta $d774
    lda #0
    sta $d775
    sta $d776
    sta $d777
    ; Read 16-bit product
    lda $d778
    sta $02
    lda $d779
    sta $03

    ; result += digit
    lda $02
    clc
    adc $05
    sta $02
    lda $03
    adc #0
    sta $03

    iny
    bne @digit

@done:
    ; Check sign flag
    lda $04
    beq @positive
    ; Negate: result = 0 - result
    sec
    lda #0
    sbc $02
    sta $02
    lda #0
    sbc $03
    sta $03
@positive:
    lda $02
    ldx $03
    bra @restore

@retzero:
    lda #0
    ldx #0
@restore:
    ; Restore ZP $02-$05 from stack using Z as scratch.
    ; A and X (return value) are preserved as PLZ only affects Z and flags.
    plz
    stz $05
    plz
    stz $04
    plz
    stz $03
    plz
    stz $02
    rts
    endproc

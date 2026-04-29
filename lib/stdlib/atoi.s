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
    ldax _p_s, s
    stax $02            ; $02/$03 = string pointer
    ldy #0

    ; --- Skip leading whitespace ---
@skipws:
    lda ($02),y
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
    stz $06             ; $06 = sign flag (0=positive)
    cmp #$2d            ; '-'
    bne @checkplus
    inc $06             ; mark negative
    iny
    bra @startnum
@checkplus:
    cmp #$2b            ; '+'
    bne @startnum
    iny
@startnum:
    ; --- Initialize result = 0 ---
    stz $04             ; $04/$05 = result (lo/hi)
    stz $05

    ; --- Parse digits ---
@digit:
    lda ($02),y
    sec
    sbc #$30            ; subtract '0'
    bcc @done           ; < '0'
    cmp #10
    bcs @done           ; > '9'
    sta $07             ; $07 = current digit

    ; result = result * 10: use hardware multiplier
    ; $D770 = multiplicand, $D774 = multiplier, product at $D778
    lda $04
    sta $d770
    lda $05
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
    sta $04
    lda $d779
    sta $05

    ; result += digit
    lda $04
    clc
    adc $07
    sta $04
    lda $05
    adc #0
    sta $05

    iny
    bne @digit
    ; Y overflow — unlikely for valid input but safe to stop

@done:
    ; Check sign flag
    lda $06
    beq @positive
    ; Negate: result = 0 - result
    sec
    lda #0
    sbc $04
    sta $04
    lda #0
    sbc $05
    sta $05
@positive:
    lda $04
    ldx $05
    rtn #0

@retzero:
    lda #0
    ldx #0
    rtn #0
    endproc

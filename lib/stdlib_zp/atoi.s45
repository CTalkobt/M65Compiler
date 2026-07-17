; atoi.s — Convert string to integer (ZP calling convention)
;
; int atoi(char *s);
;   Parses a decimal integer from a NUL-terminated string.
;   Returns the 16-bit integer value in .AX.

.global _atoi

.segment "code"

proc _atoi
    .zp_uses $03, $04
    .zp_clobbers $03, $04, $05, $06, $07, $08, $09
    .zp_release $03, $04
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z

    ; s=$03/$04 — use as indirect pointer
    ldy #0

    ; --- Skip leading whitespace ---
@skipws:
    lda ($03),y
    cmp #$20
    beq @skipnext
    cmp #$09
    bcc @checksign
    cmp #$0e
    bcc @skipnext
    bra @checksign
@skipnext:
    iny
    bne @skipws
    bra @retzero

    ; --- Check for sign ---
@checksign:
    stz $09             ; $09 = sign flag (0=positive)
    cmp #$2d            ; '-'
    bne @checkplus
    inc $09
    iny
    bra @startnum
@checkplus:
    cmp #$2b            ; '+'
    bne @startnum
    iny
@startnum:
    ; --- Initialize result = 0 ---
    stz $05             ; $05/$06 = result (lo/hi)
    stz $06

    ; --- Parse digits ---
@digit:
    lda ($03),y
    sec
    sbc #$30
    bcc @done
    cmp #10
    bcs @done
    sta $07             ; $07 = current digit

    ; result = result * 10 using hardware multiplier
    lda $05
    sta $d770
    lda $06
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
    lda $d778
    sta $05
    lda $d779
    sta $06

    ; result += digit
    lda $05
    clc
    adc $07
    sta $05
    lda $06
    adc #0
    sta $06

    iny
    bne @digit

@done:
    lda $09
    beq @positive
    ; Negate
    sec
    lda #0
    sbc $05
    sta $05
    lda #0
    sbc $06
    sta $06
@positive:
    lda $05
    ldx $06
    rts

@retzero:
    lda #0
    ldx #0
    rts
    endproc

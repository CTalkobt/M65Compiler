; strrchr.s — Locate last occurrence of character in string
;
; char *strrchr(char *s, int c);
;   Returns pointer to last occurrence of (char)c in s,
;   or NULL (0) if not found.

.global _strrchr

.segment "code"

proc _strrchr, W#_p_s, W#_p_c
    .var _fp = 0
    ldax _p_s, s
    stax $02
    lda.sp _p_c
    sta $04
    ; result = NULL
    stz $05
    stz $06
    ldy #0
@loop:
    lda ($02),y
    cmp $04
    bne @skip
    ; save current position as result
    tya
    clc
    adc $02
    sta $05
    lda #0
    adc $03
    sta $06
@skip:
    lda ($02),y
    beq @done
    iny
    bne @loop
    inc $03
    bra @loop
@done:
    lda $05
    ldx $06
    rtn #0
    endproc

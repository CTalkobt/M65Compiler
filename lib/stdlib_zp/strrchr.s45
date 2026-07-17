; strrchr.s — Locate last occurrence of character in string (ZP calling convention)
;
; char *strrchr(char *s, int c);
;   Returns pointer to last occurrence of (char)c in s, or NULL.

.global _strrchr

.segment "code"

proc _strrchr
    .zp_uses $03, $04, $05, $06
    .zp_clobbers $03, $04, $05, $06, $07, $08
    .zp_release $03, $04, $05, $06
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z

    ; s=$03/$04, c=$05 (low byte only)
    ; result in $07/$08
    stz $07
    stz $08
    ldy #0
@loop:
    lda ($03),y
    cmp $05
    bne @skip
    ; Save current position as result
    tya
    clc
    adc $03
    sta $07
    lda #0
    adc $04
    sta $08
@skip:
    lda ($03),y
    beq @done
    iny
    bne @loop
    inc $04
    bra @loop
@done:
    lda $07
    ldx $08
    rts
    endproc

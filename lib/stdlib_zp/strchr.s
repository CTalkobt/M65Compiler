; strchr.s — Locate character in string (ZP calling convention)
;
; char *strchr(char *s, int c);
;   Returns pointer to first occurrence of (char)c in s, or NULL.

.global _strchr

.segment "code"

proc _strchr
    .zp_uses $03, $04, $05, $06
    .zp_clobbers $03, $04, $05, $06
    .zp_release $03, $04, $05, $06
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z

    ; s=$03/$04, c=$05 (low byte only)
    ldy #0
@loop:
    lda ($03),y
    cmp $05
    beq @found
    lda ($03),y
    beq @notfound
    iny
    bne @loop
    inc $04
    bra @loop
@found:
    tya
    clc
    adc $03
    pha
    lda #0
    adc $04
    tax
    pla
    rts
@notfound:
    lda #0
    tax
    rts
    endproc

; strcmp.s — Compare two strings (ZP calling convention)
;
; int strcmp(char *s1, char *s2);
;   Returns 0 if equal, <0 if s1 < s2, >0 if s1 > s2.

.global _strcmp

.segment "code"

proc _strcmp
    .zp_uses $03, $04, $05, $06
    .zp_clobbers $03, $04, $05, $06
    .zp_release $03, $04, $05, $06
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z

    ; s1=$03/$04, s2=$05/$06
    ldy #0
@loop:
    lda ($03),y
    cmp ($05),y
    bne @diff
    lda ($03),y
    beq @equal
    iny
    bne @loop
    inc $04
    inc $06
    bra @loop
@equal:
    lda #0
    tax
    rts
@diff:
    bcs @greater
    lda #$FF
    tax
    rts
@greater:
    lda #1
    ldx #0
    rts
    endproc

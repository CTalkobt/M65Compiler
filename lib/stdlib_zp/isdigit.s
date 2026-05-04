; isdigit.s — Test if character is a decimal digit (ZP calling convention)
;
; int isdigit(int c);
;   Returns 1 if c is '0'-'9' ($30-$39), 0 otherwise.

.global _isdigit

.segment "code"

proc _isdigit
    .zp_uses $03, $04
    .zp_clobbers $03, $04
    .zp_release $03, $04
    .reg_clobbers A, X
    .flag_clobbers C, N, Z

    lda $03
    cmp #$30
    bcc @no
    cmp #$3a
    bcs @no
    lda #1
    ldx #0
    rts
@no:
    lda #0
    ldx #0
    rts
    endproc

; isalnum.s �� Test if character is alphanumeric (ZP calling convention)
;
; int isalnum(int c);
;   Returns 1 if c is a digit ($30-$39) or a letter
;   ($41-$5A lowercase, $C1-$DA uppercase). Returns 0 otherwise.

.global _isalnum

.segment "code"

proc _isalnum
    .zp_uses $03, $04
    .zp_clobbers $03, $04
    .zp_release $03, $04
    .reg_clobbers A, X
    .flag_clobbers C, N, Z

    lda $03
    cmp #$30
    bcc @no
    cmp #$3a
    bcc @yes
    cmp #$41
    bcc @no
    cmp #$5b
    bcc @yes
    cmp #$c1
    bcc @no
    cmp #$db
    bcs @no
@yes:
    lda #1
    ldx #0
    rts
@no:
    lda #0
    ldx #0
    rts
    endproc

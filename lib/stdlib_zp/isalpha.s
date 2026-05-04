; isalpha.s �� Test if character is alphabetic (ZP calling convention)
;
; int isalpha(int c);
;   Returns 1 if c is a letter:
;     lowercase a-z = $41-$5A, uppercase A-Z = $C1-$DA

.global _isalpha

.segment "code"

proc _isalpha
    .zp_uses $03, $04
    .zp_clobbers $03, $04
    .zp_release $03, $04
    .reg_clobbers A, X
    .flag_clobbers C, N, Z

    lda $03
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

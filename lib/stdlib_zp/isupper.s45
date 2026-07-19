; isupper.s — Test if character is uppercase (PETSCII, ZP calling convention)
;
; int isupper(int c);
;   Returns 1 if c is uppercase A-Z ($C1-$DA), 0 otherwise.

.global _isupper

.segment "code"

proc _isupper
    .zp_uses $03, $04
    .zp_clobbers $03, $04
    .zp_release $03, $04
    .reg_clobbers A, X
    .flag_clobbers C, N, Z

    lda $03
    cmp #$c1
    bcc @no
    cmp #$db
    bcs @no
    lda #1
    ldx #0
    rts
@no:
    lda #0
    ldx #0
    rts
    endproc

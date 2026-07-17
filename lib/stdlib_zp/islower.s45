; islower.s — Test if character is lowercase (PETSCII, ZP calling convention)
;
; int islower(int c);
;   Returns 1 if c is lowercase a-z ($41-$5A), 0 otherwise.

.global _islower

.segment "code"

proc _islower
    .zp_uses $03, $04
    .zp_clobbers $03, $04
    .zp_release $03, $04
    .reg_clobbers A, X
    .flag_clobbers C, N, Z

    lda $03
    cmp #$41
    bcc @no
    cmp #$5b
    bcs @no
    lda #1
    ldx #0
    rts
@no:
    lda #0
    ldx #0
    rts
    endproc

; putchar.s — Write a character to screen via KERNAL CHROUT (ZP calling convention)
;
; int putchar(int c);
;   Writes the low byte of c to the screen using KERNAL $FFD2.
;   Returns c.

.global _putchar

.segment "code"

proc _putchar
    .zp_uses $03, $04
    .zp_clobbers $03, $04
    .zp_release $03, $04
    .reg_clobbers A, X
    .flag_clobbers C, N, Z

    lda $03
    jsr $FFD2
    lda $03
    ldx $04
    rts
    endproc

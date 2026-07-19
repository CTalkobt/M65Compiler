; rand.s — Pseudo-random number generator (ZP calling convention)
;
; int rand(void);
;   Returns a pseudo-random integer 0-32767 using MEGA65 hardware RNG.

.global _rand

.segment "code"

proc _rand
    .reg_clobbers A, X
    .flag_clobbers C, N, Z

@wait1:
    lda $D7FE
    bmi @wait1
    lda $D7EF
    pha
@wait2:
    lda $D7FE
    bmi @wait2
    lda $D7EF
    and #$7F
    tax
    pla
    rts
    endproc

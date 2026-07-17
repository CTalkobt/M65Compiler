; srand.s — Seed pseudo-random number generator (ZP calling convention)
;
; void srand(unsigned int seed);
;   No-op: MEGA65 hardware RNG cannot be seeded.

.global _srand

.segment "code"

proc _srand
    .zp_uses $03, $04
    .zp_release $03, $04
    .reg_clobbers A

    ; No-op: MEGA65 hardware RNG cannot be seeded
    rts
    endproc

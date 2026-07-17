; srand.s — Seed pseudo-random number generator
;
; void srand(unsigned int seed);
;   On the MEGA65, the hardware RNG at $D7EF is autonomous and cannot
;   be seeded. This function is provided for C standard compatibility
;   but is a no-op.

.global _srand
.extern __sp_base

.segment "code"

proc _srand, W#_p_seed
    .var _fp = 0
    ; No-op: MEGA65 hardware RNG cannot be seeded
    rtn #0
    endproc

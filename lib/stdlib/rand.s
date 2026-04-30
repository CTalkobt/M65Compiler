; rand.s — Pseudo-random number generator
;
; int rand(void);
;   Returns a pseudo-random integer in the range 0-32767 (0x0000-0x7FFF).
;   Uses the MEGA65 hardware RNG at $D7EF. Busy-waits on $D7FE bit 7
;   until the random number has stabilized before reading.
;   Reads two bytes to produce a 16-bit result with bit 15 cleared
;   (matching C standard: 0 to RAND_MAX where RAND_MAX >= 32767).

.global _rand
.extern __sp_base

.segment "code"

proc _rand
    .var _fp = 0
    ; Wait for RNG ready, read low byte
@wait1:
    lda $D7FE
    bmi @wait1          ; bit 7 set = not ready
    lda $D7EF           ; read random byte (advances LFSR)
    pha                 ; save low byte

    ; Wait for RNG ready, read high byte
@wait2:
    lda $D7FE
    bmi @wait2
    lda $D7EF
    and #$7F            ; clear bit 15 to keep result positive
    tax                 ; high byte in X

    pla                 ; low byte in A
    rtn #0
    endproc

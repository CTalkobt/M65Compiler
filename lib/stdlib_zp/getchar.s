; getchar.s — Read a character from keyboard via KERNAL GETIN (ZP calling convention)
;
; int getchar(void);
;   Waits for a keypress using KERNAL $FFE4 (GETIN).
;   Returns the PETSCII character code in .AX (hi byte = 0).

.global _getchar

.segment "code"

proc _getchar
    .reg_clobbers A, X
    .flag_clobbers C, N, Z

@wait:
    jsr $FFE4
    cmp #0
    beq @wait
    ldx #0
    rts
    endproc

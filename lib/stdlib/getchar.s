; getchar.s — Read a character from keyboard via KERNAL GETIN
;
; int getchar(void);
;   Waits for a keypress using KERNAL $FFE4 (GETIN).
;   Returns the PETSCII character code in .AX (hi byte = 0).

.global _getchar
.extern __sp_base

.segment "code"

proc _getchar
    .var _fp = 0
@wait:
    jsr $FFE4
    cmp #0
    beq @wait
    ldx #0
    rts
    endproc

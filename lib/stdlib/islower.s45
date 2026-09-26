; islower.s — Test if character is lowercase (PETSCII)
;
; int islower(int c);
;   Returns 1 if c is lowercase a-z ($41-$5A), 0 otherwise.

.global _islower
.extern __sp_base

.segment "code"

proc _islower, W#_p_c
    .var _fp = 0
    lda.sp _p_c
    cmp #$41
    bcc @no
    cmp #$5b
    bcs @no
    lda #1
    ldx #0
    rtn #0
@no:
    lda #0
    ldx #0
    rtn #0
    endproc

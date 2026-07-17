; isupper.s — Test if character is uppercase (PETSCII)
;
; int isupper(int c);
;   Returns 1 if c is uppercase A-Z ($C1-$DA), 0 otherwise.

.global _isupper
.extern __sp_base

.segment "code"

proc _isupper, W#_p_c
    .var _fp = 0
    lda.sp _p_c
    cmp #$c1
    bcc @no
    cmp #$db
    bcs @no
    lda #1
    ldx #0
    rtn #0
@no:
    lda #0
    ldx #0
    rtn #0
    endproc

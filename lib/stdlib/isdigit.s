; isdigit.s — Test if character is a decimal digit
;
; int isdigit(int c);
;   Returns 1 if c is '0'-'9' ($30-$39), 0 otherwise.

.global _isdigit

.segment "code"

proc _isdigit, W#_p_c
    .var _fp = 0
    lda.sp _p_c
    cmp #$30
    bcc @no
    cmp #$3a
    bcs @no
    lda #1
    ldx #0
    rtn #0
@no:
    lda #0
    ldx #0
    rtn #0
    endproc

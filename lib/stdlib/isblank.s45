; isblank.s — Test if character is blank (space or tab)
;
; int isblank(int c);
;   Returns 1 if c is space ($20) or tab ($09), 0 otherwise.

.global _isblank
.extern __sp_base

.segment "code"

proc _isblank, W#_p_c
    .var _fp = 0
    lda.sp _p_c
    cmp #$20
    beq @yes
    cmp #$09
    beq @yes
    lda #0
    ldx #0
    rtn #0
@yes:
    lda #1
    ldx #0
    rtn #0
    endproc

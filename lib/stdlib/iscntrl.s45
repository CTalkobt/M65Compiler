; iscntrl.s — Test if character is a control character (PETSCII)
;
; int iscntrl(int c);
;   Returns 1 if c is $00-$1F or $80-$9F (PETSCII control ranges),
;   0 otherwise.

.global _iscntrl
.extern __sp_base

.segment "code"

proc _iscntrl, W#_p_c
    .var _fp = 0
    lda.sp _p_c
    ; Check $00-$1F
    cmp #$20
    bcc @yes
    ; Check $80-$9F
    cmp #$80
    bcc @no
    cmp #$a0
    bcc @yes
@no:
    lda #0
    ldx #0
    rtn #0
@yes:
    lda #1
    ldx #0
    rtn #0
    endproc

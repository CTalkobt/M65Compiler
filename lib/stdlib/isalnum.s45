; isalnum.s — Test if character is alphanumeric (PETSCII)
;
; int isalnum(int c);
;   Returns 1 if c is a digit ($30-$39) or a letter
;   ($41-$5A lowercase, $C1-$DA uppercase). Returns 0 otherwise.

.global _isalnum
.extern __sp_base

.segment "code"

proc _isalnum, W#_p_c
    .var _fp = 0
    lda.sp _p_c
    ; Check digit range $30-$39
    cmp #$30
    bcc @no
    cmp #$3a
    bcc @yes
    ; Check lowercase range $41-$5A
    cmp #$41
    bcc @no
    cmp #$5b
    bcc @yes
    ; Check uppercase range $C1-$DA
    cmp #$c1
    bcc @no
    cmp #$db
    bcs @no
@yes:
    lda #1
    ldx #0
    rtn #0
@no:
    lda #0
    ldx #0
    rtn #0
    endproc

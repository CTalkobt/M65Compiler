; isalpha.s — Test if character is alphabetic (PETSCII)
;
; int isalpha(int c);
;   Returns 1 if c is a letter:
;     lowercase a-z = $41-$5A (PETSCII unshifted)
;     uppercase A-Z = $C1-$DA (PETSCII shifted)
;   Returns 0 otherwise.

.global _isalpha

.segment "code"

proc _isalpha, W#_p_c
    .var _fp = 0
    lda.sp _p_c
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

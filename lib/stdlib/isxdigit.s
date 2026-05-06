; isxdigit.s — Test if character is a hexadecimal digit (PETSCII)
;
; int isxdigit(int c);
;   Returns 1 if c is '0'-'9' ($30-$39), 'a'-'f' ($41-$46),
;   or 'A'-'F' ($C1-$C6). Returns 0 otherwise.

.global _isxdigit
.extern __sp_base

.segment "code"

proc _isxdigit, W#_p_c
    .var _fp = 0
    lda.sp _p_c
    ; Check digits $30-$39
    cmp #$30
    bcc @no
    cmp #$3a
    bcc @yes
    ; Check lowercase a-f ($41-$46)
    cmp #$41
    bcc @no
    cmp #$47
    bcc @yes
    ; Check uppercase A-F ($C1-$C6)
    cmp #$c1
    bcc @no
    cmp #$c7
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

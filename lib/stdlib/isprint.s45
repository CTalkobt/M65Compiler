; isprint.s — Test if character is printable (PETSCII)
;
; int isprint(int c);
;   Returns 1 if c is a printable PETSCII character:
;     $20-$7E (space, digits, lowercase letters, symbols)
;     $A0-$FF (graphic chars, uppercase letters, more graphics)
;   Returns 0 otherwise (control codes $00-$1F, $7F-$9F).

.global _isprint
.extern __sp_base

.segment "code"

proc _isprint, W#_p_c
    .var _fp = 0
    lda.sp _p_c
    ; Check $20-$7E
    cmp #$20
    bcc @no
    cmp #$7f
    bcc @yes
    ; Check $A0-$FF
    cmp #$a0
    bcs @yes
@no:
    lda #0
    ldx #0
    rtn #0
@yes:
    lda #1
    ldx #0
    rtn #0
    endproc

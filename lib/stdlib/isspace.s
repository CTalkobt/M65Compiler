; isspace.s — Test if character is whitespace (PETSCII)
;
; int isspace(int c);
;   Returns 1 if c is a whitespace character:
;     $20 (space), $0D (CR/return), $09 (tab)
;   Returns 0 otherwise.
;
;   Note: PETSCII uses $0D for newline (carriage return).
;   Standard ASCII whitespace \n ($0A), \f ($0C), \v ($0B)
;   are not standard PETSCII but $09-$0D are all accepted
;   for compatibility.

.global _isspace

.segment "code"

proc _isspace, W#_p_c
    .var _fp = 0
    lda.sp _p_c
    cmp #$20
    beq @yes
    ; Check $09-$0D range (tab, LF, VT, FF, CR)
    cmp #$09
    bcc @no
    cmp #$0e
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

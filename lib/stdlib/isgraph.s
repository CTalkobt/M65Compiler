; isgraph.s — Test if character is printable and not a space (PETSCII)
;
; int isgraph(int c);
;   Returns 1 if c is a printable, non-space PETSCII character.
;   Equivalent to: isprint(c) && c != ' '

.global _isgraph
.extern __sp_base

.segment "code"

proc _isgraph, W#_p_c
    .var _fp = 0
    lda.sp _p_c
    ; Reject space ($20)
    cmp #$20
    beq @no
    ; Check $21-$7E (printable non-space)
    cmp #$21
    bcc @no
    cmp #$7f
    bcc @yes
    ; Check $A0 (non-breaking space) — exclude
    cmp #$a0
    beq @no
    ; Check $A1-$FF (graphic chars, uppercase letters)
    cmp #$a1
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

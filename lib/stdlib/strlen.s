; strlen.s — Return length of a NUL-terminated string
;
; int strlen(char *s);
;   Returns the number of bytes before the NUL terminator.
;   Limited to 255 (single-byte Y counter).

.global _strlen
.extern __sp_base

.segment "code"

proc _strlen, W#_p_s
    .var _fp = 0
    ldax _p_s, s
    stax $02
    ldy #0
@loop:
    lda ($02),y
    beq @done
    iny
    bne @loop
@done:
    tya
    ldx #0
    rtn #0
    endproc

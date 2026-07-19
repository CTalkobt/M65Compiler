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
    ; Save ZP $02/$03 (used as pointer; $02 is __zp_scratch)
    lda $02
    pha
    lda $03
    pha
    ldax _p_s+2, sp
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
    ; Restore ZP $02/$03
    taz
    pla
    sta $03
    pla
    sta $02
    tza
    rtn #0
    endproc

; strcpy.s — Copy string
;
; char *strcpy(char *dest, char *src);
;   Copies src including NUL terminator to dest. Returns dest.

.global _strcpy
.extern __sp_base

.segment "code"

proc _strcpy, W#_p_dest, W#_p_src
    .var _fp = 0
    ldax _p_dest, sp
    stax $02
    ldax _p_src, sp
    stax $04
    ldy #0
@loop:
    lda ($04),y
    sta ($02),y
    beq @done
    iny
    bne @loop
    inc $03
    inc $05
    bra @loop
@done:
    ldax _p_dest, sp
    rtn #0
    endproc

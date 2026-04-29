; strcat.s — Concatenate strings
;
; char *strcat(char *dest, char *src);
;   Appends src to the end of dest. Returns dest.

.global _strcat

.segment "code"

proc _strcat, W#_p_dest, W#_p_src
    .var _fp = 0
    ldax _p_dest, s
    stax $02
    ; Find end of dest
    ldy #0
@find_end:
    lda ($02),y
    beq @found
    iny
    bne @find_end
    inc $03
    bra @find_end
@found:
    ; Advance $02 by Y to point at the NUL
    tya
    clc
    adc $02
    sta $02
    lda #0
    adc $03
    sta $03
    ; Now copy src to $02
    ldax _p_src, s
    stax $04
    ldy #0
@copy:
    lda ($04),y
    sta ($02),y
    beq @done
    iny
    bne @copy
    inc $03
    inc $05
    bra @copy
@done:
    ldax _p_dest, s
    rtn #0
    endproc

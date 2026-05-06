; strncat.s — Concatenate at most n characters from src to dest
;
; char *strncat(char *dest, char *src, int n);
;   Appends at most n characters from src to dest, plus NUL.
;   Returns dest.

.global _strncat
.extern __sp_base

.segment "code"

proc _strncat, W#_p_dest, W#_p_src, W#_p_n
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
    ; Load src pointer
    ldax _p_src, s
    stax $04
    ; Load count
    ldax _p_n, s
    stax $06
    ldy #0
@copy:
    ; Check count
    lda $06
    ora $07
    beq @terminate
    lda ($04),y
    sta ($02),y
    beq @done          ; src ended — NUL already written
    ; Decrement count
    lda $06
    bne @nodec_hi
    dec $07
@nodec_hi:
    dec $06
    iny
    bne @copy
    inc $03
    inc $05
    bra @copy
@terminate:
    lda #0
    sta ($02),y
@done:
    ldax _p_dest, s
    rtn #0
    endproc

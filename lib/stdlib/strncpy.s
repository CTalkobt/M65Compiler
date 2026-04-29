; strncpy.s — Copy string with length limit
;
; char *strncpy(char *dest, char *src, int n);
;   Copies at most n bytes from src to dest. If src is shorter,
;   remaining bytes are filled with NUL. Returns dest.

.global _strncpy

.segment "code"

proc _strncpy, W#_p_dest, W#_p_src, W#_p_n
    .var _fp = 0
    ldax _p_dest, s
    stax $02
    ldax _p_src, s
    stax $04
    ldax _p_n, s
    stax $06            ; $06/$07 = count
    ldy #0
@copy:
    ; check count == 0
    lda $06
    ora $07
    beq @done
    lda ($04),y
    sta ($02),y
    beq @pad_start      ; hit NUL in src — pad rest
    iny
    bne @no_inc_copy
    inc $03
    inc $05
@no_inc_copy:
    ; decrement count
    lda $06
    bne @dec_lo_copy
    dec $07
@dec_lo_copy:
    dec $06
    bra @copy
@pad_start:
    ; already wrote one NUL, decrement count
    lda $06
    bne @dec_lo_pad_first
    dec $07
@dec_lo_pad_first:
    dec $06
    lda #0
@pad:
    lda $06
    ora $07
    beq @done
    iny
    bne @no_inc_pad
    inc $03
@no_inc_pad:
    lda #0
    sta ($02),y
    lda $06
    bne @dec_lo_pad
    dec $07
@dec_lo_pad:
    dec $06
    bra @pad
@done:
    ldax _p_dest, s
    rtn #0
    endproc

; memcpy.s — Copy memory area
;
; void *memcpy(void *dest, void *src, int n);
;   Copies n bytes from src to dest. Areas must not overlap.
;   Returns dest.

.global _memcpy
.extern __sp_base

.segment "code"

proc _memcpy, W#_p_dest, W#_p_src, W#_p_n
    .var _fp = 0
    ldax _p_dest, s
    stax $02
    ldax _p_src, s
    stax $04
    ldax _p_n, s
    stax $06            ; $06/$07 = count
    ldy #0
@loop:
    lda $06
    ora $07
    beq @done
    lda ($04),y
    sta ($02),y
    iny
    bne @no_inc
    inc $03
    inc $05
@no_inc:
    lda $06
    bne @dec_lo
    dec $07
@dec_lo:
    dec $06
    bra @loop
@done:
    ldax _p_dest, s
    rtn #0
    endproc

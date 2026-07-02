; memcmp.s — Compare memory areas
;
; int memcmp(void *s1, void *s2, int n);
;   Returns 0 if equal, <0 if s1 < s2, >0 if s1 > s2.

.global _memcmp
.extern __sp_base

.segment "code"

proc _memcmp, W#_p_s1, W#_p_s2, W#_p_n
    .var _fp = 0
    ldax _p_s1, sp
    stax $02
    ldax _p_s2, sp
    stax $04
    ldax _p_n, sp
    stax $06
    ldy #0
@loop:
    lda $06
    ora $07
    beq @equal
    lda ($02),y
    cmp ($04),y
    bne @diff
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
@equal:
    ldax #$0000
    rtn #0
@diff:
    bcs @greater
    ldax #$FFFF
    rtn #0
@greater:
    ldax #$0001
    rtn #0
    endproc

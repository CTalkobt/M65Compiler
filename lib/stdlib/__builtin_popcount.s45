; __builtin_popcount — count set bits
; int __builtin_popcount(unsigned int x);  // x in AX
.global ___builtin_popcount
.segment "code"

proc ___builtin_popcount, W#_p_x
    .var _fp = 0
    ldax.fp _p_x
    sta $20
    stx $21
    ldx #0          ; bit counter
    lda $20
    beq @hi
@lo:
    lsr
    bcc @nolo
    inx
@nolo:
    cmp #0
    bne @lo
@hi:
    lda $21
    beq @done
@hi2:
    lsr
    bcc @nohi
    inx
@nohi:
    cmp #0
    bne @hi2
@done:
    txa
    ldx #0
    .func_flags stack_call, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z
    endproc

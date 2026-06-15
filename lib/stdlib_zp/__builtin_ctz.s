; __builtin_ctz — count trailing zeros (undefined for 0)
; int __builtin_ctz(unsigned int x);  // x in AX
.global ___builtin_ctz
.segment "code"

proc ___builtin_ctz, W#_p_x
    .var _fp = 0
    ldax.fp _p_x
    sta $20
    stx $21
    ldx #0
    lda $20         ; start with low byte
    cmp #0
    bne @bits
    ldx #8
    lda $21         ; check high byte
    cmp #0
    beq @all16
@bits:
    ; Count trailing zeros in A
@shift:
    lsr
    bcs @done
    inx
    bne @shift
@all16:
    ldx #16
@done:
    txa
    ldx #0
    .func_flags stack_call, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z
    endproc

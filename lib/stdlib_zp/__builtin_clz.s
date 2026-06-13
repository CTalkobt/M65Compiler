; __builtin_clz — count leading zeros (undefined for 0)
; int __builtin_clz(unsigned int x);  // x in AX
.global ___builtin_clz
.segment "code"

proc ___builtin_clz, W#_p_x
    .var _fp = 0
    ldax.fp _p_x
    sta $20
    stx $21
    ldx #0          ; counter
    lda $21         ; start with high byte
@hi:
    cmp #0
    bne @hibits
    ldx #8          ; 8 leading zeros from high byte
    lda $20         ; check low byte
@lo:
    cmp #0
    beq @all16
@hibits:
    ; Count leading zeros in A
@shift:
    asl
    bcs @done
    inx
    bne @shift      ; always branches (X won't wrap to 0 before 8 iterations)
@all16:
    ldx #16
@done:
    txa
    ldx #0
    .func_flags stack_call, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z
    endproc

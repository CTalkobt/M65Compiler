; __builtin_ffs — find first set bit (1-indexed), 0 if none
; int __builtin_ffs(int x);
.global ___builtin_ffs
.segment "code"

proc ___builtin_ffs, W#_p_x
    .var _fp = 0
    ldax.fp _p_x
    sta $20
    stx $21
    ora $21
    beq @zero
    lda $20
    ldx #1
@loop:
    lsr
    bcs @found
    inx
    cpx #9
    bne @loop
    lda $21
@loop2:
    lsr
    bcs @found
    inx
    cpx #17
    bne @loop2
@zero:
    lda #0
    ldx #0
    jmp @done
@found:
    txa
    ldx #0
@done:
    .func_flags stack_call, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z
    endproc

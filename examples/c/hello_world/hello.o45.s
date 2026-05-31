    .o45
    .extern __sp_base
    __zp_scratch = $08
    __zp_scratch2 = $0A
    __zp_scratch3 = $0C
    __zp_scratch4 = $0E

    .extern _puts

    .global _main

    .segment "code"

; function _main
    proc _main
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdio.h", 3
; frame: 2 bytes (frame-allocated vRegs only)
    phw #0
    .local __vr0 = 0

@entry:
    .loc "hello.c", 4
    ldax #__str_0
    stax.fp __vr0
    ldax.fp __vr0
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _puts
    .var _fp = _fp - 2
    .loc "hello.c", 5
    lda #0
    ldx #0
@__return:
    plz
    plz
    .func_flags stack_call
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc


    .segment "data"
__str_0:
    .text "hello world"
    .byte 0

__zp_save_buf:

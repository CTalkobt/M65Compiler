* = $2000
    __sp_base = $0101
; Save ZP $08-$FF to BSS buffer
    ldx #0
@__zp_save_loop:
    lda $08,x
    sta __zp_save_buf,x
    inx
    cpx #248
    bne @__zp_save_loop
    jsr _main
    sta $02
    stx $03
; Restore ZP $08-$FF from BSS buffer
    ldx #0
@__zp_restore_loop:
    lda __zp_save_buf,x
    sta $08,x
    inx
    cpx #248
    bne @__zp_restore_loop
    lda $02
    ldx $03
__halt:
    jmp __halt
    __zp_scratch = $08
    __zp_scratch2 = $0A
    __zp_scratch3 = $0C
    __zp_scratch4 = $0E

    .extern _printf


; function _main
    proc _main
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdio.h", 3
; frame: 2 bytes (frame-allocated vRegs only)
    phw #0
    .local __vr0 = 0

@entry:
    .loc "/tmp/test_debug_enc2.c", 4
    ldax #__str_0
    stax.fp __vr0
    ldax.fp __vr0
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _printf
    .var _fp = _fp - 2
    .loc "/tmp/test_debug_enc2.c", 5
    lda #0
    ldx #0
@__return:
    plz
    plz
    .func_flags stack_call
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc


__str_0:
    .text "TEST"
    .byte 0

__zp_save_buf:

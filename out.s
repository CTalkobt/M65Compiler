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
    .global __static_chain
    .global __zp_scratch
    .global __zp_scratch2
    .global __zp_scratch3
    .global __zp_scratch4
    __static_chain = $06
    __zp_scratch = $08
    __zp_scratch2 = $0A
    __zp_scratch3 = $0C
    __zp_scratch4 = $0E


; function _main
    proc _main
    .var _fp = 0
    .loc "/tmp/test_asm_direct.c", 1
    .var @_l_i = $22
    .var @_l_sum = $20

@entry:
    .loc "/tmp/test_asm_direct.c", 2
    lda #0
    ldx #0
    sta $20
    stx $21
    .loc "/tmp/test_asm_direct.c", 3
    .crt_unroll 2
    .loc "/tmp/test_asm_direct.c", 4
    lda #0
    ldx #0
    sta $22
    stx $23
    .loc "/tmp/test_asm_direct.c", 5
    lda $20
    clc
    adc #1
    sta $24
    lda $21
    adc #0
    sta $25
    .loc "/tmp/test_asm_direct.c", 4
    lda $22
    clc
    adc #1
    sta $26
    lda $23
    adc #0
    sta $27
@for_cond0:
    lda $22
    ldx $23
    cmp.16 .AX, #4
    bcc @for_body1
    bra @for_end3
@for_body1:
    .loc "/tmp/test_asm_direct.c", 5
    lda $24
    ldx $25
    sta $20
    stx $21
@for_inc2:
    lda $26
    ldx $27
    sta $22
    stx $23
    bra @for_cond0
@for_end3:
    .loc "/tmp/test_asm_direct.c", 7
    lda $20
    ldx $21
@__return:
    .func_flags stack_call, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc


__zp_save_buf:

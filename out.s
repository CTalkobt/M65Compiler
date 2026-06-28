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


; function _constant_propagation_test
    proc _constant_propagation_test
    .var _fp = 0
    .loc "test_opt_ir.c", 6
    .var @_l_x = $20
    .var @_l_y = $22

@entry:
    .loc "test_opt_ir.c", 7
    lda #42
    ldx #0
    sta $20
    stx $21
    .loc "test_opt_ir.c", 8
    lda $20
    clc
    adc #8
    sta $22
    lda $21
    adc #0
    sta $23
    .loc "test_opt_ir.c", 9
    lda $22
    ldx $23
@__return:
    .func_flags stack_call, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    endproc

; function _copy_propagation_test
    proc _copy_propagation_test
    .var _fp = 0
    .loc "test_opt_ir.c", 12
    .var @_l_a = $20
    .var @_l_b = $22
    .var @_l_c = $24

@entry:
    .loc "test_opt_ir.c", 13
    lda #10
    ldx #0
    sta $20
    stx $21
    .loc "test_opt_ir.c", 14
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_opt_ir.c", 15
    lda $22
    ldx $23
    sta $24
    stx $25
    .loc "test_opt_ir.c", 16
    lda $24
    ldx $25
@__return:
    .func_flags stack_call, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers N, Z
    endproc

; function _unreachable_code_test
    proc _unreachable_code_test
    .var _fp = 0
    .loc "test_opt_ir.c", 19

@entry:
    .loc "test_opt_ir.c", 20
@if_then0:
    .loc "test_opt_ir.c", 21
    lda #100
    ldx #0
    bra @if_end2
@__return:
    .func_flags stack_call, leaf
    .reg_clobbers A, X
    .flag_clobbers N, Z
    endproc

; function _mixed_test
    proc _mixed_test
    .var _fp = 0
    .loc "test_opt_ir.c", 28
    .var @_l_x = $20
    .var @_l_y = $22
    .var @_l_z = $24

@entry:
    .loc "test_opt_ir.c", 29
    lda #5
    ldx #0
    sta $20
    stx $21
    .loc "test_opt_ir.c", 30
; TODO: unimplemented IR op
    lda $26
    ldx $27
    sta $22
    stx $23
    .loc "test_opt_ir.c", 31
    lda $22
    ldx $23
    sta $24
    stx $25
    .loc "test_opt_ir.c", 32
    bra @if_end5
@__return:
    .func_flags stack_call, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers N, Z
    endproc


__zp_save_buf:

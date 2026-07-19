    .o45
    .extern __sp_base
    .weak __static_chain
    .weak __zp_scratch
    .weak __zp_scratch2
    .weak __zp_scratch3
    .weak __zp_scratch4
    __static_chain = $06
    __zp_scratch = $08
    __zp_scratch2 = $0A
    __zp_scratch3 = $0C
    __zp_scratch4 = $0E

    .global _helper
    .global _test_offset

    .segment "code"

; function _helper
    proc _helper, W#@_p_x
    .var _fp = 0
    .loc "test_phase2_minimal.c", 4
; frame: 4 bytes (frame-allocated vRegs only)
    phw #0
    phw #0
    tsy
    tsx
    inx
    bne @__fp_no_carry_0
    iny
@__fp_no_carry_0:
    stx $FD
    sty $FE
    .frameptr_zp $FD
    leax.fp 0
    sta $08
    stx $08+1
    .local __vr0 = 2
    .var @_p_x = 6
; .debug_var: __helper @_p_x offset=6 size=2 type=int16 scope=parameter

    ldax.fp @_p_x
    stax.fp __vr0
@entry:
    .loc "test_phase2_minimal.c", 5
    ldax.fp 2
    add.16 .AX, #1
    sta $22
    stx $23
    lda $22
    ldx $23
@__return:
    plz
    plz
    plz
    plz
    .func_flags stack_call, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    endproc

; function _test_offset
    proc _test_offset
    .var _fp = 0
    .loc "test_phase2_minimal.c", 8
; frame: 6 bytes (frame-allocated vRegs only)
    phw #0
    phw #0
    phw #0
    tsy
    tsx
    inx
    bne @__fp_no_carry_1
    iny
@__fp_no_carry_1:
    stx $FD
    sty $FE
    .frameptr_zp $FD
    leax.fp 0
    sta $0A
    stx $0A+1
    .local __vr0 = 0
    .local __vr2 = 2
    .local __vr3 = 4
    .local @_l_a = 0
    .local @_l_b = 2
; .debug_var: __test_offset @_l_a offset=0 size=2 type=int16 scope=local
; .debug_var: __test_offset @_l_b offset=2 size=2 type=int16 scope=local

@entry:
    .loc "test_phase2_minimal.c", 9
    .loc "test_phase2_minimal.c", 10
    lda #10
    ldx #0
    push .ax
    jsr _helper
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    sta $20
    stx $21
    lda $20
    ldx $21
    stax.fp __vr2
    .loc "test_phase2_minimal.c", 11
    ldax.fp 2
    add.16 .AX, #10
    sta $22
    stx $23
    lda $22
    ldx $23
@__return:
    plz
    plz
    plz
    plz
    plz
    plz
    .func_flags stack_call
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc


__zp_save_buf:

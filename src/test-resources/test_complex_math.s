    .o45
    .org $2000
    .weak __sp_base
    __sp_base = $0101
    .weak __static_chain
    .weak __zp_scratch
    .weak __zp_scratch2
    .weak __zp_scratch3
    .weak __zp_scratch4
    .weak cc45.zeroPageStart
    __static_chain = $06
    __zp_scratch = $08
    __zp_scratch2 = $0A
    __zp_scratch3 = $0C
    __zp_scratch4 = $0E
    cc45.zeroPageStart = $08

    .global _complex_math
    .global _main

    .segment "code"

; function _complex_math
; SAC inline storage: 8 bytes
    .global _complex_math__param_a
    _complex_math__param_a: .word 0
    .global _complex_math__param_b
    _complex_math__param_b: .word 0
    .global _complex_math__param_c
    _complex_math__param_c: .word 0
    _complex_math__local_0: .word 0
    _complex_math__local_1: .word 0
    _complex_math__local_2: .word 0
    _complex_math__local_3: .word 0
    proc _complex_math, W#@_p_a, W#@_p_b, W#@_p_c
    .sac
    .var _fp = 0
    .loc "test_complex_math.c", 1
    .local @_l_res = 6
; .debug_var: __complex_math @_l_res offset=6 size=2 type=int16 scope=local
    .var @_p_a = 2
    .var @_p_b = 4
    .var @_p_c = 6
; .debug_var: __complex_math @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __complex_math @_p_b offset=4 size=2 type=int16 scope=parameter
; .debug_var: __complex_math @_p_c offset=6 size=2 type=int16 scope=parameter

@entry:
    .loc "test_complex_math.c", 2
    lda #0
    sta _complex_math__local_3
    sta _complex_math__local_3+1
    .loc "test_complex_math.c", 4
    lda _complex_math__param_b
    ldx _complex_math__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _complex_math__param_a
    ldx _complex_math__param_a+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda _complex_math__param_c
    ldx _complex_math__param_c+1
    sub.16 .AX, #5
    sta $24
    stx $25
    lda $20
    ldx $21
    mul.16 .AX, $24
    sta $22
    stx $23
    lda #15
    ldx #0
    sta $20
    stx $21
    lda _complex_math__param_a
    ldx _complex_math__param_a+1
    and.16 .AX, $20
    sta $24
    stx $25
    lda _complex_math__param_c
    ldx _complex_math__param_c+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _complex_math__param_b
    ldx _complex_math__param_b+1
    eor.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $24
    ldx $25
    ora.16 .AX, $20
    sta $26
    stx $27
    lda $22
    ldx $23
    div.16 .AX, $26
    sta $20
    stx $21
    lda $20
    ldx $21
    lsl.16 .AX
    lsl.16 .AX
    sta $22
    stx $23
    lda $22
    ldx $23
    sta _complex_math__local_3
    stx _complex_math__local_3+1
    .loc "test_complex_math.c", 7
    lda _complex_math__param_b
    ldx _complex_math__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _complex_math__param_a
    ldx _complex_math__param_a+1
    cmp.16 .AX, __zp_scratch2
    beq @or_rhs3
    bcs @and_rhs4
    bra @or_rhs3
@and_rhs4:
    lda _complex_math__param_c
    ldx _complex_math__param_c+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _complex_math__param_b
    ldx _complex_math__param_b+1
    cmp.16 .AX, __zp_scratch2
    beq @or_rhs3
    bcs @if_then0
    bra @or_rhs3
@or_rhs3:
    lda _complex_math__param_c
    ldx _complex_math__param_c+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _complex_math__param_a
    ldx _complex_math__param_a+1
    cmp.16 .AX, __zp_scratch2
    beq @if_then0
    bra @if_end2
@if_then0:
    .loc "test_complex_math.c", 8
    lda _complex_math__local_3
    ldx _complex_math__local_3+1
    add.16 .AX, #100
    sta $22
    stx $23
    sta _complex_math__local_3
    stx _complex_math__local_3+1
@if_end2:
    .loc "test_complex_math.c", 11
    lda _complex_math__local_3
    ldx _complex_math__local_3+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 8
    endproc

; function _main
; SAC inline storage: 4 bytes
    _main__local_0: .word 0
    _main__local_8: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_complex_math.c", 14
    .local @_l_res = 2
    .local @_l_result = 0
; .debug_var: __main @_l_res offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_result offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_complex_math.c", 15
    lda #5
    ldx #0
    sta $20
    stx $21
    lda #3
    ldx #0
    sta $22
    stx $23
    lda #8
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx $21
    sta $26
    stx $27
    lda $22
    ldx $23
    sta $28
    stx $29
    lda $24
    ldx $25
    sta $2A
    stx $2B
    .loc "test_complex_math.c", 2
    lda #0
    sta _main__local_8
    sta _main__local_8+1
    .loc "test_complex_math.c", 4
    lda $20
    clc
    adc #3
    sta $2C
    lda $21
    adc #0
    sta $2D
    lda $24
    sec
    sbc #5
    sta $30
    lda $25
    sbc #0
    sta $31
    lda $2C
    ldx $2D
    mul.16 .AX, $30
    sta $2E
    stx $2F
    lda #15
    ldx #0
    sta $2C
    stx $2D
    lda $20
    ldx $21
    and.16 .AX, $2C
    sta $30
    stx $31
    lda $22
    ldx $23
    eor.16 .AX, $24
    sta $2C
    stx $2D
    lda $30
    ldx $31
    ora.16 .AX, $2C
    sta $24
    stx $25
    lda $2E
    ldx $2F
    div.16 .AX, $24
    sta $2C
    stx $2D
    lda $2C
    ldx $2D
    lsl.16 .AX
    lsl.16 .AX
    sta $24
    stx $25
    lda $24
    ldx $25
    sta _main__local_8
    stx _main__local_8+1
    .loc "test_complex_math.c", 7
    lda $20
    ldx $21
    cmp.16 .AX, #3
    beq @or_rhs9
    bcs @and_rhs10
    bra @or_rhs9
@and_rhs10:
    lda $28
    ldx $29
    cmp.16 .AX, $2A
    beq @or_rhs9
    bcs @if_then6
    bra @or_rhs9
@or_rhs9:
    lda $26
    ldx $27
    cmp.16 .AX, $2A
    beq @if_then6
    bra @if_end8
@if_then6:
    .loc "test_complex_math.c", 8
    lda _main__local_8
    ldx _main__local_8+1
    add.16 .AX, #100
    sta $22
    stx $23
    sta _main__local_8
    stx _main__local_8+1
@if_end8:
    .loc "test_complex_math.c", 11
@inline_end5:
    .loc "test_complex_math.c", 15
    .loc "test_complex_math.c", 16
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc


__zp_save_buf:

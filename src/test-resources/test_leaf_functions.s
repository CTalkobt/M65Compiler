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

    .global _result
    .global _leaf_add
    .global _leaf_mul
    .global _chain_add
    .global _chain_chain_add
    .global _leaf_complex
    .global _main

    .segment "data"
    .byte 0
_result:
; .debug_var: @global _result offset=0 size=2 type=ptr scope=global
    .word 16384

    .segment "code"

; function _leaf_add
; SAC inline storage: 4 bytes
    .global _leaf_add__param_a
    _leaf_add__param_a: .word 0
    .global _leaf_add__param_b
    _leaf_add__param_b: .word 0
    _leaf_add__local_0: .word 0
    _leaf_add__local_1: .word 0
    proc _leaf_add, W#@_p_a, W#@_p_b
    .sac
    .var _fp = 0
    .loc "test_leaf_functions.c", 7
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __leaf_add @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __leaf_add @_p_b offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_leaf_functions.c", 8
    lda _leaf_add__param_b
    ldx _leaf_add__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _leaf_add__param_a
    ldx _leaf_add__param_a+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _leaf_mul
; SAC inline storage: 4 bytes
    .global _leaf_mul__param_x
    _leaf_mul__param_x: .word 0
    .global _leaf_mul__param_y
    _leaf_mul__param_y: .word 0
    _leaf_mul__local_0: .word 0
    _leaf_mul__local_1: .word 0
    proc _leaf_mul, W#@_p_x, W#@_p_y
    .sac
    .var _fp = 0
    .loc "test_leaf_functions.c", 12
    .var @_p_x = 2
    .var @_p_y = 4
; .debug_var: __leaf_mul @_p_x offset=2 size=2 type=int16 scope=parameter
; .debug_var: __leaf_mul @_p_y offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_leaf_functions.c", 13
    lda _leaf_mul__param_y
    ldx _leaf_mul__param_y+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _leaf_mul__param_x
    ldx _leaf_mul__param_x+1
    mul.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _chain_add
; SAC inline storage: 4 bytes
    .global _chain_add__param_a
    _chain_add__param_a: .word 0
    .global _chain_add__param_b
    _chain_add__param_b: .word 0
    _chain_add__local_0: .word 0
    _chain_add__local_1: .word 0
    proc _chain_add, W#@_p_a, W#@_p_b
    .sac
    .var _fp = 0
    .loc "test_leaf_functions.c", 17
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __chain_add @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __chain_add @_p_b offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_leaf_functions.c", 18
    lda _chain_add__param_a
    ldx _chain_add__param_a+1
    sta $20
    stx $21
    lda _chain_add__param_b
    ldx _chain_add__param_b+1
    sta $20
    stx $21
    .loc "test_leaf_functions.c", 8
    lda _chain_add__param_b
    ldx _chain_add__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _chain_add__param_a
    ldx _chain_add__param_a+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end0:
    .loc "test_leaf_functions.c", 18
    lda $22
    clc
    adc #10
    sta $24
    lda $23
    adc #0
    sta $25
    lda $24
    ldx $25
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _chain_chain_add
; SAC inline storage: 4 bytes
    .global _chain_chain_add__param_a
    _chain_chain_add__param_a: .word 0
    .global _chain_chain_add__param_b
    _chain_chain_add__param_b: .word 0
    _chain_chain_add__local_0: .word 0
    _chain_chain_add__local_1: .word 0
    proc _chain_chain_add, W#@_p_a, W#@_p_b
    .sac
    .var _fp = 0
    .loc "test_leaf_functions.c", 22
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __chain_chain_add @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __chain_chain_add @_p_b offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_leaf_functions.c", 23
    lda _chain_chain_add__param_a
    ldx _chain_chain_add__param_a+1
    sta $20
    stx $21
    lda _chain_chain_add__param_b
    ldx _chain_chain_add__param_b+1
    sta $20
    stx $21
    .loc "test_leaf_functions.c", 18
    lda _chain_chain_add__param_a
    ldx _chain_chain_add__param_a+1
    sta $20
    stx $21
    lda _chain_chain_add__param_b
    ldx _chain_chain_add__param_b+1
    sta $20
    stx $21
    .loc "test_leaf_functions.c", 8
    lda _chain_chain_add__param_b
    ldx _chain_chain_add__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _chain_chain_add__param_a
    ldx _chain_chain_add__param_a+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end3:
    .loc "test_leaf_functions.c", 18
    lda $22
    clc
    adc #10
    sta $24
    lda $23
    adc #0
    sta $25
    lda $24
    ldx $25
    sta $20
    stx $21
@inline_end2:
    .loc "test_leaf_functions.c", 23
    lda $20
    clc
    adc #20
    sta $24
    lda $21
    adc #0
    sta $25
    lda $24
    ldx $25
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _leaf_complex
; SAC inline storage: 6 bytes
    .global _leaf_complex__param_a
    _leaf_complex__param_a: .word 0
    .global _leaf_complex__param_b
    _leaf_complex__param_b: .word 0
    .global _leaf_complex__param_c
    _leaf_complex__param_c: .word 0
    _leaf_complex__local_0: .word 0
    _leaf_complex__local_1: .word 0
    _leaf_complex__local_2: .word 0
    proc _leaf_complex, W#@_p_a, W#@_p_b, W#@_p_c
    .sac
    .var _fp = 0
    .loc "test_leaf_functions.c", 27
    .var @_p_a = 2
    .var @_p_b = 4
    .var @_p_c = 6
; .debug_var: __leaf_complex @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __leaf_complex @_p_b offset=4 size=2 type=int16 scope=parameter
; .debug_var: __leaf_complex @_p_c offset=6 size=2 type=int16 scope=parameter

@entry:
    .loc "test_leaf_functions.c", 28
    lda _leaf_complex__param_b
    ldx _leaf_complex__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _leaf_complex__param_a
    ldx _leaf_complex__param_a+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda _leaf_complex__param_c
    ldx _leaf_complex__param_c+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx $21
    mul.16 .AX, __zp_scratch2
    sta $22
    stx $23
    lda $22
    sec
    sbc #5
    sta $24
    lda $23
    sbc #0
    sta $25
    lda $24
    ldx $25
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _main
; SAC inline storage: 10 bytes
    _main__local_0: .word 0
    _main__local_12: .word 0
    _main__local_24: .word 0
    _main__local_42: .word 0
    _main__local_66: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_leaf_functions.c", 31
    .local @_l_r1 = 0
    .local @_l_r2 = 2
    .local @_l_r3 = 4
    .local @_l_r4 = 6
    .local @_l_r5 = 8
; .debug_var: __main @_l_r1 offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_r2 offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_r3 offset=4 size=2 type=int16 scope=local
; .debug_var: __main @_l_r4 offset=6 size=2 type=int16 scope=local
; .debug_var: __main @_l_r5 offset=8 size=2 type=int16 scope=local

@entry:
    .loc "test_leaf_functions.c", 33
    lda #3
    ldx #0
    sta $20
    stx $21
    lda #4
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    sta $24
    stx $25
    lda $22
    ldx $23
    sta $24
    stx $25
    .loc "test_leaf_functions.c", 8
    lda $20
    clc
    adc #4
    sta $24
    lda $21
    adc #0
    sta $25
    lda $24
    ldx $25
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end6:
    .loc "test_leaf_functions.c", 33
    lda $22
    ldx $23
    sta _main__local_0
    stx _main__local_0+1
    lda _main__local_0
    ldx _main__local_0+1
    sta $20
    .loc "test_leaf_functions.c", 34
    lda _result
    ldx _result+1
    sta $22
    stx $23
    lda #0
    sta $24
    sta $25
    lda $20
    ldx #0
    pha
    lda $24
    ldx $25
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $22
    ldx $22+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "test_leaf_functions.c", 37
    lda #5
    ldx #0
    sta $20
    stx $21
    lda #6
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    sta $24
    stx $25
    lda $22
    ldx $23
    sta $24
    stx $25
    .loc "test_leaf_functions.c", 13
    lda $20
    ldx $21
    mul.16 .AX, $22
    sta $24
    stx $25
    lda $24
    ldx $25
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end8:
    .loc "test_leaf_functions.c", 37
    lda $22
    ldx $23
    sta _main__local_12
    stx _main__local_12+1
    lda _main__local_12
    ldx _main__local_12+1
    sta $20
    .loc "test_leaf_functions.c", 38
    lda _result
    ldx _result+1
    sta $22
    stx $23
    lda #1
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx #0
    pha
    lda $24
    ldx $25
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $22
    ldx $22+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "test_leaf_functions.c", 41
    lda #2
    ldx #0
    sta $20
    stx $21
    lda #3
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    sta $24
    stx $25
    lda $22
    ldx $23
    sta $24
    stx $25
    .loc "test_leaf_functions.c", 18
    lda $20
    ldx $21
    sta $24
    stx $25
    lda $22
    ldx $23
    sta $24
    stx $25
    .loc "test_leaf_functions.c", 8
    lda $20
    clc
    adc #3
    sta $24
    lda $21
    adc #0
    sta $25
    lda $24
    ldx $25
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end11:
    .loc "test_leaf_functions.c", 18
    lda $22
    clc
    adc #10
    sta $24
    lda $23
    adc #0
    sta $25
    lda $24
    ldx $25
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end10:
    .loc "test_leaf_functions.c", 41
    lda $22
    ldx $23
    sta _main__local_24
    stx _main__local_24+1
    lda _main__local_24
    ldx _main__local_24+1
    sta $20
    .loc "test_leaf_functions.c", 42
    lda _result
    ldx _result+1
    sta $22
    stx $23
    lda #2
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx #0
    pha
    lda $24
    ldx $25
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $22
    ldx $22+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "test_leaf_functions.c", 45
    lda #1
    ldx #0
    sta $20
    stx $21
    lda #2
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    sta $24
    stx $25
    lda $22
    ldx $23
    sta $24
    stx $25
    .loc "test_leaf_functions.c", 23
    lda $20
    ldx $21
    sta $24
    stx $25
    lda $22
    ldx $23
    sta $24
    stx $25
    .loc "test_leaf_functions.c", 18
    lda $20
    ldx $21
    sta $24
    stx $25
    lda $22
    ldx $23
    sta $24
    stx $25
    .loc "test_leaf_functions.c", 8
    lda $20
    clc
    adc #2
    sta $24
    lda $21
    adc #0
    sta $25
    lda $24
    ldx $25
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end16:
    .loc "test_leaf_functions.c", 18
    lda $22
    clc
    adc #10
    sta $24
    lda $23
    adc #0
    sta $25
    lda $24
    ldx $25
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end15:
    .loc "test_leaf_functions.c", 23
    lda $22
    clc
    adc #20
    sta $24
    lda $23
    adc #0
    sta $25
    lda $24
    ldx $25
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end14:
    .loc "test_leaf_functions.c", 45
    lda $22
    ldx $23
    sta _main__local_42
    stx _main__local_42+1
    lda _main__local_42
    ldx _main__local_42+1
    sta $20
    .loc "test_leaf_functions.c", 46
    lda _result
    ldx _result+1
    sta $22
    stx $23
    lda #3
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx #0
    pha
    lda $24
    ldx $25
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $22
    ldx $22+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "test_leaf_functions.c", 49
    lda #2
    ldx #0
    sta $20
    stx $21
    lda #3
    ldx #0
    sta $22
    stx $23
    lda #4
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx $21
    sta $26
    stx $27
    lda $22
    ldx $23
    sta $26
    stx $27
    lda $24
    ldx $25
    sta $26
    stx $27
    .loc "test_leaf_functions.c", 28
    lda $20
    clc
    adc #3
    sta $26
    lda $21
    adc #0
    sta $27
    lda $26
    ldx $27
    mul.16 .AX, $24
    sta $20
    stx $21
    lda $20
    sec
    sbc #5
    sta $24
    lda $21
    sbc #0
    sta $25
    lda $24
    ldx $25
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end20:
    .loc "test_leaf_functions.c", 49
    lda $22
    ldx $23
    sta _main__local_66
    stx _main__local_66+1
    lda _main__local_66
    ldx _main__local_66+1
    sta $20
    .loc "test_leaf_functions.c", 50
    lda _result
    ldx _result+1
    sta $22
    stx $23
    lda #4
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx #0
    pha
    lda $24
    ldx $25
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $22
    ldx $22+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "test_leaf_functions.c", 53
    lda #170
    sta $20
    lda _result
    ldx _result+1
    sta $22
    stx $23
    lda #5
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx #0
    pha
    lda $24
    ldx $25
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $22
    ldx $22+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 10
    endproc


__zp_save_buf:

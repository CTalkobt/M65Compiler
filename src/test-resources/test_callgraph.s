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

    .global _simple_add
    .global _simple_mul
    .global _square
    .global _unused_helper
    .global _compute_sum
    .global _compute_product
    .global _chain_func
    .global _main

    .segment "code"

; function _simple_add
; SAC inline storage: 4 bytes
    .global _simple_add__param_a
    _simple_add__param_a: .word 0
    .global _simple_add__param_b
    _simple_add__param_b: .word 0
    _simple_add__local_0: .word 0
    _simple_add__local_1: .word 0
    proc _simple_add, W#@_p_a, W#@_p_b
    .sac
    .var _fp = 0
    .loc "test_callgraph.c", 5
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __simple_add @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __simple_add @_p_b offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_callgraph.c", 6
    lda _simple_add__param_b
    ldx _simple_add__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _simple_add__param_a
    ldx _simple_add__param_a+1
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

; function _simple_mul
; SAC inline storage: 4 bytes
    .global _simple_mul__param_x
    _simple_mul__param_x: .word 0
    .global _simple_mul__param_y
    _simple_mul__param_y: .word 0
    _simple_mul__local_0: .word 0
    _simple_mul__local_1: .word 0
    proc _simple_mul, W#@_p_x, W#@_p_y
    .sac
    .var _fp = 0
    .loc "test_callgraph.c", 10
    .var @_p_x = 2
    .var @_p_y = 4
; .debug_var: __simple_mul @_p_x offset=2 size=2 type=int16 scope=parameter
; .debug_var: __simple_mul @_p_y offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_callgraph.c", 11
    lda _simple_mul__param_y
    ldx _simple_mul__param_y+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _simple_mul__param_x
    ldx _simple_mul__param_x+1
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

; function _square
; SAC inline storage: 2 bytes
    .global _square__param_n
    _square__param_n: .word 0
    _square__local_0: .word 0
    proc _square, W#@_p_n
    .sac
    .var _fp = 0
    .loc "test_callgraph.c", 15
    .var @_p_n = 2
; .debug_var: __square @_p_n offset=2 size=2 type=int16 scope=parameter

@entry:
    .loc "test_callgraph.c", 16
    lda _square__param_n
    ldx _square__param_n+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _square__param_n
    ldx _square__param_n+1
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
    .frame_size 2
    endproc

; function _unused_helper
; SAC inline storage: 2 bytes
    .global _unused_helper__param_n
    _unused_helper__param_n: .word 0
    _unused_helper__local_0: .word 0
    proc _unused_helper, W#@_p_n
    .sac
    .var _fp = 0
    .loc "test_callgraph.c", 20
    .var @_p_n = 2
; .debug_var: __unused_helper @_p_n offset=2 size=2 type=int16 scope=parameter

@entry:
    .loc "test_callgraph.c", 21
    lda _unused_helper__param_n
    ldx _unused_helper__param_n+1
    add.16 .AX, #1
    sta $22
    stx $23
    lda $22
    ldx $23
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _compute_sum
; SAC inline storage: 8 bytes
    .global _compute_sum__param_a
    _compute_sum__param_a: .word 0
    .global _compute_sum__param_b
    _compute_sum__param_b: .word 0
    .global _compute_sum__param_c
    _compute_sum__param_c: .word 0
    _compute_sum__local_0: .word 0
    _compute_sum__local_1: .word 0
    _compute_sum__local_2: .word 0
    _compute_sum__local_3: .word 0
    proc _compute_sum, W#@_p_a, W#@_p_b, W#@_p_c
    .sac
    .var _fp = 0
    .loc "test_callgraph.c", 25
    .local @_l_s = 6
; .debug_var: __compute_sum @_l_s offset=6 size=2 type=int16 scope=local
    .var @_p_a = 2
    .var @_p_b = 4
    .var @_p_c = 6
; .debug_var: __compute_sum @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __compute_sum @_p_b offset=4 size=2 type=int16 scope=parameter
; .debug_var: __compute_sum @_p_c offset=6 size=2 type=int16 scope=parameter

@entry:
    .loc "test_callgraph.c", 26
    lda _compute_sum__param_a
    ldx _compute_sum__param_a+1
    sta $20
    stx $21
    lda _compute_sum__param_b
    ldx _compute_sum__param_b+1
    sta $20
    stx $21
    .loc "test_callgraph.c", 6
    lda _compute_sum__param_b
    ldx _compute_sum__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _compute_sum__param_a
    ldx _compute_sum__param_a+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end0:
    .loc "test_callgraph.c", 26
    lda $22
    ldx $23
    sta _compute_sum__local_3
    stx _compute_sum__local_3+1
    .loc "test_callgraph.c", 27
    lda _compute_sum__local_3
    ldx _compute_sum__local_3+1
    sta $20
    stx $21
    lda _compute_sum__param_c
    ldx _compute_sum__param_c+1
    sta $20
    stx $21
    .loc "test_callgraph.c", 6
    lda _compute_sum__param_c
    ldx _compute_sum__param_c+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _compute_sum__local_3
    ldx _compute_sum__local_3+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end2:
    .loc "test_callgraph.c", 27
    lda $22
    ldx $23
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 8
    endproc

; function _compute_product
; SAC inline storage: 6 bytes
    .global _compute_product__param_x
    _compute_product__param_x: .word 0
    .global _compute_product__param_y
    _compute_product__param_y: .word 0
    .global _compute_product__param_z
    _compute_product__param_z: .word 0
    _compute_product__local_0: .word 0
    _compute_product__local_1: .word 0
    _compute_product__local_2: .word 0
    proc _compute_product, W#@_p_x, W#@_p_y, W#@_p_z
    .sac
    .var _fp = 0
    .loc "test_callgraph.c", 31
    .var @_p_x = 2
    .var @_p_y = 4
    .var @_p_z = 6
; .debug_var: __compute_product @_p_x offset=2 size=2 type=int16 scope=parameter
; .debug_var: __compute_product @_p_y offset=4 size=2 type=int16 scope=parameter
; .debug_var: __compute_product @_p_z offset=6 size=2 type=int16 scope=parameter

@entry:
    .loc "test_callgraph.c", 32
    lda _compute_product__param_x
    ldx _compute_product__param_x+1
    sta $20
    stx $21
    lda _compute_product__param_y
    ldx _compute_product__param_y+1
    sta $20
    stx $21
    .loc "test_callgraph.c", 11
    lda _compute_product__param_y
    ldx _compute_product__param_y+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _compute_product__param_x
    ldx _compute_product__param_x+1
    mul.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end4:
    .loc "test_callgraph.c", 32
    lda $22
    ldx $23
    sta $20
    stx $21
    lda _compute_product__param_z
    ldx _compute_product__param_z+1
    sta $20
    stx $21
    .loc "test_callgraph.c", 11
    lda _compute_product__param_z
    ldx _compute_product__param_z+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $22
    ldx $23
    mul.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end6:
    .loc "test_callgraph.c", 32
    lda $22
    ldx $23
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _chain_func
; SAC inline storage: 2 bytes
    .global _chain_func__param_n
    _chain_func__param_n: .word 0
    _chain_func__local_0: .word 0
    proc _chain_func, W#@_p_n
    .sac
    .var _fp = 0
    .loc "test_callgraph.c", 36
    .var @_p_n = 2
; .debug_var: __chain_func @_p_n offset=2 size=2 type=int16 scope=parameter

@entry:
    .loc "test_callgraph.c", 37
    lda #1
    ldx #0
    sta $20
    stx $21
    lda _chain_func__param_n
    ldx _chain_func__param_n+1
    sta $22
    stx $23
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_callgraph.c", 6
    lda _chain_func__param_n
    ldx _chain_func__param_n+1
    add.16 .AX, $20
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end8:
    .loc "test_callgraph.c", 37
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_callgraph.c", 16
    lda $20
    ldx $21
    mul.16 .AX, $20
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end10:
    .loc "test_callgraph.c", 37
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _main
; SAC inline storage: 8 bytes
    _main__local_0: .word 0
    _main__local_8: .word 0
    _main__local_17: .word 0
    _main__local_33: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_callgraph.c", 40
    .local @_l_a = 0
    .local @_l_b = 4
    .local @_l_c = 6
    .local @_l_s = 2
; .debug_var: __main @_l_a offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_b offset=4 size=2 type=int16 scope=local
; .debug_var: __main @_l_c offset=6 size=2 type=int16 scope=local
; .debug_var: __main @_l_s offset=2 size=2 type=int16 scope=local

@entry:
    .loc "test_callgraph.c", 41
    lda #1
    ldx #0
    sta $20
    stx $21
    lda #2
    ldx #0
    sta $22
    stx $23
    lda #3
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
    .loc "test_callgraph.c", 26
    lda $20
    ldx $21
    sta $24
    stx $25
    lda $22
    ldx $23
    sta $24
    stx $25
    .loc "test_callgraph.c", 6
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
@inline_end13:
    .loc "test_callgraph.c", 26
    lda $20
    ldx $21
    sta _main__local_8
    stx _main__local_8+1
    .loc "test_callgraph.c", 27
    lda _main__local_8
    ldx _main__local_8+1
    sta $20
    stx $21
    lda $26
    ldx $27
    sta $20
    stx $21
    .loc "test_callgraph.c", 6
    lda _main__local_8
    ldx _main__local_8+1
    add.16 .AX, $26
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end15:
    .loc "test_callgraph.c", 27
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end12:
    .loc "test_callgraph.c", 41
    lda $20
    ldx $21
    sta _main__local_0
    stx _main__local_0+1
    .loc "test_callgraph.c", 42
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
    .loc "test_callgraph.c", 32
    lda $20
    ldx $21
    sta $24
    stx $25
    lda $22
    ldx $23
    sta $24
    stx $25
    .loc "test_callgraph.c", 11
    lda $20
    ldx $21
    mul.16 .AX, $22
    sta $24
    stx $25
    lda $24
    ldx $25
    sta $20
    stx $21
@inline_end19:
    .loc "test_callgraph.c", 32
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $26
    ldx $27
    sta $22
    stx $23
    .loc "test_callgraph.c", 11
    lda $20
    ldx $21
    mul.16 .AX, $26
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end21:
    .loc "test_callgraph.c", 32
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end18:
    .loc "test_callgraph.c", 42
    lda $22
    ldx $23
    sta _main__local_17
    stx _main__local_17+1
    .loc "test_callgraph.c", 43
    lda #5
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_callgraph.c", 37
    lda #1
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
    .loc "test_callgraph.c", 6
    lda $20
    clc
    adc #1
    sta $24
    lda $21
    adc #0
    sta $25
    lda $24
    ldx $25
    sta $20
    stx $21
@inline_end25:
    .loc "test_callgraph.c", 37
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_callgraph.c", 16
    lda $20
    ldx $21
    mul.16 .AX, $20
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end27:
    .loc "test_callgraph.c", 37
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end24:
    .loc "test_callgraph.c", 43
    lda $22
    ldx $23
    sta _main__local_33
    stx _main__local_33+1
    .loc "test_callgraph.c", 44
    lda _main__local_17
    ldx _main__local_17+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_0
    ldx _main__local_0+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda _main__local_33
    ldx _main__local_33+1
    add.16 .AX, $20
    sta $22
    stx $23
    lda $22
    ldx $23
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 8
    endproc


__zp_save_buf:

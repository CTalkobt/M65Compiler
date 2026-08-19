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

    .global _add
    .global _multiply
    .global _compute
    .global _add_wrapper
    .global _mul_wrapper
    .global _compute_wrapper
    .global _chain_call
    .global _main

    .segment "code"

; function _add
; SAC inline storage: 4 bytes
    .global _add__param_a
    _add__param_a: .word 0
    .global _add__param_b
    _add__param_b: .word 0
    _add__local_0: .word 0
    _add__local_1: .word 0
    proc _add, W#@_p_a, W#@_p_b
    .sac
    .var _fp = 0
    .loc "test_phase4_object_attrs.c", 5
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __add @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __add @_p_b offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_phase4_object_attrs.c", 6
    lda _add__param_b
    ldx _add__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _add__param_a
    ldx _add__param_a+1
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

; function _multiply
; SAC inline storage: 4 bytes
    .global _multiply__param_x
    _multiply__param_x: .word 0
    .global _multiply__param_y
    _multiply__param_y: .word 0
    _multiply__local_0: .word 0
    _multiply__local_1: .word 0
    proc _multiply, W#@_p_x, W#@_p_y
    .sac
    .var _fp = 0
    .loc "test_phase4_object_attrs.c", 10
    .var @_p_x = 2
    .var @_p_y = 4
; .debug_var: __multiply @_p_x offset=2 size=2 type=int16 scope=parameter
; .debug_var: __multiply @_p_y offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_phase4_object_attrs.c", 11
    lda _multiply__param_y
    ldx _multiply__param_y+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _multiply__param_x
    ldx _multiply__param_x+1
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

; function _compute
; SAC inline storage: 6 bytes
    .global _compute__param_a
    _compute__param_a: .word 0
    .global _compute__param_b
    _compute__param_b: .word 0
    .global _compute__param_c
    _compute__param_c: .word 0
    _compute__local_0: .word 0
    _compute__local_1: .word 0
    _compute__local_2: .word 0
    proc _compute, W#@_p_a, W#@_p_b, W#@_p_c
    .sac
    .var _fp = 0
    .loc "test_phase4_object_attrs.c", 15
    .var @_p_a = 2
    .var @_p_b = 4
    .var @_p_c = 6
; .debug_var: __compute @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __compute @_p_b offset=4 size=2 type=int16 scope=parameter
; .debug_var: __compute @_p_c offset=6 size=2 type=int16 scope=parameter

@entry:
    .loc "test_phase4_object_attrs.c", 16
    lda _compute__param_b
    ldx _compute__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _compute__param_a
    ldx _compute__param_a+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda _compute__param_c
    ldx _compute__param_c+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx $21
    mul.16 .AX, __zp_scratch2
    sta $22
    stx $23
    lda $22
    sec
    sbc #10
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

; function _add_wrapper
; SAC inline storage: 4 bytes
    .global _add_wrapper__param_x
    _add_wrapper__param_x: .word 0
    .global _add_wrapper__param_y
    _add_wrapper__param_y: .word 0
    _add_wrapper__local_0: .word 0
    _add_wrapper__local_1: .word 0
    proc _add_wrapper, W#@_p_x, W#@_p_y
    .sac
    .var _fp = 0
    .loc "test_phase4_object_attrs.c", 20
    .var @_p_x = 2
    .var @_p_y = 4
; .debug_var: __add_wrapper @_p_x offset=2 size=2 type=int16 scope=parameter
; .debug_var: __add_wrapper @_p_y offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_phase4_object_attrs.c", 21
    lda _add_wrapper__param_x
    ldx _add_wrapper__param_x+1
    sta $20
    stx $21
    lda _add_wrapper__param_y
    ldx _add_wrapper__param_y+1
    sta $20
    stx $21
    .loc "test_phase4_object_attrs.c", 6
    lda _add_wrapper__param_y
    ldx _add_wrapper__param_y+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _add_wrapper__param_x
    ldx _add_wrapper__param_x+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end0:
    .loc "test_phase4_object_attrs.c", 21
    lda $22
    ldx $23
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _mul_wrapper
; SAC inline storage: 4 bytes
    .global _mul_wrapper__param_x
    _mul_wrapper__param_x: .word 0
    .global _mul_wrapper__param_y
    _mul_wrapper__param_y: .word 0
    _mul_wrapper__local_0: .word 0
    _mul_wrapper__local_1: .word 0
    proc _mul_wrapper, W#@_p_x, W#@_p_y
    .sac
    .var _fp = 0
    .loc "test_phase4_object_attrs.c", 25
    .var @_p_x = 2
    .var @_p_y = 4
; .debug_var: __mul_wrapper @_p_x offset=2 size=2 type=int16 scope=parameter
; .debug_var: __mul_wrapper @_p_y offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_phase4_object_attrs.c", 26
    lda _mul_wrapper__param_x
    ldx _mul_wrapper__param_x+1
    sta $20
    stx $21
    lda _mul_wrapper__param_y
    ldx _mul_wrapper__param_y+1
    sta $20
    stx $21
    .loc "test_phase4_object_attrs.c", 11
    lda _mul_wrapper__param_y
    ldx _mul_wrapper__param_y+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _mul_wrapper__param_x
    ldx _mul_wrapper__param_x+1
    mul.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end2:
    .loc "test_phase4_object_attrs.c", 26
    lda $22
    ldx $23
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _compute_wrapper
; SAC inline storage: 6 bytes
    .global _compute_wrapper__param_a
    _compute_wrapper__param_a: .word 0
    .global _compute_wrapper__param_b
    _compute_wrapper__param_b: .word 0
    .global _compute_wrapper__param_c
    _compute_wrapper__param_c: .word 0
    _compute_wrapper__local_0: .word 0
    _compute_wrapper__local_1: .word 0
    _compute_wrapper__local_2: .word 0
    proc _compute_wrapper, W#@_p_a, W#@_p_b, W#@_p_c
    .sac
    .var _fp = 0
    .loc "test_phase4_object_attrs.c", 30
    .var @_p_a = 2
    .var @_p_b = 4
    .var @_p_c = 6
; .debug_var: __compute_wrapper @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __compute_wrapper @_p_b offset=4 size=2 type=int16 scope=parameter
; .debug_var: __compute_wrapper @_p_c offset=6 size=2 type=int16 scope=parameter

@entry:
    .loc "test_phase4_object_attrs.c", 31
    lda _compute_wrapper__param_a
    ldx _compute_wrapper__param_a+1
    sta $20
    stx $21
    lda _compute_wrapper__param_b
    ldx _compute_wrapper__param_b+1
    sta $20
    stx $21
    lda _compute_wrapper__param_c
    ldx _compute_wrapper__param_c+1
    sta $20
    stx $21
    .loc "test_phase4_object_attrs.c", 16
    lda _compute_wrapper__param_b
    ldx _compute_wrapper__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _compute_wrapper__param_a
    ldx _compute_wrapper__param_a+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda _compute_wrapper__param_c
    ldx _compute_wrapper__param_c+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx $21
    mul.16 .AX, __zp_scratch2
    sta $22
    stx $23
    lda $22
    sec
    sbc #10
    sta $24
    lda $23
    sbc #0
    sta $25
    lda $24
    ldx $25
    sta $20
    stx $21
@inline_end4:
    .loc "test_phase4_object_attrs.c", 31
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _chain_call
; SAC inline storage: 6 bytes
    .global _chain_call__param_a
    _chain_call__param_a: .word 0
    .global _chain_call__param_b
    _chain_call__param_b: .word 0
    .global _chain_call__param_c
    _chain_call__param_c: .word 0
    _chain_call__local_0: .word 0
    _chain_call__local_1: .word 0
    _chain_call__local_2: .word 0
    proc _chain_call, W#@_p_a, W#@_p_b, W#@_p_c
    .sac
    .var _fp = 0
    .loc "test_phase4_object_attrs.c", 35
    .var @_p_a = 2
    .var @_p_b = 4
    .var @_p_c = 6
; .debug_var: __chain_call @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __chain_call @_p_b offset=4 size=2 type=int16 scope=parameter
; .debug_var: __chain_call @_p_c offset=6 size=2 type=int16 scope=parameter

@entry:
    .loc "test_phase4_object_attrs.c", 36
    lda _chain_call__param_a
    ldx _chain_call__param_a+1
    sta $20
    stx $21
    lda _chain_call__param_b
    ldx _chain_call__param_b+1
    sta $20
    stx $21
    .loc "test_phase4_object_attrs.c", 21
    lda _chain_call__param_a
    ldx _chain_call__param_a+1
    sta $20
    stx $21
    lda _chain_call__param_b
    ldx _chain_call__param_b+1
    sta $20
    stx $21
    .loc "test_phase4_object_attrs.c", 6
    lda _chain_call__param_b
    ldx _chain_call__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _chain_call__param_a
    ldx _chain_call__param_a+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end7:
    .loc "test_phase4_object_attrs.c", 21
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end6:
    .loc "test_phase4_object_attrs.c", 36
    lda _chain_call__param_a
    ldx _chain_call__param_a+1
    sta $22
    stx $23
    lda _chain_call__param_b
    ldx _chain_call__param_b+1
    sta $22
    stx $23
    lda _chain_call__param_c
    ldx _chain_call__param_c+1
    sta $22
    stx $23
    .loc "test_phase4_object_attrs.c", 31
    lda _chain_call__param_a
    ldx _chain_call__param_a+1
    sta $22
    stx $23
    lda _chain_call__param_b
    ldx _chain_call__param_b+1
    sta $22
    stx $23
    lda _chain_call__param_c
    ldx _chain_call__param_c+1
    sta $22
    stx $23
    .loc "test_phase4_object_attrs.c", 16
    lda _chain_call__param_b
    ldx _chain_call__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _chain_call__param_a
    ldx _chain_call__param_a+1
    add.16 .AX, __zp_scratch2
    sta $22
    stx $23
    lda _chain_call__param_c
    ldx _chain_call__param_c+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $22
    ldx $23
    mul.16 .AX, __zp_scratch2
    sta $24
    stx $25
    lda $24
    sec
    sbc #10
    sta $26
    lda $25
    sbc #0
    sta $27
    lda $26
    ldx $27
    sta $22
    stx $23
@inline_end11:
    .loc "test_phase4_object_attrs.c", 31
    lda $22
    ldx $23
    sta $24
    stx $25
@inline_end10:
    .loc "test_phase4_object_attrs.c", 36
    lda $20
    clc
    adc $24
    sta $22
    lda $21
    adc $24+1
    sta $23
    lda $22
    ldx $23
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _main
; SAC inline storage: 8 bytes
    _main__local_0: .word 0
    _main__local_8: .word 0
    _main__local_16: .word 0
    _main__local_29: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_phase4_object_attrs.c", 39
    .local @_l_r1 = 0
    .local @_l_r2 = 2
    .local @_l_r3 = 4
    .local @_l_r4 = 6
; .debug_var: __main @_l_r1 offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_r2 offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_r3 offset=4 size=2 type=int16 scope=local
; .debug_var: __main @_l_r4 offset=6 size=2 type=int16 scope=local

@entry:
    .loc "test_phase4_object_attrs.c", 42
    .loc "test_phase4_object_attrs.c", 6
@inline_end14:
    .loc "test_phase4_object_attrs.c", 42
    .loc "test_phase4_object_attrs.c", 43
    .loc "test_phase4_object_attrs.c", 11
@inline_end16:
    .loc "test_phase4_object_attrs.c", 43
    .loc "test_phase4_object_attrs.c", 44
    .loc "test_phase4_object_attrs.c", 16
@inline_end18:
    .loc "test_phase4_object_attrs.c", 44
    .loc "test_phase4_object_attrs.c", 45
    .loc "test_phase4_object_attrs.c", 36
    .loc "test_phase4_object_attrs.c", 21
    .loc "test_phase4_object_attrs.c", 6
@inline_end22:
    .loc "test_phase4_object_attrs.c", 21
@inline_end21:
    .loc "test_phase4_object_attrs.c", 36
    .loc "test_phase4_object_attrs.c", 31
    .loc "test_phase4_object_attrs.c", 16
@inline_end26:
    .loc "test_phase4_object_attrs.c", 31
@inline_end25:
    .loc "test_phase4_object_attrs.c", 36
@inline_end20:
    .loc "test_phase4_object_attrs.c", 45
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .frame_size 8
    endproc


__zp_save_buf:

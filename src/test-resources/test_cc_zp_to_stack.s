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

    .global _stack_add
    .global _zp_caller
    .global _main

    .segment "code"

; function _stack_add
; SAC inline storage: 4 bytes
    .global _stack_add__param_a
    _stack_add__param_a: .word 0
    .global _stack_add__param_b
    _stack_add__param_b: .word 0
    _stack_add__local_0: .word 0
    _stack_add__local_1: .word 0
    proc _stack_add, W#@_p_a, W#@_p_b
    .sac
    .var _fp = 0
    .loc "test_cc_zp_to_stack.c", 5
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __stack_add @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __stack_add @_p_b offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_cc_zp_to_stack.c", 6
    lda _stack_add__param_b
    ldx _stack_add__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _stack_add__param_a
    ldx _stack_add__param_a+1
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

; function _zp_caller
; SAC inline storage: 2 bytes
    .global _zp_caller__param_x
    _zp_caller__param_x: .word 0
    _zp_caller__local_0: .word 0
    proc _zp_caller, W#@_p_x
    .sac
    .var _fp = 0
    .loc "test_cc_zp_to_stack.c", 10
    .var @_p_x = 2
; .debug_var: __zp_caller @_p_x offset=2 size=2 type=int16 scope=parameter

@entry:
    .loc "test_cc_zp_to_stack.c", 13
    lda #10
    ldx #0
    sta $20
    stx $21
    lda _zp_caller__param_x
    ldx _zp_caller__param_x+1
    sta $22
    stx $23
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_cc_zp_to_stack.c", 6
    lda _zp_caller__param_x
    ldx _zp_caller__param_x+1
    add.16 .AX, $20
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end0:
    .loc "test_cc_zp_to_stack.c", 13
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
; SAC zero-alloc leaf: no storage overhead
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_cc_zp_to_stack.c", 16

@entry:
    .loc "test_cc_zp_to_stack.c", 17
    brk
@__return:
    rts
    .func_flags stack_call, static_alloc, zeroalloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 0
    endproc


__zp_save_buf:

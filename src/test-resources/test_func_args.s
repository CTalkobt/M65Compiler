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
    .global _sub
    .global _three_args
    .global _nested_call
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
    .loc "test_func_args.c", 4
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __add @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __add @_p_b offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_func_args.c", 5
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

; function _sub
; SAC inline storage: 4 bytes
    .global _sub__param_a
    _sub__param_a: .word 0
    .global _sub__param_b
    _sub__param_b: .word 0
    _sub__local_0: .word 0
    _sub__local_1: .word 0
    proc _sub, W#@_p_a, W#@_p_b
    .sac
    .var _fp = 0
    .loc "test_func_args.c", 8
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __sub @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __sub @_p_b offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_func_args.c", 9
    lda _sub__param_b
    ldx _sub__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _sub__param_a
    ldx _sub__param_a+1
    sub.16 .AX, __zp_scratch2
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

; function _three_args
; SAC inline storage: 6 bytes
    .global _three_args__param_a
    _three_args__param_a: .word 0
    .global _three_args__param_b
    _three_args__param_b: .word 0
    .global _three_args__param_c
    _three_args__param_c: .word 0
    _three_args__local_0: .word 0
    _three_args__local_1: .word 0
    _three_args__local_2: .word 0
    proc _three_args, W#@_p_a, W#@_p_b, W#@_p_c
    .sac
    .var _fp = 0
    .loc "test_func_args.c", 12
    .var @_p_a = 2
    .var @_p_b = 4
    .var @_p_c = 6
; .debug_var: __three_args @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __three_args @_p_b offset=4 size=2 type=int16 scope=parameter
; .debug_var: __three_args @_p_c offset=6 size=2 type=int16 scope=parameter

@entry:
    .loc "test_func_args.c", 13
    lda _three_args__param_b
    ldx _three_args__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _three_args__param_a
    ldx _three_args__param_a+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda _three_args__param_c
    ldx _three_args__param_c+1
    add.16 .AX, $20
    sta $22
    stx $23
    lda $22
    ldx $23
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _nested_call
; SAC inline storage: 2 bytes
    .global _nested_call__param_x
    _nested_call__param_x: .word 0
    _nested_call__local_0: .word 0
    proc _nested_call, W#@_p_x
    .sac
    .var _fp = 0
    .loc "test_func_args.c", 16
    .var @_p_x = 2
; .debug_var: __nested_call @_p_x offset=2 size=2 type=int16 scope=parameter

@entry:
    .loc "test_func_args.c", 17
    lda _nested_call__param_x
    ldx _nested_call__param_x+1
    sta $20
    stx $21
    lda _nested_call__param_x
    ldx _nested_call__param_x+1
    sta $20
    stx $21
    .loc "test_func_args.c", 5
    lda _nested_call__param_x
    ldx _nested_call__param_x+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _nested_call__param_x
    ldx _nested_call__param_x+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end0:
    .loc "test_func_args.c", 17
    lda $22
    ldx $23
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _main
; SAC inline storage: 18 bytes
    _main__local_0: .word 0
    _main__local_2: .word 0
    _main__local_4: .word 0
    _main__local_11: .word 0
    _main__local_18: .word 0
    _main__local_20: .word 0
    _main__local_29: .word 0
    _main__local_40: .word 0
    _main__local_49: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_func_args.c", 20
    .local @_l_a = 0
    .local @_l_b = 2
    .local @_l_c = 8
    .local @_l_r1 = 4
    .local @_l_r2 = 6
    .local @_l_r3 = 10
    .local @_l_r4 = 12
    .local @_l_r5 = 14
    .local @_l_r6 = 16
; .debug_var: __main @_l_a offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_b offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_c offset=8 size=2 type=int16 scope=local
; .debug_var: __main @_l_r1 offset=4 size=2 type=int16 scope=local
; .debug_var: __main @_l_r2 offset=6 size=2 type=int16 scope=local
; .debug_var: __main @_l_r3 offset=10 size=2 type=int16 scope=local
; .debug_var: __main @_l_r4 offset=12 size=2 type=int16 scope=local
; .debug_var: __main @_l_r5 offset=14 size=2 type=int16 scope=local
; .debug_var: __main @_l_r6 offset=16 size=2 type=int16 scope=local

@entry:
    .loc "test_func_args.c", 21
    lda #100
    sta _main__local_0
    lda #0
    sta _main__local_0+1
    .loc "test_func_args.c", 22
    lda #50
    sta _main__local_2
    lda #0
    sta _main__local_2+1
    .loc "test_func_args.c", 25
    lda _main__local_0
    ldx _main__local_0+1
    sta $20
    stx $21
    lda _main__local_2
    ldx _main__local_2+1
    sta $20
    stx $21
    .loc "test_func_args.c", 5
    lda _main__local_2
    ldx _main__local_2+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_0
    ldx _main__local_0+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end2:
    .loc "test_func_args.c", 25
    lda $22
    ldx $23
    sta _main__local_4
    stx _main__local_4+1
    .loc "test_func_args.c", 26
    lda _main__local_4
    ldx _main__local_4+1
    cmp.16 .AX, #150
    bne @if_then4
    bra @if_end6
@if_then4:
    lda #1
    ldx #0
    bra @__return
@if_end6:
    .loc "test_func_args.c", 29
    lda _main__local_0
    ldx _main__local_0+1
    sta $20
    stx $21
    lda _main__local_2
    ldx _main__local_2+1
    sta $20
    stx $21
    .loc "test_func_args.c", 9
    lda _main__local_2
    ldx _main__local_2+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_0
    ldx _main__local_0+1
    sub.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end7:
    .loc "test_func_args.c", 29
    lda $22
    ldx $23
    sta _main__local_11
    stx _main__local_11+1
    .loc "test_func_args.c", 30
    lda _main__local_11
    ldx _main__local_11+1
    cmp.16 .AX, #50
    bne @if_then9
    bra @if_end11
@if_then9:
    lda #2
    ldx #0
    bra @__return
@if_end11:
    .loc "test_func_args.c", 33
    lda #25
    sta _main__local_18
    lda #0
    sta _main__local_18+1
    .loc "test_func_args.c", 34
    lda _main__local_0
    ldx _main__local_0+1
    sta $20
    stx $21
    lda _main__local_2
    ldx _main__local_2+1
    sta $20
    stx $21
    lda _main__local_18
    ldx _main__local_18+1
    sta $20
    stx $21
    .loc "test_func_args.c", 13
    lda _main__local_2
    ldx _main__local_2+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_0
    ldx _main__local_0+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda _main__local_18
    ldx _main__local_18+1
    add.16 .AX, $20
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end12:
    .loc "test_func_args.c", 34
    lda $20
    ldx $21
    sta _main__local_20
    stx _main__local_20+1
    .loc "test_func_args.c", 35
    lda _main__local_20
    ldx _main__local_20+1
    cmp.16 .AX, #175
    bne @if_then14
    bra @if_end16
@if_then14:
    lda #3
    ldx #0
    bra @__return
@if_end16:
    .loc "test_func_args.c", 38
    lda _main__local_0
    ldx _main__local_0+1
    add.16 .AX, #10
    sta $22
    stx $23
    lda _main__local_2
    ldx _main__local_2+1
    sub.16 .AX, #10
    sta $24
    stx $25
    lda $22
    ldx $23
    sta $20
    stx $21
    lda $24
    ldx $25
    sta $20
    stx $21
    .loc "test_func_args.c", 5
    lda $22
    clc
    adc $24
    sta $20
    lda $23
    adc $24+1
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end17:
    .loc "test_func_args.c", 38
    lda $22
    ldx $23
    sta _main__local_29
    stx _main__local_29+1
    .loc "test_func_args.c", 39
    lda _main__local_29
    ldx _main__local_29+1
    cmp.16 .AX, #150
    bne @if_then19
    bra @if_end21
@if_then19:
    lda #4
    ldx #0
    bra @__return
@if_end21:
    .loc "test_func_args.c", 42
    lda _main__local_0
    ldx _main__local_0+1
    sta $20
    stx $21
    .loc "test_func_args.c", 17
    lda _main__local_0
    ldx _main__local_0+1
    sta $20
    stx $21
    lda _main__local_0
    ldx _main__local_0+1
    sta $20
    stx $21
    .loc "test_func_args.c", 5
    lda _main__local_0
    ldx _main__local_0+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_0
    ldx _main__local_0+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end23:
    .loc "test_func_args.c", 17
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end22:
    .loc "test_func_args.c", 42
    lda $20
    ldx $21
    sta _main__local_40
    stx _main__local_40+1
    .loc "test_func_args.c", 43
    lda _main__local_40
    ldx _main__local_40+1
    cmp.16 .AX, #200
    bne @if_then26
    bra @if_end28
@if_then26:
    lda #5
    ldx #0
    bra @__return
@if_end28:
    .loc "test_func_args.c", 46
    lda _main__local_4
    ldx _main__local_4+1
    sta $20
    stx $21
    lda _main__local_11
    ldx _main__local_11+1
    sta $20
    stx $21
    .loc "test_func_args.c", 5
    lda _main__local_11
    ldx _main__local_11+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_4
    ldx _main__local_4+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end29:
    .loc "test_func_args.c", 46
    lda $22
    ldx $23
    sta _main__local_49
    stx _main__local_49+1
    .loc "test_func_args.c", 47
    lda _main__local_49
    ldx _main__local_49+1
    cmp.16 .AX, #200
    bne @if_then31
    bra @if_end33
@if_then31:
    lda #6
    ldx #0
    bra @__return
@if_end33:
    .loc "test_func_args.c", 49
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 18
    endproc


__zp_save_buf:

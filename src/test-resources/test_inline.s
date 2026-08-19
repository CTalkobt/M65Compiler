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
    .global _square
    .global _identity
    .global _noop
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
    .loc "test_inline.c", 4
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __add @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __add @_p_b offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_inline.c", 5
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

; function _square
; SAC inline storage: 2 bytes
    .global _square__param_x
    _square__param_x: .word 0
    _square__local_0: .word 0
    proc _square, W#@_p_x
    .sac
    .var _fp = 0
    .loc "test_inline.c", 8
    .var @_p_x = 2
; .debug_var: __square @_p_x offset=2 size=2 type=int16 scope=parameter

@entry:
    .loc "test_inline.c", 9
    lda _square__param_x
    ldx _square__param_x+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _square__param_x
    ldx _square__param_x+1
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

; function _identity
; SAC inline storage: 2 bytes
    .global _identity__param_x
    _identity__param_x: .word 0
    _identity__local_0: .word 0
    proc _identity, W#@_p_x
    .sac
    .var _fp = 0
    .loc "test_inline.c", 12
    .var @_p_x = 2
; .debug_var: __identity @_p_x offset=2 size=2 type=int16 scope=parameter

@entry:
    .loc "test_inline.c", 13
    lda _identity__param_x
    ldx _identity__param_x+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers N, Z
    .frame_size 2
    endproc

; function _noop
; SAC zero-alloc leaf: no storage overhead
    proc _noop
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_inline.c", 16

@entry:
    .loc "test_inline.c", 17
@__return:
    rts
    .func_flags stack_call, static_alloc, zeroalloc, leaf
    .frame_size 0
    endproc

; function _main
; SAC inline storage: 4 bytes
    _main__local_32: .word 0
    _main__local_39: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_inline.c", 20
    .local @_l_x = 0
    .local @_l_y = 2
; .debug_var: __main @_l_x offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_y offset=2 size=2 type=int16 scope=local

@entry:
    .loc "test_inline.c", 22
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
    .loc "test_inline.c", 5
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
@inline_end3:
    .loc "test_inline.c", 22
    lda $20
    ldx $21
    cmp.16 .AX, #7
    bne @if_then0
    bra @if_end2
@if_then0:
    lda #1
    ldx #0
    bra @__return
@if_end2:
    .loc "test_inline.c", 25
    lda #10
    ldx #0
    sta $20
    stx $21
    lda #20
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
    .loc "test_inline.c", 5
    lda $20
    clc
    adc #20
    sta $24
    lda $21
    adc #0
    sta $25
    lda $24
    ldx $25
    sta $20
    stx $21
@inline_end8:
    .loc "test_inline.c", 25
    lda $20
    ldx $21
    cmp.16 .AX, #30
    bne @if_then5
    bra @if_end7
@if_then5:
    lda #2
    ldx #0
    bra @__return
@if_end7:
    .loc "test_inline.c", 28
    lda #3
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_inline.c", 9
    lda $20
    ldx $21
    mul.16 .AX, $20
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end13:
    .loc "test_inline.c", 28
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
    .loc "test_inline.c", 5
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
@inline_end15:
    .loc "test_inline.c", 28
    lda $20
    ldx $21
    cmp.16 .AX, #10
    bne @if_then10
    bra @if_end12
@if_then10:
    lda #3
    ldx #0
    bra @__return
@if_end12:
    .loc "test_inline.c", 31
    lda #42
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_inline.c", 13
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end20:
    .loc "test_inline.c", 31
    lda $22
    ldx $23
    cmp.16 .AX, #42
    bne @if_then17
    bra @inline_end22
@if_then17:
    lda #4
    ldx #0
    bra @__return
@inline_end22:
    .loc "test_inline.c", 37
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
    .loc "test_inline.c", 5
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
@inline_end24:
    .loc "test_inline.c", 37
    lda $20
    ldx $21
    sta _main__local_32
    stx _main__local_32+1
    .loc "test_inline.c", 38
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
    .loc "test_inline.c", 5
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
@inline_end26:
    .loc "test_inline.c", 38
    lda $20
    ldx $21
    sta _main__local_39
    stx _main__local_39+1
    .loc "test_inline.c", 39
    lda _main__local_39
    ldx _main__local_39+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_32
    ldx _main__local_32+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #10
    bne @if_then28
    bra @if_end30
@if_then28:
    lda #5
    ldx #0
    bra @__return
@if_end30:
    .loc "test_inline.c", 41
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

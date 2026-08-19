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
    .global _subtract
    .global _compute
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
    .loc "test_sac_nested.c", 5
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __add @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __add @_p_b offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_sac_nested.c", 6
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

; function _subtract
; SAC inline storage: 4 bytes
    .global _subtract__param_a
    _subtract__param_a: .word 0
    .global _subtract__param_b
    _subtract__param_b: .word 0
    _subtract__local_0: .word 0
    _subtract__local_1: .word 0
    proc _subtract, W#@_p_a, W#@_p_b
    .sac
    .var _fp = 0
    .loc "test_sac_nested.c", 9
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __subtract @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __subtract @_p_b offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_sac_nested.c", 10
    lda _subtract__param_b
    ldx _subtract__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _subtract__param_a
    ldx _subtract__param_a+1
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

; function _compute
; SAC inline storage: 10 bytes
    .global _compute__param_x
    _compute__param_x: .word 0
    .global _compute__param_y
    _compute__param_y: .word 0
    .global _compute__param_z
    _compute__param_z: .word 0
    _compute__local_0: .word 0
    _compute__local_1: .word 0
    _compute__local_2: .word 0
    _compute__local_3: .word 0
    _compute__local_8: .word 0
    proc _compute, W#@_p_x, W#@_p_y, W#@_p_z
    .sac
    .var _fp = 0
    .loc "test_sac_nested.c", 13
    .local @_l_diff = 8
    .local @_l_sum = 6
; .debug_var: __compute @_l_diff offset=8 size=2 type=int16 scope=local
; .debug_var: __compute @_l_sum offset=6 size=2 type=int16 scope=local
    .var @_p_x = 2
    .var @_p_y = 4
    .var @_p_z = 6
; .debug_var: __compute @_p_x offset=2 size=2 type=int16 scope=parameter
; .debug_var: __compute @_p_y offset=4 size=2 type=int16 scope=parameter
; .debug_var: __compute @_p_z offset=6 size=2 type=int16 scope=parameter

@entry:
    .loc "test_sac_nested.c", 15
    lda _compute__param_x
    ldx _compute__param_x+1
    sta $20
    stx $21
    lda _compute__param_y
    ldx _compute__param_y+1
    sta $20
    stx $21
    .loc "test_sac_nested.c", 6
    lda _compute__param_y
    ldx _compute__param_y+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _compute__param_x
    ldx _compute__param_x+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end0:
    .loc "test_sac_nested.c", 15
    lda $22
    ldx $23
    sta _compute__local_3
    stx _compute__local_3+1
    .loc "test_sac_nested.c", 16
    lda _compute__param_z
    ldx _compute__param_z+1
    sta $20
    stx $21
    lda _compute__local_3
    ldx _compute__local_3+1
    sta $20
    stx $21
    .loc "test_sac_nested.c", 10
    lda _compute__local_3
    ldx _compute__local_3+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _compute__param_z
    ldx _compute__param_z+1
    sub.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end2:
    .loc "test_sac_nested.c", 16
    lda $22
    ldx $23
    sta _compute__local_8
    stx _compute__local_8+1
    .loc "test_sac_nested.c", 17
    lda _compute__local_8
    ldx _compute__local_8+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 10
    endproc

; function _main
; SAC inline storage: 14 bytes
    _main__local_0: .word 0
    _main__local_20: .word 0
    _main__local_28: .word 0
    _main__local_33: .word 0
    _main__local_40: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_sac_nested.c", 20
    .local @_l_diff = 10
    .local @_l_r1 = 0
    .local @_l_r2 = 6
    .local @_l_r3 = 12
    .local @_l_sum = 8
; .debug_var: __main @_l_diff offset=10 size=2 type=int16 scope=local
; .debug_var: __main @_l_r1 offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_r2 offset=6 size=2 type=int16 scope=local
; .debug_var: __main @_l_r3 offset=12 size=2 type=int16 scope=local
; .debug_var: __main @_l_sum offset=8 size=2 type=int16 scope=local

@entry:
    .loc "test_sac_nested.c", 23
    lda #5
    ldx #0
    sta $20
    stx $21
    lda #10
    ldx #0
    sta $22
    stx $23
    lda #30
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
    .loc "test_sac_nested.c", 15
    lda $20
    ldx $21
    sta $24
    stx $25
    lda $22
    ldx $23
    sta $24
    stx $25
    .loc "test_sac_nested.c", 6
    lda $20
    clc
    adc #10
    sta $24
    lda $21
    adc #0
    sta $25
    lda $24
    ldx $25
    sta $20
    stx $21
@inline_end5:
    .loc "test_sac_nested.c", 15
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_sac_nested.c", 16
    lda $26
    ldx $27
    sta $20
    stx $21
    lda $22
    ldx $23
    sta $20
    stx $21
    .loc "test_sac_nested.c", 10
    lda $26
    sec
    sbc $22
    sta $20
    lda $27
    sbc $22+1
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end7:
    .loc "test_sac_nested.c", 16
    lda $22
    ldx $23
    sta $20
    stx $21
    .loc "test_sac_nested.c", 17
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end4:
    .loc "test_sac_nested.c", 23
    lda $22
    ldx $23
    sta _main__local_0
    stx _main__local_0+1
    .loc "test_sac_nested.c", 24
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, #15
    bne @if_then10
    bra @if_end12
@if_then10:
    lda #1
    ldx #0
    bra @__return
@if_end12:
    .loc "test_sac_nested.c", 28
    lda #2
    ldx #0
    sta $20
    stx $21
    lda #3
    ldx #0
    sta $22
    stx $23
    lda #20
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
    .loc "test_sac_nested.c", 15
    lda $20
    ldx $21
    sta $24
    stx $25
    lda $22
    ldx $23
    sta $24
    stx $25
    .loc "test_sac_nested.c", 6
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
@inline_end14:
    .loc "test_sac_nested.c", 15
    lda $20
    ldx $21
    sta _main__local_28
    stx _main__local_28+1
    .loc "test_sac_nested.c", 16
    lda $26
    ldx $27
    sta $20
    stx $21
    lda _main__local_28
    ldx _main__local_28+1
    sta $20
    stx $21
    .loc "test_sac_nested.c", 10
    lda _main__local_28
    ldx _main__local_28+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $26
    ldx $27
    sub.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end16:
    .loc "test_sac_nested.c", 16
    lda $22
    ldx $23
    sta _main__local_33
    stx _main__local_33+1
    .loc "test_sac_nested.c", 17
    lda _main__local_33
    ldx _main__local_33+1
    sta $20
    stx $21
@inline_end13:
    .loc "test_sac_nested.c", 28
    lda $20
    ldx $21
    sta _main__local_20
    stx _main__local_20+1
    .loc "test_sac_nested.c", 29
    lda _main__local_20
    ldx _main__local_20+1
    cmp.16 .AX, #15
    bne @if_then19
    bra @if_end21
@if_then19:
    lda #2
    ldx #0
    bra @__return
@if_end21:
    .loc "test_sac_nested.c", 32
    lda #100
    ldx #0
    sta $20
    stx $21
    lda #200
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
    .loc "test_sac_nested.c", 6
    lda $20
    clc
    adc #200
    sta $24
    lda $21
    adc #0
    sta $25
    lda $24
    ldx $25
    sta $20
    stx $21
@inline_end22:
    .loc "test_sac_nested.c", 32
    lda $20
    ldx $21
    sta _main__local_40
    stx _main__local_40+1
    .loc "test_sac_nested.c", 33
    lda _main__local_40
    ldx _main__local_40+1
    cmp.16 .AX, #300
    bne @if_then24
    bra @if_end26
@if_then24:
    lda #3
    ldx #0
    bra @__return
@if_end26:
    .loc "test_sac_nested.c", 36
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 14
    endproc


__zp_save_buf:

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

    .global _non_sac_add
    .global _sac_multiply
    .global _main

    .segment "code"

; function _non_sac_add
; SAC inline storage: 4 bytes
    .global _non_sac_add__param_a
    _non_sac_add__param_a: .word 0
    .global _non_sac_add__param_b
    _non_sac_add__param_b: .word 0
    _non_sac_add__local_0: .word 0
    _non_sac_add__local_1: .word 0
    proc _non_sac_add, W#@_p_a, W#@_p_b
    .sac
    .var _fp = 0
    .loc "test_sac_pragma.c", 6
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __non_sac_add @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __non_sac_add @_p_b offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_sac_pragma.c", 7
    lda _non_sac_add__param_b
    ldx _non_sac_add__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _non_sac_add__param_a
    ldx _non_sac_add__param_a+1
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

; function _sac_multiply
; SAC inline storage: 4 bytes
    .global _sac_multiply__param_a
    _sac_multiply__param_a: .word 0
    .global _sac_multiply__param_b
    _sac_multiply__param_b: .word 0
    _sac_multiply__local_0: .word 0
    _sac_multiply__local_1: .word 0
    proc _sac_multiply, W#@_p_a, W#@_p_b
    .sac
    .var _fp = 0
    .loc "test_sac_pragma.c", 11
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __sac_multiply @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __sac_multiply @_p_b offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_sac_pragma.c", 12
    lda _sac_multiply__param_b
    ldx _sac_multiply__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _sac_multiply__param_a
    ldx _sac_multiply__param_a+1
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

; function _main
; SAC inline storage: 6 bytes
    _main__local_0: .word 0
    _main__local_9: .word 0
    _main__local_18: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_sac_pragma.c", 15
    .local @_l_r1 = 0
    .local @_l_r2 = 2
    .local @_l_r3 = 4
; .debug_var: __main @_l_r1 offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_r2 offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_r3 offset=4 size=2 type=int16 scope=local

@entry:
    .loc "test_sac_pragma.c", 17
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
    .loc "test_sac_pragma.c", 7
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
@inline_end0:
    .loc "test_sac_pragma.c", 17
    lda $20
    ldx $21
    sta _main__local_0
    stx _main__local_0+1
    .loc "test_sac_pragma.c", 18
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, #30
    bne @if_then2
    bra @if_end4
@if_then2:
    lda #1
    ldx #0
    bra @__return
@if_end4:
    .loc "test_sac_pragma.c", 21
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
    .loc "test_sac_pragma.c", 12
    lda $20
    ldx $21
    mul.16 .AX, $22
    sta $24
    stx $25
    lda $24
    ldx $25
    sta $20
    stx $21
@inline_end5:
    .loc "test_sac_pragma.c", 21
    lda $20
    ldx $21
    sta _main__local_9
    stx _main__local_9+1
    .loc "test_sac_pragma.c", 22
    lda _main__local_9
    ldx _main__local_9+1
    cmp.16 .AX, #30
    bne @if_then7
    bra @if_end9
@if_then7:
    lda #2
    ldx #0
    bra @__return
@if_end9:
    .loc "test_sac_pragma.c", 25
    lda #70
    ldx #0
    sta $20
    stx $21
    lda _main__local_9
    ldx _main__local_9+1
    sta $22
    stx $23
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_sac_pragma.c", 7
    lda _main__local_9
    ldx _main__local_9+1
    add.16 .AX, $20
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end10:
    .loc "test_sac_pragma.c", 25
    lda $20
    ldx $21
    sta _main__local_18
    stx _main__local_18+1
    .loc "test_sac_pragma.c", 26
    lda _main__local_18
    ldx _main__local_18+1
    cmp.16 .AX, #100
    bne @if_then12
    bra @if_end14
@if_then12:
    lda #3
    ldx #0
    bra @__return
@if_end14:
    .loc "test_sac_pragma.c", 29
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc


__zp_save_buf:

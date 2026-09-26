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

    .global _lib_func
    .global _sac_wrapper
    .global _main

    .segment "code"

; function _lib_func
; SAC inline storage: 2 bytes
    .global _lib_func__param_a
    _lib_func__param_a: .word 0
    _lib_func__local_0: .word 0
    proc _lib_func, W#@_p_a
    .sac
    .var _fp = 0
    .loc "test_sac_compat.c", 6
    .var @_p_a = 2
; .debug_var: __lib_func @_p_a offset=2 size=2 type=int16 scope=parameter

@entry:
    .loc "test_sac_compat.c", 7
    lda #2
    ldx #0
    sta $20
    stx $21
    lda _lib_func__param_a
    ldx _lib_func__param_a+1
    lsl.16 .AX
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z
    .frame_size 2
    endproc

; function _sac_wrapper
; SAC inline storage: 2 bytes
    .global _sac_wrapper__param_x
    _sac_wrapper__param_x: .word 0
    _sac_wrapper__local_0: .word 0
    proc _sac_wrapper, W#@_p_x
    .sac
    .var _fp = 0
    .loc "test_sac_compat.c", 11
    .var @_p_x = 2
; .debug_var: __sac_wrapper @_p_x offset=2 size=2 type=int16 scope=parameter

@entry:
    .loc "test_sac_compat.c", 12
    lda _sac_wrapper__param_x
    ldx _sac_wrapper__param_x+1
    sta $20
    stx $21
    .loc "test_sac_compat.c", 7
    lda #2
    ldx #0
    sta $20
    stx $21
    lda _sac_wrapper__param_x
    ldx _sac_wrapper__param_x+1
    lsl.16 .AX
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end0:
    .loc "test_sac_compat.c", 12
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
    .frame_size 2
    endproc

; function _main
; SAC inline storage: 8 bytes
    _main__local_0: .word 0
    _main__local_8: .word 0
    _main__local_20: .word 0
    _main__local_32: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_sac_compat.c", 15
    .local @_l_r1 = 0
    .local @_l_r2 = 2
    .local @_l_r3 = 4
    .local @_l_r4 = 6
; .debug_var: __main @_l_r1 offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_r2 offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_r3 offset=4 size=2 type=int16 scope=local
; .debug_var: __main @_l_r4 offset=6 size=2 type=int16 scope=local

@entry:
    .loc "test_sac_compat.c", 17
    lda #5
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_sac_compat.c", 7
    lda #2
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    lsl.16 .AX
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end2:
    .loc "test_sac_compat.c", 17
    lda $20
    ldx $21
    sta _main__local_0
    stx _main__local_0+1
    .loc "test_sac_compat.c", 18
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, #10
    bne @if_then4
    bra @if_end6
@if_then4:
    lda #1
    ldx #0
    bra @__return
@if_end6:
    .loc "test_sac_compat.c", 21
    lda #5
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_sac_compat.c", 12
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_sac_compat.c", 7
    lda #2
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    lsl.16 .AX
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end8:
    .loc "test_sac_compat.c", 12
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
@inline_end7:
    .loc "test_sac_compat.c", 21
    lda $20
    ldx $21
    sta _main__local_8
    stx _main__local_8+1
    .loc "test_sac_compat.c", 22
    lda _main__local_8
    ldx _main__local_8+1
    cmp.16 .AX, #20
    bne @if_then11
    bra @if_end13
@if_then11:
    lda #2
    ldx #0
    bra @__return
@if_end13:
    .loc "test_sac_compat.c", 25
    lda #10
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_sac_compat.c", 12
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_sac_compat.c", 7
    lda #2
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    lsl.16 .AX
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end15:
    .loc "test_sac_compat.c", 12
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
@inline_end14:
    .loc "test_sac_compat.c", 25
    lda $20
    ldx $21
    sta _main__local_20
    stx _main__local_20+1
    .loc "test_sac_compat.c", 26
    lda _main__local_20
    ldx _main__local_20+1
    cmp.16 .AX, #30
    bne @if_then18
    bra @if_end20
@if_then18:
    lda #3
    ldx #0
    bra @__return
@if_end20:
    .loc "test_sac_compat.c", 29
    lda #100
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_sac_compat.c", 7
    lda #2
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    lsl.16 .AX
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end21:
    .loc "test_sac_compat.c", 29
    lda $20
    ldx $21
    sta _main__local_32
    stx _main__local_32+1
    .loc "test_sac_compat.c", 30
    lda _main__local_32
    ldx _main__local_32+1
    cmp.16 .AX, #200
    bne @if_then23
    bra @if_end25
@if_then23:
    lda #4
    ldx #0
    bra @__return
@if_end25:
    .loc "test_sac_compat.c", 33
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 8
    endproc


__zp_save_buf:

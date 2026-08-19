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

    .global _sub
    .global _main

    .segment "code"

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
    .loc "test_sub.c", 1
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __sub @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __sub @_p_b offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_sub.c", 2
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

; function _main
; SAC inline storage: 2 bytes
    _main__local_0: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_sub.c", 5
    .local @_l_res = 0
; .debug_var: __main @_l_res offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_sub.c", 6
    lda #10
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
    .loc "test_sub.c", 2
    lda $20
    sec
    sbc #3
    sta $24
    lda $21
    sbc #0
    sta $25
    lda $24
    ldx $25
    sta $20
    stx $21
@inline_end0:
    .loc "test_sub.c", 6
    lda $20
    ldx $21
    sta _main__local_0
    stx _main__local_0+1
    .loc "test_sub.c", 7
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, #7
    beq @if_then2
    bra @if_else3
@if_then2:
    .loc "test_sub.c", 8
    lda #0
    ldx #0
    bra @__return
    bra @if_end4
@if_else3:
    .loc "test_sub.c", 10
    lda #1
    ldx #0
    bra @__return
@if_end4:
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc


__zp_save_buf:

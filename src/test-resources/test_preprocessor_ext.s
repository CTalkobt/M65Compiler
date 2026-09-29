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

    .global _main

    .segment "code"

; function _main
; SAC inline storage: 4 bytes
    _main__local_0: .word 0
    _main__local_2: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_preprocessor_ext.c", 14
    .local @_l_line = 2
    .local @_l_success = 0
; .debug_var: __main @_l_line offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_success offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_preprocessor_ext.c", 15
    lda #0
    sta _main__local_0
    sta _main__local_0+1
    .loc "test_preprocessor_ext.c", 200
    lda #200
    sta _main__local_2
    lda #0
    sta _main__local_2+1
    .loc "test_preprocessor_ext.c", 201
    lda _main__local_2
    ldx _main__local_2+1
    cmp.16 .AX, #200
    beq @if_then0
    bra @if_end2
@if_then0:
    lda _main__local_0
    ldx _main__local_0+1
    sta $20
    stx $21
    lda $20
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta _main__local_0
    stx _main__local_0+1
@if_end2:
    .loc "test_preprocessor_ext.c", 204
    lda _main__local_0
    ldx _main__local_0+1
    sta $20
    stx $21
    lda $20
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta _main__local_0
    stx _main__local_0+1
    .loc "test_preprocessor_ext.c", 207
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, #2
    beq @if_then3
    bra @if_end5
@if_then3:
    lda #0
    ldx #0
    bra @__return
@if_end5:
    .loc "test_preprocessor_ext.c", 208
    lda #1
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc


__zp_save_buf:

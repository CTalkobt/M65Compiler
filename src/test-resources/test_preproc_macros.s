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
; SAC inline storage: 6 bytes
    _main__local_0: .word 0
    _main__local_6: .word 0
    _main__local_12: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_preproc_macros.c", 6
    .local @_l_myvar = 2
    .local @_l_s = 4
    .local @_l_success = 0
; .debug_var: __main @_l_myvar offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_s offset=4 size=2 type=ptr scope=local
; .debug_var: __main @_l_success offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_preproc_macros.c", 11
    lda #0
    sta _main__local_0
    sta _main__local_0+1
    .loc "test_preproc_macros.c", 15
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
    .loc "test_preproc_macros.c", 19
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
    .loc "test_preproc_macros.c", 22
    lda #5
    sta _main__local_6
    lda #0
    sta _main__local_6+1
    .loc "test_preproc_macros.c", 23
    lda _main__local_6
    ldx _main__local_6+1
    cmp.16 .AX, #5
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
    .loc "test_preproc_macros.c", 26
    .loc "test_preproc_macros.c", 28
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, #3
    beq @if_then4
    bra @if_end6
@if_then4:
    lda #0
    ldx #0
    bra @__return
@if_end6:
    .loc "test_preproc_macros.c", 29
    lda #1
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc


    .segment "data"
__str_3:
    .text "hello"
    .byte 0

__zp_save_buf:

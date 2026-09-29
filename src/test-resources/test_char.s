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
    _main__local_2: .word 0
    _main__local_4: .word 0
    _main__local_9: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_char.c", 1
    .local @_l_c = 0
    .local @_l_d = 4
    .local @_l_i = 2
; .debug_var: __main @_l_c offset=0 size=2 type=int8 scope=local
; .debug_var: __main @_l_d offset=4 size=2 type=int8 scope=local
; .debug_var: __main @_l_i offset=2 size=2 type=int16 scope=local

@entry:
    .loc "test_char.c", 2
    lda #10
    sta _main__local_0
    lda #0
    sta _main__local_0+1
    .loc "test_char.c", 3
    .loc "test_char.c", 5
    lda #5
    sta $20
    lda _main__local_0
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    clc
    adc $24
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_4
    .loc "test_char.c", 6
    lda #15
    sta $20
    lda _main__local_4
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    cmp.16 .AX, $24
    beq @if_then0
    bra @if_end2
@if_then0:
    .loc "test_char.c", 7
    lda #1
    ldx #0
    bra @__return
@if_end2:
    .loc "test_char.c", 9
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

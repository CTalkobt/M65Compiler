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
; SAC inline storage: 2 bytes
    _main__local_0: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_char_flow.c", 1
    .local @_l_c = 0
; .debug_var: __main @_l_c offset=0 size=2 type=int8 scope=local

@entry:
    .loc "test_char_flow.c", 2
    lda #1
    sta _main__local_0
    lda #0
    sta _main__local_0+1
    .loc "test_char_flow.c", 3
    lda _main__local_0
    bne @if_then0
    bra @while_cond3_ph
@if_then0:
    .loc "test_char_flow.c", 4
    lda #0
    sta $20
    sta _main__local_0
@while_cond3_ph:
    .loc "test_char_flow.c", 7
    lda #0
    sta $20
    lda $20
    ldx #0
    ldx #0
    sta $22
    stx $23
@while_cond3:
    lda #0
    sta $20
    lda _main__local_0
    ldx #0
    sta $24
    stx $25
    lda $24
    ldx $25
    cmp.16 .AX, $22
    beq @while_body4
    bra @while_end5
@while_body4:
    .loc "test_char_flow.c", 8
    lda #1
    sta $28
    sta _main__local_0
    bra @while_cond3
@while_end5:
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc


__zp_save_buf:

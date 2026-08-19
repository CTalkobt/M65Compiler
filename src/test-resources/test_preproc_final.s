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
; SAC inline storage: 8 bytes
    _main__local_0: .word 0
    _main__local_2: .word 0
    _main__local_4: .word 0
    _main__local_6: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_preproc_final.c", 3
    .local @_l_success_comments = 4
    .local @_l_success_elif = 2
    .local @_l_success_strings = 6
    .local @_l_success_v2 = 0
; .debug_var: __main @_l_success_comments offset=4 size=2 type=int16 scope=local
; .debug_var: __main @_l_success_elif offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_success_strings offset=6 size=2 type=int16 scope=local
; .debug_var: __main @_l_success_v2 offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_preproc_final.c", 4
    lda #0
    sta _main__local_0
    sta _main__local_0+1
    .loc "test_preproc_final.c", 5
    lda #0
    sta _main__local_2
    sta _main__local_2+1
    .loc "test_preproc_final.c", 6
    lda #0
    sta _main__local_4
    sta _main__local_4+1
    .loc "test_preproc_final.c", 7
    lda #0
    sta _main__local_6
    sta _main__local_6+1
    .loc "test_preproc_final.c", 11
    lda #1
    sta _main__local_0
    lda #0
    sta _main__local_0+1
    .loc "test_preproc_final.c", 18
    lda #1
    sta _main__local_2
    lda #0
    sta _main__local_2+1
    .loc "test_preproc_final.c", 26
    lda #1
    sta _main__local_4
    lda #0
    sta _main__local_4+1
    .loc "test_preproc_final.c", 36
    lda #1
    sta _main__local_6
    lda #0
    sta _main__local_6+1
    .loc "test_preproc_final.c", 38
    lda _main__local_0
    ldx _main__local_0+1
    bne @and_rhs5
    cmp #$00
    bne @and_rhs5
    bra @if_end2
@and_rhs5:
    lda _main__local_2
    ldx _main__local_2+1
    bne @and_rhs4
    cmp #$00
    bne @and_rhs4
    bra @if_end2
@and_rhs4:
    lda _main__local_4
    ldx _main__local_4+1
    bne @and_rhs3
    cmp #$00
    bne @and_rhs3
    bra @if_end2
@and_rhs3:
    lda _main__local_6
    ldx _main__local_6+1
    bne @if_then0
    cmp #$00
    bne @if_then0
    bra @if_end2
@if_then0:
    lda #0
    ldx #0
    bra @__return
@if_end2:
    .loc "test_preproc_final.c", 39
    lda #1
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers N, Z
    .frame_size 8
    endproc


__zp_save_buf:

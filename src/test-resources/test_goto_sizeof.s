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
; SAC inline storage: 39 bytes
    _main__local_0: .word 0
    _main__local_2: .word 0
    _main__local_4: .word 0
    _main__local_6: .word 0
    _main__local_7: .word 0
    _main__local_9: .word 0
    _main__local_11: .word 0
    _main__local_12: .word 0
    _main__local_14: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_goto_sizeof.c", 7
    .local @_l_arr = 19
    .local @_l_p = 14
    .local @_l_s_arr = 10
    .local @_l_s_char = 2
    .local @_l_s_expr = 8
    .local @_l_s_int = 0
    .local @_l_s_ptr = 4
    .local @_l_s_struct = 6
    .local @_l_x = 12
; .debug_var: __main @_l_arr offset=19 size=2 type=int16 scope=local
; .debug_var: __main @_l_p offset=14 size=2 type=int16 scope=local
; .debug_var: __main @_l_s_arr offset=10 size=2 type=int16 scope=local
; .debug_var: __main @_l_s_char offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_s_expr offset=8 size=2 type=int16 scope=local
; .debug_var: __main @_l_s_int offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_s_ptr offset=4 size=2 type=int16 scope=local
; .debug_var: __main @_l_s_struct offset=6 size=2 type=int16 scope=local
; .debug_var: __main @_l_x offset=12 size=2 type=int16 scope=local

@entry:
    .loc "test_goto_sizeof.c", 8
    lda #2
    sta _main__local_0
    lda #0
    sta _main__local_0+1
    .loc "test_goto_sizeof.c", 9
    lda #1
    sta _main__local_2
    lda #0
    sta _main__local_2+1
    .loc "test_goto_sizeof.c", 10
    lda #2
    sta _main__local_4
    lda #0
    sta _main__local_4+1
    .loc "test_goto_sizeof.c", 13
    lda #5
    sta _main__local_7
    lda #0
    sta _main__local_7+1
    .loc "test_goto_sizeof.c", 14
    lda #2
    sta _main__local_9
    lda #0
    sta _main__local_9+1
    .loc "test_goto_sizeof.c", 17
    lda #20
    sta _main__local_12
    lda #0
    sta _main__local_12+1
    .loc "test_goto_sizeof.c", 19
    lda #0
    sta _main__local_14
    sta _main__local_14+1
    .loc "test_goto_sizeof.c", 20
    bra @start
@bad:
    .loc "test_goto_sizeof.c", 23
    lda #1
    ldx #0
    bra @__return
@start:
    .loc "test_goto_sizeof.c", 26
    lda #10
    sta _main__local_14
    lda #0
    sta _main__local_14+1
    .loc "test_goto_sizeof.c", 27
    lda _main__local_14
    ldx _main__local_14+1
    cmp.16 .AX, #10
    beq @if_then0
    bra @bad
@if_then0:
    bra @bad
@next:
    .loc "test_goto_sizeof.c", 31
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, #2
    beq @and_rhs10
    bra @if_end5
@and_rhs10:
    lda _main__local_2
    ldx _main__local_2+1
    cmp.16 .AX, #1
    beq @and_rhs9
    bra @if_end5
@and_rhs9:
    lda _main__local_4
    ldx _main__local_4+1
    cmp.16 .AX, #2
    beq @and_rhs8
    bra @if_end5
@and_rhs8:
    lda _main__local_7
    ldx _main__local_7+1
    cmp.16 .AX, #6
    beq @and_rhs7
    bra @if_end5
@and_rhs7:
    lda _main__local_9
    ldx _main__local_9+1
    cmp.16 .AX, #6
    beq @and_rhs6
    bra @if_end5
@and_rhs6:
    lda _main__local_12
    ldx _main__local_12+1
    cmp.16 .AX, #20
    beq @if_then3
    bra @if_end5
@if_then3:
    .loc "test_goto_sizeof.c", 32
    lda #0
    ldx #0
    bra @__return
@if_end5:
    .loc "test_goto_sizeof.c", 35
    lda #2
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 39
    endproc


__zp_save_buf:

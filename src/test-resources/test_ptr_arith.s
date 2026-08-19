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

    .global _test_ptr
    .global _main

    .segment "code"

; function _test_ptr
; SAC inline storage: 4 bytes
    _test_ptr__local_0: .word 0
    _test_ptr__local_2: .word 0
    proc _test_ptr
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_ptr_arith.c", 1
    .local @_l_p_char = 0
    .local @_l_p_int = 2
; .debug_var: __test_ptr @_l_p_char offset=0 size=2 type=ptr scope=local
; .debug_var: __test_ptr @_l_p_int offset=2 size=2 type=ptr scope=local

@entry:
    .loc "test_ptr_arith.c", 2
    lda #232
    sta _test_ptr__local_0
    lda #3
    sta _test_ptr__local_0+1
    .loc "test_ptr_arith.c", 3
    lda #208
    sta _test_ptr__local_2
    lda #7
    sta _test_ptr__local_2+1
    .loc "test_ptr_arith.c", 6
    lda _test_ptr__local_0
    ldx _test_ptr__local_0+1
    add.16 .AX, #1
    sta $22
    stx $23
    sta _test_ptr__local_0
    stx _test_ptr__local_0+1
    .loc "test_ptr_arith.c", 7
    lda _test_ptr__local_0
    ldx _test_ptr__local_0+1
    add.16 .AX, #5
    sta $22
    stx $23
    sta _test_ptr__local_0
    stx _test_ptr__local_0+1
    .loc "test_ptr_arith.c", 10
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    mul.16 .AX, #2
    sta $22
    stx $23
    lda _test_ptr__local_2
    ldx _test_ptr__local_2+1
    add.16 .AX, $22
    sta $20
    stx $21
    sta _test_ptr__local_2
    stx _test_ptr__local_2+1
    .loc "test_ptr_arith.c", 11
    lda #10
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    mul.16 .AX, #2
    sta $22
    stx $23
    lda _test_ptr__local_2
    ldx _test_ptr__local_2+1
    add.16 .AX, $22
    sta $20
    stx $21
    sta _test_ptr__local_2
    stx _test_ptr__local_2+1
    .loc "test_ptr_arith.c", 13
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    mul.16 .AX, #2
    sta $22
    stx $23
    lda _test_ptr__local_2
    ldx _test_ptr__local_2+1
    sub.16 .AX, $22
    sta $20
    stx $21
    sta _test_ptr__local_2
    stx _test_ptr__local_2+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _main
; SAC inline storage: 4 bytes
    _main__local_0: .word 0
    _main__local_2: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_ptr_arith.c", 16
    .local @_l_p_char = 0
    .local @_l_p_int = 2
; .debug_var: __main @_l_p_char offset=0 size=2 type=ptr scope=local
; .debug_var: __main @_l_p_int offset=2 size=2 type=ptr scope=local

@entry:
    .loc "test_ptr_arith.c", 2
    lda #232
    sta _main__local_0
    lda #3
    sta _main__local_0+1
    .loc "test_ptr_arith.c", 3
    lda #208
    sta _main__local_2
    lda #7
    sta _main__local_2+1
    .loc "test_ptr_arith.c", 6
    lda _main__local_0
    ldx _main__local_0+1
    add.16 .AX, #1
    sta $22
    stx $23
    sta _main__local_0
    stx _main__local_0+1
    .loc "test_ptr_arith.c", 7
    lda _main__local_0
    ldx _main__local_0+1
    add.16 .AX, #5
    sta $22
    stx $23
    sta _main__local_0
    stx _main__local_0+1
    .loc "test_ptr_arith.c", 10
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    mul.16 .AX, #2
    sta $22
    stx $23
    lda _main__local_2
    ldx _main__local_2+1
    add.16 .AX, $22
    sta $20
    stx $21
    sta _main__local_2
    stx _main__local_2+1
    .loc "test_ptr_arith.c", 11
    lda #10
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    mul.16 .AX, #2
    sta $22
    stx $23
    lda _main__local_2
    ldx _main__local_2+1
    add.16 .AX, $22
    sta $20
    stx $21
    sta _main__local_2
    stx _main__local_2+1
    .loc "test_ptr_arith.c", 13
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    mul.16 .AX, #2
    sta $22
    stx $23
    lda _main__local_2
    ldx _main__local_2+1
    sub.16 .AX, $22
    sta $20
    stx $21
    sta _main__local_2
    stx _main__local_2+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc


__zp_save_buf:

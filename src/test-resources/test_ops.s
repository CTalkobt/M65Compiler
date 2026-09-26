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

    .global _test_ops
    .global _main

    .segment "code"

; function _test_ops
; SAC inline storage: 6 bytes
    _test_ops__local_0: .word 0
    _test_ops__local_2: .word 0
    _test_ops__local_4: .word 0
    proc _test_ops
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_ops.c", 1
    .local @_l_a = 0
    .local @_l_b = 2
    .local @_l_c = 4
; .debug_var: __test_ops @_l_a offset=0 size=2 type=int16 scope=local
; .debug_var: __test_ops @_l_b offset=2 size=2 type=int16 scope=local
; .debug_var: __test_ops @_l_c offset=4 size=2 type=int16 scope=local

@entry:
    .loc "test_ops.c", 2
    lda #10
    sta _test_ops__local_0
    lda #0
    sta _test_ops__local_0+1
    .loc "test_ops.c", 3
    lda #20
    sta _test_ops__local_2
    lda #0
    sta _test_ops__local_2+1
    .loc "test_ops.c", 4
    lda #0
    sta _test_ops__local_4
    sta _test_ops__local_4+1
    .loc "test_ops.c", 7
    lda _test_ops__local_2
    ldx _test_ops__local_2+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _test_ops__local_0
    ldx _test_ops__local_0+1
    and.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _test_ops__local_4
    stx _test_ops__local_4+1
    .loc "test_ops.c", 8
    lda _test_ops__local_2
    ldx _test_ops__local_2+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _test_ops__local_0
    ldx _test_ops__local_0+1
    ora.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _test_ops__local_4
    stx _test_ops__local_4+1
    .loc "test_ops.c", 9
    lda _test_ops__local_2
    ldx _test_ops__local_2+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _test_ops__local_0
    ldx _test_ops__local_0+1
    eor.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _test_ops__local_4
    stx _test_ops__local_4+1
    .loc "test_ops.c", 10
    lda _test_ops__local_0
    ldx _test_ops__local_0+1
    not.16 .AX
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _test_ops__local_4
    stx _test_ops__local_4+1
    .loc "test_ops.c", 13
    lda _test_ops__local_0
    ldx _test_ops__local_0+1
    lsl.16 .AX
    lsl.16 .AX
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _test_ops__local_4
    stx _test_ops__local_4+1
    .loc "test_ops.c", 14
    lda _test_ops__local_2
    ldx _test_ops__local_2+1
    lsr.16 .AX
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _test_ops__local_4
    stx _test_ops__local_4+1
    .loc "test_ops.c", 17
    lda _test_ops__local_0
    ldx _test_ops__local_0+1
    bne @and_rhs3
    cmp #$00
    bne @and_rhs3
    bra @if_end2
@and_rhs3:
    lda _test_ops__local_2
    ldx _test_ops__local_2+1
    bne @if_then0
    cmp #$00
    bne @if_then0
    bra @if_end2
@if_then0:
    .loc "test_ops.c", 18
    lda #1
    sta _test_ops__local_4
    lda #0
    sta _test_ops__local_4+1
@if_end2:
    .loc "test_ops.c", 21
    lda _test_ops__local_4
    ldx _test_ops__local_4+1
    bne @if_then4
    cmp #$00
    bne @if_then4
@or_rhs7:
    lda #0
    sta $20
    sta $21
    bra @if_end6
@if_then4:
    .loc "test_ops.c", 22
    lda #2
    sta _test_ops__local_4
    lda #0
    sta _test_ops__local_4+1
@if_end6:
    .loc "test_ops.c", 25
    lda _test_ops__local_4
    ldx _test_ops__local_4+1
    bne @if_end10
    cmp #$00
    bne @if_end10
@if_then8:
    .loc "test_ops.c", 26
    lda #3
    sta _test_ops__local_4
    lda #0
    sta _test_ops__local_4+1
@if_end10:
    .loc "test_ops.c", 30
    lda _test_ops__local_0
    ldx _test_ops__local_0+1
    neg.16 .AX
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _test_ops__local_4
    stx _test_ops__local_4+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z
    .frame_size 6
    endproc

; function _main
; SAC inline storage: 0 bytes
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_ops.c", 33

@entry:
    .loc "test_ops.c", 34
    jsr _test_ops
    .loc "test_ops.c", 35
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 0
    endproc


__zp_save_buf:

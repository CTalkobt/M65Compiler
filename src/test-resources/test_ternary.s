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
; SAC inline storage: 18 bytes
    _main__local_0: .word 0
    _main__local_2: .word 0
    _main__local_4: .word 0
    _main__local_6: .word 0
    _main__local_7: .word 0
    _main__local_9: .word 0
    _main__local_10: .word 0
    _main__local_13: .word 0
    _main__local_16: .word 0
    _main__local_20: .word 0
    _main__local_22: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_ternary.c", 2
    .local @_l_a = 0
    .local @_l_b = 2
    .local @_l_c = 8
    .local @_l_d = 10
    .local @_l_e = 12
    .local @_l_f = 14
    .local @_l_g = 16
    .local @_l_h = 4
    .local @_l_i = 6
; .debug_var: __main @_l_a offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_b offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_c offset=8 size=2 type=int16 scope=local
; .debug_var: __main @_l_d offset=10 size=2 type=int16 scope=local
; .debug_var: __main @_l_e offset=12 size=2 type=int16 scope=local
; .debug_var: __main @_l_f offset=14 size=2 type=int16 scope=local
; .debug_var: __main @_l_g offset=16 size=2 type=int16 scope=local
; .debug_var: __main @_l_h offset=4 size=2 type=int16 scope=local
; .debug_var: __main @_l_i offset=6 size=2 type=int16 scope=local

@entry:
    .loc "test_ternary.c", 3
    lda #15
    sta _main__local_0
    lda #0
    sta _main__local_0+1
    .loc "test_ternary.c", 4
    lda #20
    sta _main__local_2
    lda #0
    sta _main__local_2+1
    .loc "test_ternary.c", 7
    lda _main__local_2
    ldx _main__local_2+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, __zp_scratch2
    beq @tern_else1
    bcs @tern_then0
    bra @tern_else1
@tern_then0:
    bra @tern_end2
@tern_else1:
@tern_end2:
    lda _main__local_2
    ldx _main__local_2+1
    sta _main__local_4
    stx _main__local_4+1
    .loc "test_ternary.c", 8
    lda _main__local_2
    ldx _main__local_2+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, __zp_scratch2
    bcc @tern_then3
    bra @tern_else4
@tern_then3:
    bra @tern_end5
@tern_else4:
@tern_end5:
    lda _main__local_2
    ldx _main__local_2+1
    sta _main__local_7
    stx _main__local_7+1
    .loc "test_ternary.c", 11
    lda #7
    ldx #0
    sta $20
    stx $21
    lda _main__local_0
    ldx _main__local_0+1
    and.16 .AX, $20
    sta $22
    stx $23
    lda $22
    ldx $23
    sta _main__local_10
    stx _main__local_10+1
    .loc "test_ternary.c", 12
    lda #8
    ldx #0
    sta $20
    stx $21
    lda _main__local_2
    ldx _main__local_2+1
    ora.16 .AX, $20
    sta $22
    stx $23
    lda $22
    ldx $23
    sta _main__local_13
    stx _main__local_13+1
    .loc "test_ternary.c", 13
    lda _main__local_2
    ldx _main__local_2+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_0
    ldx _main__local_0+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda #35
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    eor.16 .AX, $22
    sta $24
    stx $25
    lda $24
    ldx $25
    sta _main__local_16
    stx _main__local_16+1
    .loc "test_ternary.c", 14
    lda #8
    sta _main__local_20
    lda #0
    sta _main__local_20+1
    .loc "test_ternary.c", 15
    lda #4
    sta _main__local_22
    lda #0
    sta _main__local_22+1
    .loc "test_ternary.c", 17
    lda _main__local_4
    ldx _main__local_4+1
    cmp.16 .AX, #20
    beq @and_rhs14
    bra @if_end8
@and_rhs14:
    lda _main__local_7
    ldx _main__local_7+1
    cmp.16 .AX, #15
    beq @and_rhs13
    bra @if_end8
@and_rhs13:
    lda _main__local_10
    ldx _main__local_10+1
    cmp.16 .AX, #7
    beq @and_rhs12
    bra @if_end8
@and_rhs12:
    lda _main__local_13
    ldx _main__local_13+1
    cmp.16 .AX, #28
    beq @and_rhs11
    bra @if_end8
@and_rhs11:
    lda _main__local_16
    ldx _main__local_16+1
    stx __zp_scratch
    ora __zp_scratch
    beq @and_rhs10
    bra @if_end8
@and_rhs10:
    lda _main__local_20
    ldx _main__local_20+1
    cmp.16 .AX, #8
    beq @and_rhs9
    bra @if_end8
@and_rhs9:
    lda _main__local_22
    ldx _main__local_22+1
    cmp.16 .AX, #4
    beq @if_then6
    bra @if_end8
@if_then6:
    .loc "test_ternary.c", 18
    lda #0
    ldx #0
    bra @__return
@if_end8:
    .loc "test_ternary.c", 20
    lda #1
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 18
    endproc


__zp_save_buf:

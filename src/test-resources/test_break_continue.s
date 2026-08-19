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
    _main__local_11: .word 0
    _main__local_12: .word 0
    _main__local_24: .word 0
    _main__local_26: .word 0
    _main__local_35: .word 0
    _main__local_36: .word 0
    _main__local_37: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_break_continue.c", 1
    .local @_l_i = 0
    .local @_l_j = 4
    .local @_l_k = 8
    .local @_l_nested_sum = 16
    .local @_l_sum = 2
    .local @_l_sum2 = 6
    .local @_l_sum3 = 10
    .local @_l_x = 12
    .local @_l_y = 14
; .debug_var: __main @_l_i offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_j offset=4 size=2 type=int16 scope=local
; .debug_var: __main @_l_k offset=8 size=2 type=int16 scope=local
; .debug_var: __main @_l_nested_sum offset=16 size=2 type=int16 scope=local
; .debug_var: __main @_l_sum offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_sum2 offset=6 size=2 type=int16 scope=local
; .debug_var: __main @_l_sum3 offset=10 size=2 type=int16 scope=local
; .debug_var: __main @_l_x offset=12 size=2 type=int16 scope=local
; .debug_var: __main @_l_y offset=14 size=2 type=int16 scope=local

@entry:
    .loc "test_break_continue.c", 2
    lda #0
    sta _main__local_0
    sta _main__local_0+1
    .loc "test_break_continue.c", 3
    lda #0
    sta _main__local_2
    sta _main__local_2+1
@while_cond0:
    .loc "test_break_continue.c", 6
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, #10
    bcc @while_body1
    bra @while_end2
@while_body1:
    .loc "test_break_continue.c", 7
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, #5
    beq @while_end2
@if_end5:
    .loc "test_break_continue.c", 8
    lda _main__local_0
    ldx _main__local_0+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_2
    ldx _main__local_2+1
    add.16 .AX, __zp_scratch2
    sta $28
    stx $29
    sta _main__local_2
    stx _main__local_2+1
    .loc "test_break_continue.c", 9
    inc.16f __vr0
    bra @while_cond0
@while_end2:
    .loc "test_break_continue.c", 15
    lda #0
    sta _main__local_12
    sta _main__local_12+1
    .loc "test_break_continue.c", 16
    lda #0
    sta _main__local_11
    sta _main__local_11+1
@for_cond6:
    lda _main__local_11
    ldx _main__local_11+1
    cmp.16 .AX, #10
    bcc @for_body7
    bra @for_end9
@for_body7:
    .loc "test_break_continue.c", 17
    lda #2
    ldx #0
    sta $24
    stx $25
    lda _main__local_11
    ldx _main__local_11+1
    and.16 .AX, #1
    sta $26
    stx $27
    lda $26
    ora $27
    beq @for_inc8
@if_end12:
    .loc "test_break_continue.c", 18
    lda _main__local_11
    ldx _main__local_11+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_12
    ldx _main__local_12+1
    add.16 .AX, __zp_scratch2
    sta $2C
    stx $2D
    sta _main__local_12
    stx _main__local_12+1
@for_inc8:
    .loc "test_break_continue.c", 16
    inc.16f __vr11
    bra @for_cond6
@for_end9:
    .loc "test_break_continue.c", 23
    lda #0
    sta _main__local_24
    sta _main__local_24+1
    .loc "test_break_continue.c", 24
    lda #0
    sta _main__local_26
    sta _main__local_26+1
@do_body13:
    .loc "test_break_continue.c", 26
    lda _main__local_24
    ldx _main__local_24+1
    cmp.16 .AX, #3
    beq @do_end15
@if_end18:
    .loc "test_break_continue.c", 27
    lda _main__local_24
    ldx _main__local_24+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_26
    ldx _main__local_26+1
    add.16 .AX, __zp_scratch2
    sta $24
    stx $25
    sta _main__local_26
    stx _main__local_26+1
    .loc "test_break_continue.c", 28
    inc.16f __vr24
@do_cond14:
    .loc "test_break_continue.c", 29
    lda _main__local_24
    ldx _main__local_24+1
    cmp.16 .AX, #10
    bcc @do_body13
@do_end15:
    .loc "test_break_continue.c", 35
    lda #0
    sta _main__local_37
    sta _main__local_37+1
    .loc "test_break_continue.c", 36
    lda #0
    sta _main__local_35
    sta _main__local_35+1
@for_cond19:
    lda _main__local_35
    ldx _main__local_35+1
    cmp.16 .AX, #3
    bcc @for_body20
    bra @for_end22
@for_body20:
    .loc "test_break_continue.c", 37
    lda #0
    sta _main__local_36
    sta _main__local_36+1
@for_cond23:
    lda _main__local_36
    ldx _main__local_36+1
    cmp.16 .AX, #10
    bcc @for_body24
    bra @for_end26
@for_body24:
    .loc "test_break_continue.c", 38
    lda _main__local_36
    ldx _main__local_36+1
    cmp.16 .AX, #2
    beq @for_end26
@if_end29:
    .loc "test_break_continue.c", 39
    inc.16f __vr37
@for_inc25:
    .loc "test_break_continue.c", 37
    inc.16f __vr36
    bra @for_cond23
@for_end26:
@for_inc21:
    .loc "test_break_continue.c", 36
    inc.16f __vr35
    bra @for_cond19
@for_end22:
    .loc "test_break_continue.c", 44
    lda _main__local_12
    ldx _main__local_12+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_2
    ldx _main__local_2+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda _main__local_26
    ldx _main__local_26+1
    add.16 .AX, $20
    sta $22
    stx $23
    lda _main__local_37
    ldx _main__local_37+1
    add.16 .AX, $22
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 18
    endproc


__zp_save_buf:

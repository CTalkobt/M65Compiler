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
; SAC inline storage: 30 bytes
    _main__local_0: .word 0
    _main__local_2: .word 0
    _main__local_4: .word 0
    _main__local_9: .word 0
    _main__local_14: .word 0
    _main__local_16: .word 0
    _main__local_18: .word 0
    _main__local_26: .word 0
    _main__local_28: .word 0
    _main__local_30: .word 0
    _main__local_38: .word 0
    _main__local_39: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_mixed_types.c", 4
    .local @_l_a = 8
    .local @_l_arr = 22
    .local @_l_b = 10
    .local @_l_big = 14
    .local @_l_c = 0
    .local @_l_c2 = 6
    .local @_l_i = 2
    .local @_l_idx = 20
    .local @_l_r1 = 4
    .local @_l_small = 16
    .local @_l_sum = 12
    .local @_l_wrapped = 18
; .debug_var: __main @_l_a offset=8 size=2 type=int8 scope=local
; .debug_var: __main @_l_arr offset=22 size=2 type=int16 scope=local
; .debug_var: __main @_l_b offset=10 size=2 type=int8 scope=local
; .debug_var: __main @_l_big offset=14 size=2 type=int8 scope=local
; .debug_var: __main @_l_c offset=0 size=2 type=int8 scope=local
; .debug_var: __main @_l_c2 offset=6 size=2 type=int8 scope=local
; .debug_var: __main @_l_i offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_idx offset=20 size=2 type=int8 scope=local
; .debug_var: __main @_l_r1 offset=4 size=2 type=int16 scope=local
; .debug_var: __main @_l_small offset=16 size=2 type=int8 scope=local
; .debug_var: __main @_l_sum offset=12 size=2 type=int8 scope=local
; .debug_var: __main @_l_wrapped offset=18 size=2 type=int8 scope=local

@entry:
    .loc "test_mixed_types.c", 5
    lda #200
    sta _main__local_0
    lda #0
    sta _main__local_0+1
    .loc "test_mixed_types.c", 6
    lda #232
    sta _main__local_2
    lda #3
    sta _main__local_2+1
    lda _main__local_0
    ldx #0
    sta $20
    stx $21
    .loc "test_mixed_types.c", 9
    lda _main__local_2
    ldx _main__local_2+1
    add.16 .AX, $20
    sta $22
    stx $23
    sta _main__local_4
    stx _main__local_4+1
    .loc "test_mixed_types.c", 10
    lda _main__local_4
    ldx _main__local_4+1
    cmp.16 .AX, #1200
    bne @if_then0
    bra @if_end2
@if_then0:
    lda #1
    ldx #0
    bra @__return
@if_end2:
    .loc "test_mixed_types.c", 13
    lda _main__local_2
    ldx _main__local_2+1
    sta _main__local_9
    .loc "test_mixed_types.c", 14
    lda #232
    sta $20
    lda _main__local_9
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
    bne @if_then3
    bra @if_end5
@if_then3:
    lda #2
    ldx #0
    bra @__return
@if_end5:
    .loc "test_mixed_types.c", 17
    lda #100
    sta _main__local_14
    lda #0
    sta _main__local_14+1
    .loc "test_mixed_types.c", 18
    lda #50
    sta _main__local_16
    lda #0
    sta _main__local_16+1
    lda _main__local_14
    ldx #0
    sta $20
    stx $21
    lda _main__local_16
    ldx #0
    sta $22
    stx $23
    .loc "test_mixed_types.c", 19
    lda $20
    clc
    adc $22
    sta $24
    lda $21
    adc $22+1
    sta $25
    lda $24
    ldx $25
    sta _main__local_18
    .loc "test_mixed_types.c", 20
    lda #150
    sta $20
    lda _main__local_18
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
    bne @if_then6
    bra @if_end8
@if_then6:
    lda #3
    ldx #0
    bra @__return
@if_end8:
    .loc "test_mixed_types.c", 23
    lda #250
    sta _main__local_26
    lda #0
    sta _main__local_26+1
    .loc "test_mixed_types.c", 24
    lda #10
    sta _main__local_28
    lda #0
    sta _main__local_28+1
    lda _main__local_26
    ldx #0
    sta $20
    stx $21
    lda _main__local_28
    ldx #0
    sta $22
    stx $23
    .loc "test_mixed_types.c", 25
    lda $20
    clc
    adc $22
    sta $24
    lda $21
    adc $22+1
    sta $25
    lda $24
    ldx $25
    sta _main__local_30
    .loc "test_mixed_types.c", 26
    lda #4
    sta $20
    lda _main__local_30
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
    bne @if_then9
    bra @if_end11
@if_then9:
    lda #4
    ldx #0
    bra @__return
@if_end11:
    .loc "test_mixed_types.c", 30
    lda #2
    sta _main__local_39
    lda #0
    sta _main__local_39+1
    .loc "test_mixed_types.c", 31
    lda #42
    ldx #0
    sta $20
    stx $21
    leax.local 22
    sta $22
    stx $23
    lda $20
    ldx $21
    pha
    phx
    lda _main__local_39
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $22
    ldx $22+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    .loc "test_mixed_types.c", 32
    leax.local 22
    sta $20
    stx $21
    lda #2
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $26
    stx $27
    lda $26
    ldx $27
    cmp.16 .AX, #42
    bne @if_then12
    bra @if_end14
@if_then12:
    lda #5
    ldx #0
    bra @__return
@if_end14:
    .loc "test_mixed_types.c", 34
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 30
    endproc


__zp_save_buf:

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
; SAC inline storage: 16 bytes
    _main__local_0: .word 0
    _main__local_1: .word 0
    _main__local_14: .word 0
    _main__local_21: .word 0
    _main__local_23: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_dead_store.c", 5
    .local @_l_a = 4
    .local @_l_b = 6
    .local @_l_results = 8
    .local @_l_sum = 0
    .local @_l_x = 2
; .debug_var: __main @_l_a offset=4 size=2 type=int16 scope=local
; .debug_var: __main @_l_b offset=6 size=2 type=int16 scope=local
; .debug_var: __main @_l_results offset=8 size=2 type=int16 scope=local
; .debug_var: __main @_l_sum offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_x offset=2 size=2 type=int16 scope=local

@entry:
    .loc "test_dead_store.c", 9
    lda #0
    sta _main__local_1
    sta _main__local_1+1
    .loc "test_dead_store.c", 10
    inc.16f __vr1
    .loc "test_dead_store.c", 11
    lda _main__local_1
    ldx _main__local_1+1
    add.16 .AX, #2
    sta $22
    stx $23
    sta _main__local_1
    stx _main__local_1+1
    .loc "test_dead_store.c", 12
    lda _main__local_1
    ldx _main__local_1+1
    add.16 .AX, #3
    sta $22
    stx $23
    sta _main__local_1
    stx _main__local_1+1
    .loc "test_dead_store.c", 13
    lda _main__local_1
    ldx _main__local_1+1
    add.16 .AX, #4
    sta $22
    stx $23
    sta _main__local_1
    stx _main__local_1+1
    .loc "test_dead_store.c", 14
    leax.local 8
    sta $20
    stx $21
    lda #0
    sta $22
    sta $23
    lda _main__local_1
    ldx _main__local_1+1
    pha
    phx
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
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    .loc "test_dead_store.c", 17
    lda #5
    sta _main__local_14
    lda #0
    sta _main__local_14+1
    .loc "test_dead_store.c", 18
    lda #2
    ldx #0
    sta $20
    stx $21
    lda _main__local_14
    ldx _main__local_14+1
    lsl.16 .AX
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_14
    stx _main__local_14+1
    .loc "test_dead_store.c", 19
    leax.local 8
    sta $20
    stx $21
    lda #1
    ldx #0
    sta $22
    stx $23
    lda _main__local_14
    ldx _main__local_14+1
    pha
    phx
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
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    .loc "test_dead_store.c", 22
    lda #1
    sta _main__local_21
    lda #0
    sta _main__local_21+1
    .loc "test_dead_store.c", 23
    lda #2
    sta _main__local_23
    lda #0
    sta _main__local_23+1
    .loc "test_dead_store.c", 24
    lda _main__local_23
    ldx _main__local_23+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_21
    ldx _main__local_21+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    sta _main__local_21
    stx _main__local_21+1
    .loc "test_dead_store.c", 25
    lda _main__local_21
    ldx _main__local_21+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_23
    ldx _main__local_23+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    sta _main__local_23
    stx _main__local_23+1
    .loc "test_dead_store.c", 26
    leax.local 8
    sta $20
    stx $21
    lda #2
    ldx #0
    sta $22
    stx $23
    lda _main__local_21
    ldx _main__local_21+1
    pha
    phx
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
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    .loc "test_dead_store.c", 27
    leax.local 8
    sta $20
    stx $21
    lda #3
    ldx #0
    sta $22
    stx $23
    lda _main__local_23
    ldx _main__local_23+1
    pha
    phx
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
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    .loc "test_dead_store.c", 29
    leax.local 8
    sta $20
    stx $21
    lda #0
    sta $22
    sta $23
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
    cmp.16 .AX, #10
    bne @if_then0
    bra @if_end2
@if_then0:
    lda #1
    ldx #0
    bra @__return
@if_end2:
    .loc "test_dead_store.c", 30
    leax.local 8
    sta $20
    stx $21
    lda #1
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
    cmp.16 .AX, #10
    bne @if_then3
    bra @if_end5
@if_then3:
    lda #2
    ldx #0
    bra @__return
@if_end5:
    .loc "test_dead_store.c", 31
    leax.local 8
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
    cmp.16 .AX, #3
    bne @if_then6
    bra @if_end8
@if_then6:
    lda #3
    ldx #0
    bra @__return
@if_end8:
    .loc "test_dead_store.c", 32
    leax.local 8
    sta $20
    stx $21
    lda #3
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
    cmp.16 .AX, #5
    bne @if_then9
    bra @if_end11
@if_then9:
    lda #4
    ldx #0
    bra @__return
@if_end11:
    .loc "test_dead_store.c", 33
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 16
    endproc


__zp_save_buf:

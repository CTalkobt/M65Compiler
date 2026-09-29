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

    .global _add_chars
    .global _main

    .segment "code"

; function _add_chars
; SAC inline storage: 4 bytes
    .global _add_chars__param_a
    _add_chars__param_a: .word 0
    .global _add_chars__param_b
    _add_chars__param_b: .word 0
    _add_chars__local_0: .word 0
    _add_chars__local_1: .word 0
    _add_chars__local_5: .word 0
    proc _add_chars, B#@_p_a, B#@_p_b
    .sac
    .var _fp = 0
    .loc "test_char_local.c", 4
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __add_chars @_p_a offset=2 size=2 type=int8 scope=parameter
; .debug_var: __add_chars @_p_b offset=4 size=2 type=int8 scope=parameter

@entry:
    lda _add_chars__param_a
    ldx #0
    sta $20
    stx $21
    lda _add_chars__param_b
    ldx #0
    sta $22
    stx $23
    .loc "test_char_local.c", 5
    lda $20
    ldx $21
    clc
    adc $22
    sta $24
    stx $25
    lda $24
    ldx $25
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _main
; SAC inline storage: 14 bytes
    _main__local_0: .word 0
    _main__local_2: .word 0
    _main__local_4: .word 0
    _main__local_12: .word 0
    _main__local_20: .word 0
    _main__local_21: .word 0
    _main__local_22: .word 0
    _main__local_23: .word 0
    _main__local_31: .word 0
    _main__local_49: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_char_local.c", 8
    .local @_l_arr = 10
    .local @_l_r = 8
    .local @_l_w = 6
    .local @_l_x = 0
    .local @_l_y = 2
    .local @_l_z = 4
; .debug_var: __main @_l_arr offset=10 size=2 type=int8 scope=local
; .debug_var: __main @_l_r offset=8 size=2 type=int8 scope=local
; .debug_var: __main @_l_w offset=6 size=2 type=int8 scope=local
; .debug_var: __main @_l_x offset=0 size=2 type=int8 scope=local
; .debug_var: __main @_l_y offset=2 size=2 type=int8 scope=local
; .debug_var: __main @_l_z offset=4 size=2 type=int8 scope=local

@entry:
    .loc "test_char_local.c", 9
    lda #10
    sta _main__local_0
    lda #0
    sta _main__local_0+1
    .loc "test_char_local.c", 10
    lda #20
    sta _main__local_2
    lda #0
    sta _main__local_2+1
    lda _main__local_0
    ldx #0
    sta $20
    stx $21
    lda _main__local_2
    ldx #0
    sta $22
    stx $23
    .loc "test_char_local.c", 11
    lda $20
    clc
    adc $22
    sta $24
    lda $21
    adc $22+1
    sta $25
    lda $24
    ldx $25
    sta _main__local_4
    .loc "test_char_local.c", 13
    lda #30
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
    bne @if_then0
    bra @if_end2
@if_then0:
    lda #1
    ldx #0
    bra @__return
@if_end2:
    lda _main__local_4
    ldx #0
    sta $20
    stx $21
    lda _main__local_0
    ldx #0
    sta $22
    stx $23
    .loc "test_char_local.c", 16
    lda $20
    sec
    sbc $22
    sta $24
    lda $21
    sbc $22+1
    sta $25
    lda $24
    ldx $25
    sta _main__local_12
    .loc "test_char_local.c", 17
    lda #20
    sta $20
    lda _main__local_12
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
    .loc "test_char_local.c", 20
    lda _main__local_0
    ldx #0
    sta $20
    stx $21
    lda _main__local_2
    ldx #0
    sta $22
    stx $23
    .loc "test_char_local.c", 5
    lda $20
    clc
    adc $22
    sta $24
    lda $21
    adc $22+1
    sta $25
@inline_end6:
    .loc "test_char_local.c", 20
    lda $24
    ldx #0
    sta _main__local_20
    .loc "test_char_local.c", 21
    lda #30
    sta $20
    lda _main__local_20
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
    bne @if_then8
    bra @if_end10
@if_then8:
    lda #3
    ldx #0
    bra @__return
@if_end10:
    .loc "test_char_local.c", 25
    leax.local 10
    sta $20
    stx $21
    lda #0
    sta $22
    sta $23
    lda _main__local_0
    pha
    lda $22
    ldx $23
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
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "test_char_local.c", 26
    leax.local 10
    sta $20
    stx $21
    lda #1
    ldx #0
    sta $22
    stx $23
    lda _main__local_2
    pha
    lda $22
    ldx $23
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
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "test_char_local.c", 27
    leax.local 10
    sta $20
    stx $21
    lda #0
    sta $22
    sta $23
    lda $22
    ldx $23
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
    ldx #0
    sta $26
    leax.local 10
    sta $20
    stx $21
    lda #1
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
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
    ldx #0
    sta $28
    lda $26
    ldx #0
    ldx #0
    sta $20
    stx $21
    lda $28
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    clc
    adc $22
    sta $24
    stx $25
    leax.local 10
    sta $20
    stx $21
    lda #2
    ldx #0
    sta $22
    stx $23
    lda $24
    ldx $25
    pha
    lda $22
    ldx $23
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
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "test_char_local.c", 28
    lda #0
    sta $20
    leax.local 10
    sta $22
    stx $23
    lda #3
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx #0
    pha
    lda $24
    ldx $25
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
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "test_char_local.c", 30
    leax.local 10
    sta $20
    stx $21
    lda #2
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
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
    ldx #0
    sta $26
    lda #30
    sta $20
    lda $26
    ldx #0
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
    bne @if_then11
    bra @if_end13
@if_then11:
    lda #4
    ldx #0
    bra @__return
@if_end13:
    .loc "test_char_local.c", 32
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 14
    endproc


__zp_save_buf:

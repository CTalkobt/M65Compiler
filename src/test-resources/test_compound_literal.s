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

    .global _res
    .global _sum_point
    .global _main

    .segment "data"
    .byte 0
_res:
; .debug_var: @global _res offset=0 size=2 type=ptr scope=global
    .word 16384

    .segment "code"

; function _sum_point
; SAC inline storage: 2 bytes
    .global _sum_point__param_p
    _sum_point__param_p: .word 0
    _sum_point__local_0: .word 0
    _sum_point__local_1: .word 0
    proc _sum_point, W#@_p_p
    .sac
    .var _fp = 0
    .loc "test_compound_literal.c", 7
    .var @_p_p = 2
; .debug_var: __sum_point @_p_p offset=2 size=2 type=ptr scope=parameter

@entry:
    .loc "test_compound_literal.c", 8
    lda _sum_point__param_p
    ldx _sum_point__param_p+1
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $20
    stx $21
    lda _sum_point__param_p
    ldx _sum_point__param_p+1
    add.16 .AX, #2
    sta $22
    stx $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $24
    stx $25
    lda $20
    clc
    adc $24
    sta $22
    lda $21
    adc $24+1
    sta $23
    lda $22
    ldx $23
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _main
; SAC inline storage: 26 bytes
    _main__local_0: .long 0
    _main__local_3: .word 0
    _main__local_7: .word 0
    _main__local_13: .word 0
    _main__local_17: .word 0
    _main__local_23: .word 0
    _main__local_28: .word 0
    _main__local_29: .word 0
    _main__local_32: .word 0
    _main__local_35: .word 0
    _main__local_36: .word 0
    _main__local_37: .word 0
    _main__local_39: .word 0
    _main__local_48: .word 0
    _main__local_52: .word 0
    _main__local_54: .word 0
    _main__local_56: .long 0
    _main__local_58: .word 0
    _main__local_61: .word 0
    _main__local_67: .word 0
    _main__local_71: .long 0
    _main__local_74: .word 0
    _main__local_78: .word 0
    _main__local_84: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_compound_literal.c", 13
    .local @_l_a = 6
    .local @_l_b = 8
    .local @_l_c = 2
    .local @_l_p = 10
    .local @_l_p2 = 14
    .local @_l_p3 = 18
    .local @_l_r = 4
    .local @_l_v = 0
; .debug_var: __main @_l_a offset=6 size=2 type=int16 scope=local
; .debug_var: __main @_l_b offset=8 size=2 type=int16 scope=local
; .debug_var: __main @_l_c offset=2 size=2 type=int8 scope=local
; .debug_var: __main @_l_p offset=10 size=4 type=int32 scope=local
; .debug_var: __main @_l_p2 offset=14 size=4 type=int32 scope=local
; .debug_var: __main @_l_p3 offset=18 size=4 type=int32 scope=local
; .debug_var: __main @_l_r offset=4 size=2 type=int16 scope=local
; .debug_var: __main @_l_v offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_compound_literal.c", 15
    leax.local 10
    sta $20
    stx $21
    lda #10
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    ldy #0
    sta ($20),y
    txa
    iny
    sta ($20),y
    lda #20
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    add.16 .AX, #2
    sta $24
    stx $25
    lda $22
    ldx $23
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_compound_literal.c", 16
    leax.local 10
    sta $20
    stx $21
    ldy #0
    lda ($20),y
    pha
    iny
    lda ($20),y
    tax
    pla
    sta $22
    stx $23
    leax.local 10
    sta $20
    stx $21
    lda $20
    ldx $21
    add.16 .AX, #2
    sta $24
    stx $25
    ldy #0
    lda ($24),y
    pha
    iny
    lda ($24),y
    tax
    pla
    sta $20
    stx $21
    lda $22
    ldx $23
    clc
    adc $20
    sta $24
    stx $25
    lda _res
    ldx _res+1
    sta $20
    stx $21
    lda #0
    sta $22
    sta $23
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
    .loc "test_compound_literal.c", 19
    lda #42
    sta _main__local_17
    lda #0
    sta _main__local_17+1
    .loc "test_compound_literal.c", 20
    lda _main__local_17
    ldx _main__local_17+1
    sta $20
    lda _res
    ldx _res+1
    sta $22
    stx $23
    lda #1
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
    .loc "test_compound_literal.c", 23
    lda #7
    sta _main__local_23
    lda #0
    sta _main__local_23+1
    .loc "test_compound_literal.c", 24
    lda _res
    ldx _res+1
    sta $20
    stx $21
    lda #2
    ldx #0
    sta $22
    stx $23
    lda _main__local_23
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
    .loc "test_compound_literal.c", 27
    leax.local 22
    sta $20
    stx $21
    lda #100
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    ldy #0
    sta ($20),y
    txa
    iny
    sta ($20),y
    lda #200
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    add.16 .AX, #2
    sta $24
    stx $25
    lda $22
    ldx $23
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_compound_literal.c", 8
    ldy #0
    lda ($20),y
    pha
    iny
    lda ($20),y
    tax
    pla
    sta $22
    stx $23
    ldy #0
    lda ($24),y
    pha
    iny
    lda ($24),y
    tax
    pla
    sta $20
    stx $21
    lda $22
    clc
    adc $20
    sta $24
    lda $23
    adc $20+1
    sta $25
    lda $24
    ldx $25
    sta $20
    stx $21
@inline_end0:
    .loc "test_compound_literal.c", 27
    lda $20
    ldx $21
    sta _main__local_28
    stx _main__local_28+1
    .loc "test_compound_literal.c", 28
    lda _main__local_28
    ldx _main__local_28+1
    sta $20
    lda _res
    ldx _res+1
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
    .loc "test_compound_literal.c", 29
    lda _main__local_28
    ldx _main__local_28+1
    txa
    ldx #0
    sta $20
    stx $21
    lda _res
    ldx _res+1
    sta $22
    stx $23
    lda #4
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx $21
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
    .loc "test_compound_literal.c", 32
    lda #5
    sta _main__local_52
    lda #0
    sta _main__local_52+1
    .loc "test_compound_literal.c", 33
    lda #15
    sta _main__local_54
    lda #0
    sta _main__local_54+1
    .loc "test_compound_literal.c", 34
    leax.local 14
    sta $20
    stx $21
    lda _main__local_52
    ldx _main__local_52+1
    ldy #0
    sta ($20),y
    txa
    iny
    sta ($20),y
    lda $20
    ldx $21
    add.16 .AX, #2
    sta $22
    stx $23
    lda _main__local_54
    ldx _main__local_54+1
    ldy #0
    sta ($22),y
    txa
    iny
    sta ($22),y
    .loc "test_compound_literal.c", 35
    leax.local 14
    sta $20
    stx $21
    ldy #0
    lda ($20),y
    pha
    iny
    lda ($20),y
    tax
    pla
    sta $22
    stx $23
    leax.local 14
    sta $20
    stx $21
    lda $20
    ldx $21
    add.16 .AX, #2
    sta $24
    stx $25
    ldy #0
    lda ($24),y
    pha
    iny
    lda ($24),y
    tax
    pla
    sta $20
    stx $21
    lda $22
    ldx $23
    clc
    adc $20
    sta $24
    stx $25
    lda _res
    ldx _res+1
    sta $20
    stx $21
    lda #5
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
    .loc "test_compound_literal.c", 38
    leax.local 18
    sta $20
    stx $21
    lda #0
    sta $22
    sta $23
    lda $22
    ldx $23
    ldy #0
    sta ($20),y
    txa
    iny
    sta ($20),y
    lda #0
    sta $22
    sta $23
    lda $20
    ldx $21
    add.16 .AX, #2
    sta $24
    stx $25
    lda $22
    ldx $23
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_compound_literal.c", 39
    leax.local 18
    sta $20
    stx $21
    ldy #0
    lda ($20),y
    pha
    iny
    lda ($20),y
    tax
    pla
    sta $22
    stx $23
    leax.local 18
    sta $20
    stx $21
    lda $20
    ldx $21
    add.16 .AX, #2
    sta $24
    stx $25
    ldy #0
    lda ($24),y
    pha
    iny
    lda ($24),y
    tax
    pla
    sta $20
    stx $21
    lda $22
    ldx $23
    clc
    adc $20
    sta $24
    stx $25
    lda _res
    ldx _res+1
    sta $20
    stx $21
    lda #6
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
    .loc "test_compound_literal.c", 41
    brk
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 26
    endproc


__zp_save_buf:

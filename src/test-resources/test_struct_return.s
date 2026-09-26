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

    .global _results
    .global _make_point
    .global _sum_point
    .global _main

    .segment "data"
    .byte 0
_results:
; .debug_var: @global _results offset=0 size=2 type=ptr scope=global
    .word 16384

    .segment "code"

; function _make_point

    ; Static buffer for struct return from _make_point
    _make_point__struct_buf:
    .byte 0, 0, 0, 0

; SAC inline storage: 8 bytes
    .global _make_point__param_x
    _make_point__param_x: .word 0
    .global _make_point__param_y
    _make_point__param_y: .word 0
    _make_point__local_0: .word 0
    _make_point__local_1: .word 0
    _make_point__local_2: .long 0
    _make_point__local_4: .word 0
    proc _make_point, W#@_p_x, W#@_p_y
    .sac
    .var _fp = 0
    .loc "test_struct_return.c", 12
    .local @_l_p = 4
; .debug_var: __make_point @_l_p offset=4 size=4 type=int32 scope=local
    .var @_p_x = 2
    .var @_p_y = 4
; .debug_var: __make_point @_p_x offset=2 size=2 type=int16 scope=parameter
; .debug_var: __make_point @_p_y offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_struct_return.c", 14
    leax.local 4
    sta $20
    stx $21
    lda _make_point__param_x
    ldx _make_point__param_x+1
    ldy #0
    sta ($20),y
    txa
    iny
    sta ($20),y
    .loc "test_struct_return.c", 15
    leax.local 4
    sta $20
    stx $21
    lda $20
    ldx $21
    add.16 .AX, #2
    sta $22
    stx $23
    lda _make_point__param_y
    ldx _make_point__param_y+1
    ldy #0
    sta ($22),y
    txa
    iny
    sta ($22),y
    .loc "test_struct_return.c", 16
    lda _make_point__local_2
    ldx _make_point__local_2+1
    ldy _make_point__local_2+2
    ldz _make_point__local_2+3
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 8
    endproc

; function _sum_point
; SAC inline storage: 2 bytes
    .global _sum_point__param_p
    _sum_point__param_p: .word 0
    _sum_point__local_0: .word 0
    _sum_point__local_1: .word 0
    proc _sum_point, W#@_p_p
    .sac
    .var _fp = 0
    .loc "test_struct_return.c", 19
    .var @_p_p = 2
; .debug_var: __sum_point @_p_p offset=2 size=2 type=ptr scope=parameter

@entry:
    .loc "test_struct_return.c", 20
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
; SAC inline storage: 28 bytes
    _main__local_0: .long 0
    _main__local_1: .word 0
    _main__local_4: .word 0
    _main__local_5: .word 0
    _main__local_6: .word 0
    _main__local_7: .long 0
    _main__local_9: .word 0
    _main__local_14: .word 0
    _main__local_27: .long 0
    _main__local_28: .word 0
    _main__local_31: .word 0
    _main__local_32: .word 0
    _main__local_33: .word 0
    _main__local_34: .long 0
    _main__local_36: .word 0
    _main__local_41: .word 0
    _main__local_54: .long 0
    _main__local_55: .word 0
    _main__local_58: .word 0
    _main__local_59: .word 0
    _main__local_60: .word 0
    _main__local_61: .long 0
    _main__local_63: .word 0
    _main__local_68: .word 0
    _main__local_69: .word 0
    _main__local_70: .word 0
    _main__local_81: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_struct_return.c", 23
    .local @_l_p = 20
    .local @_l_p1 = 0
    .local @_l_p2 = 8
    .local @_l_p3 = 16
; .debug_var: __main @_l_p offset=20 size=4 type=int32 scope=local
; .debug_var: __main @_l_p1 offset=0 size=4 type=int32 scope=local
; .debug_var: __main @_l_p2 offset=8 size=4 type=int32 scope=local
; .debug_var: __main @_l_p3 offset=16 size=4 type=int32 scope=local

@entry:
    .loc "test_struct_return.c", 25
    lda #1
    ldx #0
    sta $20
    stx $21
    lda #2
    ldx #0
    sta $22
    stx $23
    .loc "test_struct_return.c", 14
    leax.local 24
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_struct_return.c", 15
    leax.local 24
    sta $20
    stx $21
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
    lda _main__local_7
    ldx _main__local_7+1
    ldy _main__local_7+2
    ldz _main__local_7+3
    sta $20
    stx $21
    sty $22
    stz $23
    .loc "test_struct_return.c", 16
@inline_end0:
    .loc "test_struct_return.c", 25
    lda $20
    ldx $21
    ldy $22
    ldz $23
    sta _main__local_0
    stx _main__local_0+1
    sty _main__local_0+2
    stz _main__local_0+3
    .loc "test_struct_return.c", 26
    leax.local 0
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
    lda $22
    ldx $23
    sta $20
    lda _results
    ldx _results+1
    sta $22
    stx $23
    lda #0
    sta $24
    sta $25
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
    .loc "test_struct_return.c", 27
    leax.local 0
    sta $20
    stx $21
    lda $20
    ldx $21
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
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda _results
    ldx _results+1
    sta $20
    stx $21
    lda #1
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx #0
    pha
    lda $24
    ldx $25
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
    .loc "test_struct_return.c", 30
    lda #3
    ldx #0
    sta $20
    stx $21
    lda #4
    ldx #0
    sta $22
    stx $23
    .loc "test_struct_return.c", 14
    leax.local 24
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_struct_return.c", 15
    leax.local 24
    sta $20
    stx $21
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
    lda _main__local_34
    ldx _main__local_34+1
    ldy _main__local_34+2
    ldz _main__local_34+3
    sta $20
    stx $21
    sty $22
    stz $23
    .loc "test_struct_return.c", 16
@inline_end2:
    .loc "test_struct_return.c", 30
    lda $20
    ldx $21
    ldy $22
    ldz $23
    sta _main__local_27
    stx _main__local_27+1
    sty _main__local_27+2
    stz _main__local_27+3
    .loc "test_struct_return.c", 31
    leax.local 8
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
    lda $22
    ldx $23
    sta $20
    lda _results
    ldx _results+1
    sta $22
    stx $23
    lda #2
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
    .loc "test_struct_return.c", 32
    leax.local 8
    sta $20
    stx $21
    lda $20
    ldx $21
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
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda _results
    ldx _results+1
    sta $20
    stx $21
    lda #3
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx #0
    pha
    lda $24
    ldx $25
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
    .loc "test_struct_return.c", 35
    lda #10
    ldx #0
    sta $20
    stx $21
    lda #20
    ldx #0
    sta $22
    stx $23
    .loc "test_struct_return.c", 14
    leax.local 20
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_struct_return.c", 15
    leax.local 20
    sta $20
    stx $21
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
    lda _main__local_61
    ldx _main__local_61+1
    ldy _main__local_61+2
    ldz _main__local_61+3
    sta $20
    stx $21
    sty $22
    stz $23
    .loc "test_struct_return.c", 16
@inline_end4:
    .loc "test_struct_return.c", 35
    lda $20
    ldx $21
    ldy $22
    ldz $23
    sta _main__local_54
    stx _main__local_54+1
    sty _main__local_54+2
    stz _main__local_54+3
    .loc "test_struct_return.c", 36
    leax.local 16
    sta $20
    stx $21
    .loc "test_struct_return.c", 20
    ldy #0
    lda ($20),y
    pha
    iny
    lda ($20),y
    tax
    pla
    sta $22
    stx $23
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
@inline_end6:
    lda $20
    ldx $21
    sta $22
    .loc "test_struct_return.c", 36
    lda _results
    ldx _results+1
    sta $20
    stx $21
    lda #4
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx #0
    pha
    lda $24
    ldx $25
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
    .loc "test_struct_return.c", 37
    leax.local 16
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
    lda $22
    ldx $23
    sta $20
    lda _results
    ldx _results+1
    sta $22
    stx $23
    lda #4
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
    .loc "test_struct_return.c", 38
    leax.local 16
    sta $20
    stx $21
    lda $20
    ldx $21
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
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda _results
    ldx _results+1
    sta $20
    stx $21
    lda #5
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx #0
    pha
    lda $24
    ldx $25
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
    .loc "test_struct_return.c", 41
    lda #170
    sta $20
    lda _results
    ldx _results+1
    sta $22
    stx $23
    lda #6
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
    .loc "test_struct_return.c", 43
    brk
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 28
    endproc


__zp_save_buf:

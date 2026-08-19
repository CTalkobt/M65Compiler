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

    .global _r
    .global _global_a
    .global _global_b
    .global _global_c
    .global _add_longs
    .global _bitwise_or
    .global _negate_long
    .global _main

    .segment "data"
    .byte 0
_r:
; .debug_var: @global _r offset=0 size=2 type=ptr scope=global
    .word 16384
_global_a:
; .debug_var: @global _global_a offset=0 size=4 type=int32 scope=global
    .dword 100000
_global_b:
; .debug_var: @global _global_b offset=0 size=4 type=int32 scope=global
    .dword 200000

    .segment "bss"
_global_c:
; .debug_var: @global _global_c offset=0 size=4 type=int32 scope=global
    .res 4

    .segment "code"

; function _add_longs

    ; Static buffer for struct return from _add_longs
    _add_longs__struct_buf:
    .byte 0, 0, 0, 0

; SAC inline storage: 8 bytes
    .global _add_longs__param_a
    _add_longs__param_a: .long 0
    .global _add_longs__param_b
    _add_longs__param_b: .long 0
    _add_longs__local_0: .long 0
    _add_longs__local_1: .long 0
    proc _add_longs, D#@_p_a, D#@_p_b
    .sac
    .var _fp = 0
    .loc "test_long.c", 7
    .var @_p_a = 2
    .var @_p_b = 6
; .debug_var: __add_longs @_p_a offset=2 size=4 type=int32 scope=parameter
; .debug_var: __add_longs @_p_b offset=6 size=4 type=int32 scope=parameter

@entry:
    .loc "test_long.c", 8
    lda _add_longs__param_b
    ldx _add_longs__param_b+1
    ldy _add_longs__param_b+2
    ldz _add_longs__param_b+3
    sta $10
    stx $11
    sty $12
    stz $13
    lda _add_longs__param_a
    ldx _add_longs__param_a+1
    ldy _add_longs__param_a+2
    ldz _add_longs__param_a+3
    add.32 .AXYZ, $10
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 8
    endproc

; function _bitwise_or

    ; Static buffer for struct return from _bitwise_or
    _bitwise_or__struct_buf:
    .byte 0, 0, 0, 0

; SAC inline storage: 8 bytes
    .global _bitwise_or__param_a
    _bitwise_or__param_a: .long 0
    .global _bitwise_or__param_b
    _bitwise_or__param_b: .long 0
    _bitwise_or__local_0: .long 0
    _bitwise_or__local_1: .long 0
    proc _bitwise_or, D#@_p_a, D#@_p_b
    .sac
    .var _fp = 0
    .loc "test_long.c", 11
    .var @_p_a = 2
    .var @_p_b = 6
; .debug_var: __bitwise_or @_p_a offset=2 size=4 type=int32 scope=parameter
; .debug_var: __bitwise_or @_p_b offset=6 size=4 type=int32 scope=parameter

@entry:
    .loc "test_long.c", 12
    lda _bitwise_or__param_b
    ldx _bitwise_or__param_b+1
    ldy _bitwise_or__param_b+2
    ldz _bitwise_or__param_b+3
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _bitwise_or__param_a
    ldx _bitwise_or__param_a+1
    ldy _bitwise_or__param_a+2
    ldz _bitwise_or__param_a+3
    ora.32 .AXYZ, __zp_scratch2
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers N, Z
    .frame_size 8
    endproc

; function _negate_long

    ; Static buffer for struct return from _negate_long
    _negate_long__struct_buf:
    .byte 0, 0, 0, 0

; SAC inline storage: 4 bytes
    .global _negate_long__param_x
    _negate_long__param_x: .long 0
    _negate_long__local_0: .long 0
    proc _negate_long, D#@_p_x
    .sac
    .var _fp = 0
    .loc "test_long.c", 15
    .var @_p_x = 2
; .debug_var: __negate_long @_p_x offset=2 size=4 type=int32 scope=parameter

@entry:
    .loc "test_long.c", 16
    lda _negate_long__param_x
    ldx _negate_long__param_x+1
    ldy _negate_long__param_x+2
    ldz _negate_long__param_x+3
    neg.16 .AX
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z
    .frame_size 4
    endproc

; function _main
; SAC inline storage: 40 bytes
    _main__local_0: .long 0
    _main__local_2: .long 0
    _main__local_4: .long 0
    _main__local_10: .long 0
    _main__local_18: .long 0
    _main__local_23: .long 0
    _main__local_26: .long 0
    _main__local_52: .long 0
    _main__local_64: .word 0
    _main__local_66: .long 0
    _main__local_75: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_long.c", 23
    .local @_l_big = 36
    .local @_l_comp = 24
    .local @_l_diff = 28
    .local @_l_masked = 32
    .local @_l_narrow = 2
    .local @_l_neg = 20
    .local @_l_small = 0
    .local @_l_sum = 12
    .local @_l_x = 4
    .local @_l_y = 8
    .local @_l_z = 16
; .debug_var: __main @_l_big offset=36 size=4 type=int32 scope=local
; .debug_var: __main @_l_comp offset=24 size=4 type=int32 scope=local
; .debug_var: __main @_l_diff offset=28 size=4 type=int32 scope=local
; .debug_var: __main @_l_masked offset=32 size=4 type=int32 scope=local
; .debug_var: __main @_l_narrow offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_neg offset=20 size=4 type=int32 scope=local
; .debug_var: __main @_l_small offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_sum offset=12 size=4 type=int32 scope=local
; .debug_var: __main @_l_x offset=4 size=4 type=int32 scope=local
; .debug_var: __main @_l_y offset=8 size=4 type=int32 scope=local
; .debug_var: __main @_l_z offset=16 size=4 type=int32 scope=local

@entry:
    .loc "test_long.c", 24
    lda #80
    sta _main__local_0
    lda #195
    sta _main__local_0+1
    .loc "test_long.c", 25
    lda #112
    ldx #17
    ldy #1
    ldz #0
    sta $20
    stx $21
    sty $22
    stz $23
    sta _main__local_2
    stx _main__local_2+1
    sty _main__local_2+2
    stz _main__local_2+3
    .loc "test_long.c", 26
    lda _main__local_0
    ldx _main__local_0+1
    ldy _main__local_0+2
    ldz _main__local_0+3
    sta $20
    stx $21
    sty $22
    stz $23
    lda _main__local_2
    ldx _main__local_2+1
    ldy _main__local_2+2
    ldz _main__local_2+3
    sta $20
    stx $21
    sty $22
    stz $23
    .loc "test_long.c", 8
    lda _main__local_2
    ldx _main__local_2+1
    ldy _main__local_2+2
    ldz _main__local_2+3
    sta $10
    stx $11
    sty $12
    stz $13
    lda _main__local_0
    ldx _main__local_0+1
    ldy _main__local_0+2
    ldz _main__local_0+3
    add.32 .AXYZ, $10
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    sta $24
    stx $25
    lda $24
    ldx $25
    sta $20
    stx $21
    sty $22
    stz $23
@inline_end0:
    .loc "test_long.c", 26
    lda $20
    ldx $21
    ldy $22
    ldz $23
    sta _main__local_4
    stx _main__local_4+1
    sty _main__local_4+2
    stz _main__local_4+3
    .loc "test_long.c", 27
    lda _global_a
    ldx _global_a+1
    ldy _global_a+2
    ldz _global_a+3
    sta $20
    stx $21
    sty $22
    stz $23
    lda _global_b
    ldx _global_b+1
    ldy _global_b+2
    ldz _global_b+3
    sta $24
    stx $25
    sty $26
    stz $27
    lda $20
    ldx $21
    ldy $22
    ldz $23
    sta $28
    stx $29
    sty $2A
    stz $2B
    lda $24
    ldx $25
    ldy $26
    ldz $27
    sta $28
    stx $29
    sty $2A
    stz $2B
    .loc "test_long.c", 12
    lda $20
    ldx $21
    ldy $22
    ldz $23
    ora.32 .AXYZ, $24
    sta $28
    stx $29
    sty $2A
    stz $2B
    lda $28
    ldx $29
    ldy $2A
    ldz $2B
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    sty $24
    stz $25
@inline_end2:
    .loc "test_long.c", 27
    lda $22
    ldx $23
    ldy $24
    ldz $25
    sta _main__local_10
    stx _main__local_10+1
    sty _main__local_10+2
    stz _main__local_10+3
    .loc "test_long.c", 28
    lda _main__local_0
    ldx _main__local_0+1
    ldy _main__local_0+2
    ldz _main__local_0+3
    sta $20
    stx $21
    sty $22
    stz $23
    .loc "test_long.c", 16
    lda _main__local_0
    ldx _main__local_0+1
    ldy _main__local_0+2
    ldz _main__local_0+3
    neg.16 .AX
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    sta $24
    stx $25
    lda $24
    ldx $25
    sta $20
    stx $21
    sty $22
    stz $23
@inline_end4:
    .loc "test_long.c", 28
    lda $20
    ldx $21
    ldy $22
    ldz $23
    sta _main__local_18
    stx _main__local_18+1
    sty _main__local_18+2
    stz _main__local_18+3
    .loc "test_long.c", 29
    lda _global_a
    ldx _global_a+1
    ldy _global_a+2
    ldz _global_a+3
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    not.16 .AX
    sta $24
    stx $25
    sty $26
    stz $27
    lda $24
    ldx $25
    ldy $26
    ldz $27
    sta _main__local_23
    stx _main__local_23+1
    sty _main__local_23+2
    stz _main__local_23+3
    .loc "test_long.c", 30
    lda _global_b
    ldx _global_b+1
    ldy _global_b+2
    ldz _global_b+3
    sta $20
    stx $21
    sty $22
    stz $23
    lda _global_a
    ldx _global_a+1
    ldy _global_a+2
    ldz _global_a+3
    sta $24
    stx $25
    sty $26
    stz $27
    lda $20
    ldx $21
    ldy $22
    ldz $23
    sub.32 .AXYZ, $24
    sta $28
    stx $29
    sty $2A
    stz $2B
    sta _main__local_26
    stx _main__local_26+1
    sty _main__local_26+2
    stz _main__local_26+3
    .loc "test_long.c", 33
    lda #4
    sta $20
    lda _r
    ldx _r+1
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
    .loc "test_long.c", 36
    lda #255
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    ldy #0
    ldz #0
    sta $22
    stx $23
    sty $24
    stz $25
    lda _main__local_4
    ldx _main__local_4+1
    ldy _main__local_4+2
    ldz _main__local_4+3
    and $22
    sta $26
    stx $27
    sty $28
    stz $29
    lda $26
    ldx $27
    ldy $28
    ldz $29
    sta $20
    lda _r
    ldx _r+1
    sta $20
    stx $21
    lda #1
    ldx #0
    sta $22
    stx $23
    lda $26
    ldx $27
    ldy $28
    ldz $29
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
    .loc "test_long.c", 39
    lda _global_a
    ldx _global_a+1
    ldy _global_a+2
    ldz _global_a+3
    sta $20
    stx $21
    sty $22
    stz $23
    lda _global_b
    ldx _global_b+1
    ldy _global_b+2
    ldz _global_b+3
    sta $24
    stx $25
    sty $26
    stz $27
    lda $20
    ldx $21
    ldy $22
    ldz $23
    cmp.s32 .AXYZ, $24
    bcc @if_then6
    bra @if_else7
@if_then6:
    .loc "test_long.c", 40
    lda #1
    sta $20
    lda _r
    ldx _r+1
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
    bra @if_end8
@if_else7:
    .loc "test_long.c", 42
    lda #0
    sta $20
    lda _r
    ldx _r+1
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
@if_end8:
    .loc "test_long.c", 45
    lda _global_a
    ldx _global_a+1
    ldy _global_a+2
    ldz _global_a+3
    sta $20
    stx $21
    sty $22
    stz $23
    lda #255
    sta $24
    sta $25
    lda $24
    ldx $25
    ldy #0
    ldz #0
    sta $26
    stx $27
    sty $28
    stz $29
    lda $20
    ldx $21
    ldy $22
    ldz $23
    and.32 .AXYZ, $26
    sta $2A
    stx $2B
    sty $2C
    stz $2D
    lda $2A
    ldx $2B
    ldy $2C
    ldz $2D
    sta _main__local_52
    stx _main__local_52+1
    sty _main__local_52+2
    stz _main__local_52+3
    .loc "test_long.c", 46
    lda #255
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    ldy #0
    ldz #0
    sta $22
    stx $23
    sty $24
    stz $25
    lda _main__local_52
    ldx _main__local_52+1
    ldy _main__local_52+2
    ldz _main__local_52+3
    and $22
    sta $26
    stx $27
    sty $28
    stz $29
    lda $26
    ldx $27
    ldy $28
    ldz $29
    sta $20
    lda _r
    ldx _r+1
    sta $20
    stx $21
    lda #3
    ldx #0
    sta $22
    stx $23
    lda $26
    ldx $27
    ldy $28
    ldz $29
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
    .loc "test_long.c", 49
    lda #42
    sta _main__local_64
    lda #0
    sta _main__local_64+1
    .loc "test_long.c", 50
    lda _main__local_64
    ldx _main__local_64+1
    ldy #0
    ldz #0
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    sta _main__local_66
    stx _main__local_66+1
    sty _main__local_66+2
    stz _main__local_66+3
    .loc "test_long.c", 51
    lda #255
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    ldy #0
    ldz #0
    sta $22
    stx $23
    sty $24
    stz $25
    lda _main__local_66
    ldx _main__local_66+1
    ldy _main__local_66+2
    ldz _main__local_66+3
    and $22
    sta $26
    stx $27
    sty $28
    stz $29
    lda $26
    ldx $27
    ldy $28
    ldz $29
    sta $20
    lda _r
    ldx _r+1
    sta $20
    stx $21
    lda #4
    ldx #0
    sta $22
    stx $23
    lda $26
    ldx $27
    ldy $28
    ldz $29
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
    .loc "test_long.c", 54
    lda _global_a
    ldx _global_a+1
    ldy _global_a+2
    ldz _global_a+3
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    sta $24
    stx $25
    lda $24
    ldx $25
    sta _main__local_75
    stx _main__local_75+1
    .loc "test_long.c", 55
    lda #255
    ldx #0
    sta $20
    stx $21
    lda _main__local_75
    ldx _main__local_75+1
    and $20
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    lda _r
    ldx _r+1
    sta $20
    stx $21
    lda #5
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
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
    .loc "test_long.c", 58
    lda #255
    sta $20
    sta $21
    sta $22
    sta $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    sta _global_c
    stx _global_c+1
    sty _global_c+2
    stz _global_c+3
    .loc "test_long.c", 59
    lda _global_c
    ldx _global_c+1
    ldy _global_c+2
    ldz _global_c+3
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    sta $24
    stx $25
    sty $26
    stz $27
    lda $24
    ldx $25
    ldy $26
    ldz $27
    add.32 .AXYZ, #1
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    sta _global_c
    stx _global_c+1
    sty _global_c+2
    stz _global_c+3
    .loc "test_long.c", 60
    lda _global_c
    ldx _global_c+1
    ldy _global_c+2
    ldz _global_c+3
    sta $20
    stx $21
    sty $22
    stz $23
    lda #255
    ldx #0
    sta $24
    stx $25
    lda $24
    ldx $25
    ldy #0
    ldz #0
    sta $26
    stx $27
    sty $28
    stz $29
    lda $20
    ldx $21
    ldy $22
    ldz $23
    and $26
    sta $2A
    stx $2B
    sty $2C
    stz $2D
    lda $2A
    ldx $2B
    ldy $2C
    ldz $2D
    sta $20
    lda _r
    ldx _r+1
    sta $20
    stx $21
    lda #6
    ldx #0
    sta $22
    stx $23
    lda $2A
    ldx $2B
    ldy $2C
    ldz $2D
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
    .loc "test_long.c", 62
    lda #170
    sta $20
    lda _r
    ldx _r+1
    sta $22
    stx $23
    lda #7
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
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 40
    endproc


__zp_save_buf:

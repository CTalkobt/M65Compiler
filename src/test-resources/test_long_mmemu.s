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
    .loc "test_long_mmemu.c", 6
    .var @_p_a = 2
    .var @_p_b = 6
; .debug_var: __add_longs @_p_a offset=2 size=4 type=int32 scope=parameter
; .debug_var: __add_longs @_p_b offset=6 size=4 type=int32 scope=parameter

@entry:
    .loc "test_long_mmemu.c", 7
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

; function _main
; SAC inline storage: 12 bytes
    _main__local_4: .long 0
    _main__local_33: .word 0
    _main__local_35: .long 0
    _main__local_41: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_long_mmemu.c", 14
    .local @_l_big = 8
    .local @_l_narrow = 2
    .local @_l_small = 0
    .local @_l_sum = 4
; .debug_var: __main @_l_big offset=8 size=4 type=int32 scope=local
; .debug_var: __main @_l_narrow offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_small offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_sum offset=4 size=4 type=int32 scope=local

@entry:
    .loc "test_long_mmemu.c", 17
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
    .loc "test_long_mmemu.c", 20
    lda #80
    ldx #195
    sta $20
    stx $21
    lda #112
    ldx #17
    ldy #1
    ldz #0
    sta $22
    stx $23
    sty $24
    stz $25
    lda $20
    ldx $21
    ldy #0
    ldz #0
    sta $26
    stx $27
    sty $28
    stz $29
    lda $26
    ldx $27
    ldy $28
    ldz $29
    sta $2A
    stx $2B
    sty $2C
    stz $2D
    lda $22
    ldx $23
    ldy $24
    ldz $25
    sta $2A
    stx $2B
    sty $2C
    stz $2D
    .loc "test_long_mmemu.c", 7
    lda $26
    ldx $27
    ldy $28
    ldz $29
    add.32 .AXYZ, $22
    sta $2A
    stx $2B
    sty $2C
    stz $2D
    lda $2A
    ldx $2B
    ldy $2C
    ldz $2D
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    sty $24
    stz $25
@inline_end0:
    .loc "test_long_mmemu.c", 20
    lda $22
    ldx $23
    ldy $24
    ldz $25
    sta _main__local_4
    stx _main__local_4+1
    sty _main__local_4+2
    stz _main__local_4+3
    .loc "test_long_mmemu.c", 21
    lda _main__local_4
    ldx _main__local_4+1
    ldy _main__local_4+2
    ldz _main__local_4+3
    sta $20
    lda _r
    ldx _r+1
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
    .loc "test_long_mmemu.c", 24
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
    bcc @if_then2
    bra @if_else3
@if_then2:
    .loc "test_long_mmemu.c", 25
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
    bra @if_end4
@if_else3:
    .loc "test_long_mmemu.c", 27
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
@if_end4:
    .loc "test_long_mmemu.c", 30
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
    lda _r
    ldx _r+1
    sta $20
    stx $21
    lda #3
    ldx #0
    sta $22
    stx $23
    lda $24
    ldx #0
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
    .loc "test_long_mmemu.c", 33
    lda #42
    sta _main__local_33
    lda #0
    sta _main__local_33+1
    .loc "test_long_mmemu.c", 34
    lda _main__local_33
    ldx _main__local_33+1
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
    sta _main__local_35
    stx _main__local_35+1
    sty _main__local_35+2
    stz _main__local_35+3
    .loc "test_long_mmemu.c", 35
    lda _main__local_35
    ldx _main__local_35+1
    ldy _main__local_35+2
    ldz _main__local_35+3
    sta $20
    lda _r
    ldx _r+1
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
    .loc "test_long_mmemu.c", 38
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
    sta _main__local_41
    stx _main__local_41+1
    .loc "test_long_mmemu.c", 39
    lda _main__local_41
    ldx _main__local_41+1
    sta $20
    lda _r
    ldx _r+1
    sta $22
    stx $23
    lda #5
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
    .loc "test_long_mmemu.c", 42
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
    .loc "test_long_mmemu.c", 43
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
    .loc "test_long_mmemu.c", 44
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
    lda _r
    ldx _r+1
    sta $20
    stx $21
    lda #6
    ldx #0
    sta $22
    stx $23
    lda $24
    ldx #0
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
    .loc "test_long_mmemu.c", 48
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
    add.32 .AXYZ, $24
    sta $28
    stx $29
    sty $2A
    stz $2B
    lda $28
    ldx $29
    ldy $2A
    ldz $2B
    sta _global_c
    stx _global_c+1
    sty _global_c+2
    stz _global_c+3
    .loc "test_long_mmemu.c", 49
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
    lda _r
    ldx _r+1
    sta $20
    stx $21
    lda #7
    ldx #0
    sta $22
    stx $23
    lda $24
    ldx #0
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
    .loc "test_long_mmemu.c", 50
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
    txa
    pha
    tya
    tax
    tza
    tay
    cmp #$80
    lda #0
    sbc #0
    taz
    pla
    sta $24
    stx $25
    sty $26
    stz $27
    lda $24
    ldx $25
    ldy $26
    ldz $27
    sta $20
    lda _r
    ldx _r+1
    sta $22
    stx $23
    lda #8
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
    .loc "test_long_mmemu.c", 51
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
    tya
    pha
    tza
    tax
    cmp #$80
    lda #0
    sbc #0
    tay
    taz
    pla
    sta $24
    stx $25
    sty $26
    stz $27
    lda $24
    ldx $25
    ldy $26
    ldz $27
    sta $20
    lda _r
    ldx _r+1
    sta $22
    stx $23
    lda #9
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
    .loc "test_long_mmemu.c", 52
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
    tza
    cmp #$80
    pha
    lda #0
    sbc #0
    tax
    tay
    taz
    pla
    sta $24
    stx $25
    sty $26
    stz $27
    lda $24
    ldx $25
    ldy $26
    ldz $27
    sta $20
    lda _r
    ldx _r+1
    sta $22
    stx $23
    lda #10
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
    .loc "test_long_mmemu.c", 54
    lda #170
    sta $20
    lda _r
    ldx _r+1
    sta $22
    stx $23
    lda #11
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
    .loc "test_long_mmemu.c", 56
    brk
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 12
    endproc


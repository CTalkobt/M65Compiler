    .o45
    .org $2000
    .weak __sp_base
    __sp_base = $0101
    .weak __static_chain
    .weak __zp_scratch
    .weak __zp_scratch2
    .weak __zp_scratch3
    .weak __zp_scratch4
    .global cc45.zeroPageStart
    __static_chain = $06
    __zp_scratch = $08
    __zp_scratch2 = $0A
    __zp_scratch3 = $0C
    __zp_scratch4 = $0E
    cc45.zeroPageStart = $08

    .extern _printf

    .global _r
    .global _add_short
    .global _mul_short
    .global _main

    .segment "data"
    .byte 0
_r:
; .debug_var: @global _r offset=0 size=2 type=ptr scope=global
    .word 16384

    .segment "code"

; function _add_short
; SAC inline storage: 6 bytes
    .global _add_short__param_a
    _add_short__param_a: .word 0
    .global _add_short__param_b
    _add_short__param_b: .word 0
    _add_short__local_0: .word 0
    _add_short__local_1: .word 0
    _add_short__local_2: .word 0
    _add_short__local_4: .word 0
    _add_short__local_6: .word 0
    proc _add_short, W#@_p_a, W#@_p_b
    .sac
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdio.h", 5
    .local @_l_result = 4
; .debug_var: __add_short @_l_result offset=4 size=2 type=int16 scope=local
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __add_short @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __add_short @_p_b offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_short.c", 10
    ldax #__str_0
    sta _add_short__local_2
    stx _add_short__local_2+1
    lda _add_short__param_b
    ldx _add_short__param_b+1
    sta $28
    stx $29
    lda _add_short__param_a
    ldx _add_short__param_a+1
    sta $2A
    stx $2B
    lda _add_short__local_2
    ldx _add_short__local_2+1
    sta $2C
    stx $2D
    lda $28
    ldx $29
    push .ax
    lda $2A
    ldx $2B
    push .ax
    lda $2C
    ldx $2D
    push .ax
    jsr _printf
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    .loc "test_short.c", 11
    lda _add_short__param_b
    ldx _add_short__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _add_short__param_a
    ldx _add_short__param_a+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    sta _add_short__local_4
    stx _add_short__local_4+1
    .loc "test_short.c", 12
    ldax #__str_1
    sta _add_short__local_6
    stx _add_short__local_6+1
    lda _add_short__local_4
    ldx _add_short__local_4+1
    sta $28
    stx $29
    lda _add_short__local_6
    ldx _add_short__local_6+1
    sta $2A
    stx $2B
    lda $28
    ldx $29
    push .ax
    lda $2A
    ldx $2B
    push .ax
    jsr _printf
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    .loc "test_short.c", 13
    lda _add_short__local_4
    ldx _add_short__local_4+1
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _mul_short
; SAC inline storage: 6 bytes
    .global _mul_short__param_a
    _mul_short__param_a: .word 0
    .global _mul_short__param_b
    _mul_short__param_b: .word 0
    _mul_short__local_0: .word 0
    _mul_short__local_1: .word 0
    _mul_short__local_2: .word 0
    _mul_short__local_4: .word 0
    _mul_short__local_6: .word 0
    proc _mul_short, W#@_p_a, W#@_p_b
    .sac
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdio.h", 12
    .local @_l_result = 4
; .debug_var: __mul_short @_l_result offset=4 size=2 type=int16 scope=local
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __mul_short @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __mul_short @_p_b offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_short.c", 17
    ldax #__str_2
    sta _mul_short__local_2
    stx _mul_short__local_2+1
    lda _mul_short__param_b
    ldx _mul_short__param_b+1
    sta $28
    stx $29
    lda _mul_short__param_a
    ldx _mul_short__param_a+1
    sta $2A
    stx $2B
    lda _mul_short__local_2
    ldx _mul_short__local_2+1
    sta $2C
    stx $2D
    lda $28
    ldx $29
    push .ax
    lda $2A
    ldx $2B
    push .ax
    lda $2C
    ldx $2D
    push .ax
    jsr _printf
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    .loc "test_short.c", 18
    lda _mul_short__param_b
    ldx _mul_short__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _mul_short__param_a
    ldx _mul_short__param_a+1
    mul.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _mul_short__local_4
    stx _mul_short__local_4+1
    .loc "test_short.c", 19
    ldax #__str_3
    sta _mul_short__local_6
    stx _mul_short__local_6+1
    lda _mul_short__local_4
    ldx _mul_short__local_4+1
    sta $28
    stx $29
    lda _mul_short__local_6
    ldx _mul_short__local_6+1
    sta $2A
    stx $2B
    lda $28
    ldx $29
    push .ax
    lda $2A
    ldx $2B
    push .ax
    jsr _printf
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    .loc "test_short.c", 20
    lda _mul_short__local_4
    ldx _mul_short__local_4+1
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _main
; SAC inline storage: 8 bytes
    _main__local_0: .word 0
    _main__local_2: .word 0
    _main__local_4: .word 0
    _main__local_6: .word 0
    _main__local_8: .word 0
    _main__local_10: .word 0
    _main__local_12: .word 0
    _main__local_14: .word 0
    _main__local_30: .word 0
    _main__local_31: .word 0
    _main__local_49: .word 0
    _main__local_51: .word 0
    _main__local_55: .word 0
    _main__local_57: .word 0
    _main__local_61: .word 0
    _main__local_63: .word 0
    _main__local_67: .word 0
    _main__local_69: .word 0
    _main__local_73: .word 0
    _main__local_75: .word 0
    _main__local_79: .word 0
    _main__local_81: .word 0
    _main__local_85: .word 0
    _main__local_87: .word 0
    _main__local_91: .word 0
    _main__local_93: .word 0
    proc _main
    .sac
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdio.h", 19
    .local @_l_neg = 6
    .local @_l_x = 0
    .local @_l_y = 2
    .local @_l_z = 4
; .debug_var: __main @_l_neg offset=6 size=2 type=int16 scope=local
; .debug_var: __main @_l_x offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_y offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_z offset=4 size=2 type=int16 scope=local

@entry:
    .loc "test_short.c", 24
    ldax #__str_4
    sta _main__local_0
    stx _main__local_0+1
    lda _main__local_0
    ldx _main__local_0+1
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    jsr _printf
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    .loc "test_short.c", 25
    lda #10
    sta _main__local_2
    lda #0
    sta _main__local_2+1
    .loc "test_short.c", 26
    lda #20
    sta _main__local_4
    lda #0
    sta _main__local_4+1
    .loc "test_short.c", 27
    ldax #__str_5
    sta _main__local_6
    stx _main__local_6+1
    lda _main__local_4
    ldx _main__local_4+1
    sta $28
    stx $29
    lda _main__local_2
    ldx _main__local_2+1
    sta $2A
    stx $2B
    lda _main__local_6
    ldx _main__local_6+1
    sta $2C
    stx $2D
    lda $28
    ldx $29
    push .ax
    lda $2A
    ldx $2B
    push .ax
    lda $2C
    ldx $2D
    push .ax
    jsr _printf
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    .loc "test_short.c", 28
    lda _main__local_4
    ldx _main__local_4+1
    sta _add_short__param_b
    stx _add_short__param_b+1
    lda _main__local_2
    ldx _main__local_2+1
    sta _add_short__param_a
    stx _add_short__param_a+1
    jsr _add_short
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_8
    stx _main__local_8+1
    .loc "test_short.c", 29
    ldax #__str_6
    sta _main__local_10
    stx _main__local_10+1
    lda _main__local_8
    ldx _main__local_8+1
    sta $28
    stx $29
    lda _main__local_10
    ldx _main__local_10+1
    sta $2A
    stx $2B
    lda $28
    ldx $29
    push .ax
    lda $2A
    ldx $2B
    push .ax
    jsr _printf
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    .loc "test_short.c", 30
    lda #251
    sta _main__local_12
    lda #255
    sta _main__local_12+1
    .loc "test_short.c", 32
    ldax #__str_7
    sta _main__local_14
    stx _main__local_14+1
    lda _main__local_14
    ldx _main__local_14+1
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    jsr _printf
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    lda _main__local_8
    ldx _main__local_8+1
    sta $20
    .loc "test_short.c", 33
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
    .loc "test_short.c", 34
    lda _main__local_12
    ldx _main__local_12+1
    clc
    adc #10
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    lda _r
    ldx _r+1
    sta $20
    stx $21
    lda #1
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
    .loc "test_short.c", 35
    lda #2
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
    .loc "test_short.c", 36
    lda _main__local_31
    ldx _main__local_31+1
    sta _mul_short__param_b
    stx _mul_short__param_b+1
    lda _main__local_30
    ldx _main__local_30+1
    sta _mul_short__param_a
    stx _mul_short__param_a+1
    jsr _mul_short
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda _r
    ldx _r+1
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
    lda _main__local_2
    ldx _main__local_2+1
    sta $20
    .loc "test_short.c", 37
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
    lda _main__local_4
    ldx _main__local_4+1
    sta $20
    .loc "test_short.c", 38
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
    .loc "test_short.c", 39
    lda #170
    sta $20
    lda _r
    ldx _r+1
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
    .loc "test_short.c", 40
    ldax #__str_8
    sta _main__local_49
    stx _main__local_49+1
    lda _main__local_49
    ldx _main__local_49+1
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    jsr _printf
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    .loc "test_short.c", 41
    ldax #__str_9
    sta _main__local_51
    stx _main__local_51+1
    lda _r
    ldx _r+1
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
    sta _main__local_55
    lda _main__local_55
    sta $28
    stx $29
    lda _main__local_51
    ldx _main__local_51+1
    sta $2A
    stx $2B
    lda $28
    ldx $29
    push .ax
    lda $2A
    ldx $2B
    push .ax
    jsr _printf
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    .loc "test_short.c", 42
    ldax #__str_10
    sta _main__local_57
    stx _main__local_57+1
    lda _r
    ldx _r+1
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
    sta _main__local_61
    lda _main__local_61
    sta $28
    stx $29
    lda _main__local_57
    ldx _main__local_57+1
    sta $2A
    stx $2B
    lda $28
    ldx $29
    push .ax
    lda $2A
    ldx $2B
    push .ax
    jsr _printf
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    .loc "test_short.c", 43
    ldax #__str_11
    sta _main__local_63
    stx _main__local_63+1
    lda _r
    ldx _r+1
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
    sta _main__local_67
    lda _main__local_67
    sta $28
    stx $29
    lda _main__local_63
    ldx _main__local_63+1
    sta $2A
    stx $2B
    lda $28
    ldx $29
    push .ax
    lda $2A
    ldx $2B
    push .ax
    jsr _printf
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    .loc "test_short.c", 44
    ldax #__str_12
    sta _main__local_69
    stx _main__local_69+1
    lda _r
    ldx _r+1
    sta $20
    stx $21
    lda #3
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
    sta _main__local_73
    lda _main__local_73
    sta $28
    stx $29
    lda _main__local_69
    ldx _main__local_69+1
    sta $2A
    stx $2B
    lda $28
    ldx $29
    push .ax
    lda $2A
    ldx $2B
    push .ax
    jsr _printf
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    .loc "test_short.c", 45
    ldax #__str_13
    sta _main__local_75
    stx _main__local_75+1
    lda _r
    ldx _r+1
    sta $20
    stx $21
    lda #4
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
    sta _main__local_79
    lda _main__local_79
    sta $28
    stx $29
    lda _main__local_75
    ldx _main__local_75+1
    sta $2A
    stx $2B
    lda $28
    ldx $29
    push .ax
    lda $2A
    ldx $2B
    push .ax
    jsr _printf
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    .loc "test_short.c", 46
    ldax #__str_14
    sta _main__local_81
    stx _main__local_81+1
    lda _r
    ldx _r+1
    sta $20
    stx $21
    lda #5
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
    sta _main__local_85
    lda _main__local_85
    sta $28
    stx $29
    lda _main__local_81
    ldx _main__local_81+1
    sta $2A
    stx $2B
    lda $28
    ldx $29
    push .ax
    lda $2A
    ldx $2B
    push .ax
    jsr _printf
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    .loc "test_short.c", 47
    ldax #__str_15
    sta _main__local_87
    stx _main__local_87+1
    lda _r
    ldx _r+1
    sta $20
    stx $21
    lda #6
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
    sta _main__local_91
    lda _main__local_91
    sta $28
    stx $29
    lda _main__local_87
    ldx _main__local_87+1
    sta $2A
    stx $2B
    lda $28
    ldx $29
    push .ax
    lda $2A
    ldx $2B
    push .ax
    jsr _printf
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    .loc "test_short.c", 48
    ldax #__str_16
    sta _main__local_93
    stx _main__local_93+1
    lda _main__local_93
    ldx _main__local_93+1
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    jsr _printf
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 8
    endproc


    .segment "data"
__str_0:
    .text "add_short: a=%d, b=%d
"
    .byte 0
__str_1:
    .text "add_short result: %d
"
    .byte 0
__str_2:
    .text "mul_short: a=%d, b=%d
"
    .byte 0
__str_3:
    .text "mul_short result: %d
"
    .byte 0
__str_4:
    .text "=== test_short START ===
"
    .byte 0
__str_5:
    .text "main: x=%d, y=%d
"
    .byte 0
__str_6:
    .text "main: z (add result)=%d
"
    .byte 0
__str_7:
    .text "Writing results to 0x4000...
"
    .byte 0
__str_8:
    .text "Results written:
"
    .byte 0
__str_9:
    .text "r[0]=%02X (expect 1E)
"
    .byte 0
__str_10:
    .text "r[1]=%02X (expect 05)
"
    .byte 0
__str_11:
    .text "r[2]=%02X (expect 02)
"
    .byte 0
__str_12:
    .text "r[3]=%02X (expect 0C)
"
    .byte 0
__str_13:
    .text "r[4]=%02X (expect 0A)
"
    .byte 0
__str_14:
    .text "r[5]=%02X (expect 14)
"
    .byte 0
__str_15:
    .text "r[6]=%02X (expect AA)
"
    .byte 0
__str_16:
    .text "=== test_short END ===
"
    .byte 0

__zp_save_buf:

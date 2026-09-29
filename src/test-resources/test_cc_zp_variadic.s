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

    .extern _assert_fail

    .global _results
    .global _sum_three
    .global _zp_caller
    .global _stack_caller
    .global _main

    .segment "data"
    .byte 0
_results:
; .debug_var: @global _results offset=0 size=2 type=ptr scope=global
    .word 16384

    .segment "code"

; function _sum_three
; SAC inline storage: 6 bytes
    .global _sum_three__param_a
    _sum_three__param_a: .word 0
    .global _sum_three__param_b
    _sum_three__param_b: .word 0
    .global _sum_three__param_c
    _sum_three__param_c: .word 0
    _sum_three__local_0: .word 0
    _sum_three__local_1: .word 0
    _sum_three__local_2: .word 0
    proc _sum_three, W#@_p_a, W#@_p_b, W#@_p_c
    .sac
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdio.h", 8
    .var @_p_a = 2
    .var @_p_b = 4
    .var @_p_c = 6
; .debug_var: __sum_three @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __sum_three @_p_b offset=4 size=2 type=int16 scope=parameter
; .debug_var: __sum_three @_p_c offset=6 size=2 type=int16 scope=parameter

@entry:
    .loc "test_cc_zp_variadic.c", 13
    lda _sum_three__param_b
    ldx _sum_three__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _sum_three__param_a
    ldx _sum_three__param_a+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda _sum_three__param_c
    ldx _sum_three__param_c+1
    add.16 .AX, $20
    sta $22
    stx $23
    lda $22
    ldx $23
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _zp_caller
; SAC inline storage: 6 bytes
    .global _zp_caller__param_x
    _zp_caller__param_x: .word 0
    .global _zp_caller__param_y
    _zp_caller__param_y: .word 0
    _zp_caller__local_0: .word 0
    _zp_caller__local_1: .word 0
    _zp_caller__local_2: .word 0
    proc _zp_caller, W#@_p_x, W#@_p_y
    .sac
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdio.h", 13
    .local @_l_result = 4
; .debug_var: __zp_caller @_l_result offset=4 size=2 type=int16 scope=local
    .var @_p_x = 2
    .var @_p_y = 4
; .debug_var: __zp_caller @_p_x offset=2 size=2 type=int16 scope=parameter
; .debug_var: __zp_caller @_p_y offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_cc_zp_variadic.c", 19
    lda _zp_caller__param_y
    ldx _zp_caller__param_y+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _zp_caller__param_x
    ldx _zp_caller__param_x+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda _zp_caller__param_x
    ldx _zp_caller__param_x+1
    sta $22
    stx $23
    lda _zp_caller__param_y
    ldx _zp_caller__param_y+1
    sta $22
    stx $23
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_cc_zp_variadic.c", 13
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $20
    clc
    adc $20
    sta $22
    lda $21
    adc $20+1
    sta $23
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end0:
    .loc "test_cc_zp_variadic.c", 19
    lda $20
    ldx $21
    sta _zp_caller__local_2
    stx _zp_caller__local_2+1
    .loc "test_cc_zp_variadic.c", 20
    lda _zp_caller__local_2
    ldx _zp_caller__local_2+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _stack_caller
; SAC inline storage: 6 bytes
    .global _stack_caller__param_x
    _stack_caller__param_x: .word 0
    .global _stack_caller__param_y
    _stack_caller__param_y: .word 0
    _stack_caller__local_0: .word 0
    _stack_caller__local_1: .word 0
    _stack_caller__local_2: .word 0
    proc _stack_caller, W#@_p_x, W#@_p_y
    .sac
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdio.h", 20
    .local @_l_result = 4
; .debug_var: __stack_caller @_l_result offset=4 size=2 type=int16 scope=local
    .var @_p_x = 2
    .var @_p_y = 4
; .debug_var: __stack_caller @_p_x offset=2 size=2 type=int16 scope=parameter
; .debug_var: __stack_caller @_p_y offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_cc_zp_variadic.c", 25
    lda _stack_caller__param_y
    ldx _stack_caller__param_y+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _stack_caller__param_x
    ldx _stack_caller__param_x+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda _stack_caller__param_x
    ldx _stack_caller__param_x+1
    sta $22
    stx $23
    lda _stack_caller__param_y
    ldx _stack_caller__param_y+1
    sta $22
    stx $23
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_cc_zp_variadic.c", 13
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $20
    clc
    adc $20
    sta $22
    lda $21
    adc $20+1
    sta $23
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end2:
    .loc "test_cc_zp_variadic.c", 25
    lda $20
    ldx $21
    sta _stack_caller__local_2
    stx _stack_caller__local_2+1
    .loc "test_cc_zp_variadic.c", 26
    lda _stack_caller__local_2
    ldx _stack_caller__local_2+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _main
; SAC inline storage: 12 bytes
    _main__local_0: .word 0
    _main__local_18: .word 0
    _main__local_20: .word 0
    _main__local_21: .word 0
    _main__local_22: .word 0
    _main__local_36: .word 0
    _main__local_54: .word 0
    _main__local_56: .word 0
    _main__local_57: .word 0
    _main__local_58: .word 0
    _main__local_72: .word 0
    _main__local_78: .word 0
    _main__local_90: .word 0
    _main__local_92: .word 0
    _main__local_93: .word 0
    _main__local_94: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/assert.h", 4
    .local @_l_r1 = 0
    .local @_l_r2 = 4
    .local @_l_r3 = 8
    .local @_l_result = 10
; .debug_var: __main @_l_r1 offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_r2 offset=4 size=2 type=int16 scope=local
; .debug_var: __main @_l_r3 offset=8 size=2 type=int16 scope=local
; .debug_var: __main @_l_result offset=10 size=2 type=int16 scope=local

@entry:
    .loc "test_cc_zp_variadic.c", 31
    lda #5
    ldx #0
    sta $20
    stx $21
    lda #10
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    sta $24
    stx $25
    lda $22
    ldx $23
    sta $24
    stx $25
    .loc "test_cc_zp_variadic.c", 25
    lda $20
    clc
    adc #10
    sta $24
    lda $21
    adc #0
    sta $25
    lda $20
    ldx $21
    sta $26
    stx $27
    lda $22
    ldx $23
    sta $20
    stx $21
    lda $24
    ldx $25
    sta $20
    stx $21
    .loc "test_cc_zp_variadic.c", 13
    lda $24
    ldx $25
    sta $20
    stx $21
    lda $24
    clc
    adc $24
    sta $20
    lda $25
    adc $24+1
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end5:
    .loc "test_cc_zp_variadic.c", 25
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
    .loc "test_cc_zp_variadic.c", 26
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end4:
    .loc "test_cc_zp_variadic.c", 31
    lda $22
    ldx $23
    sta _main__local_0
    stx _main__local_0+1
    .loc "test_cc_zp_variadic.c", 32
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, #30
    beq @tern_then8
    bra @tern_else9
@tern_then8:
    bra @tern_end10
@tern_else9:
    ldax #__str_11
    sta _main__local_20
    stx _main__local_20+1
    ldax #__str_12
    sta _main__local_21
    stx _main__local_21+1
    lda #32
    ldx #0
    sta _main__local_22
    stx _main__local_22+1
    lda _main__local_22
    ldx _main__local_22+1
    sta $28
    stx $29
    lda _main__local_21
    ldx _main__local_21+1
    sta $2A
    stx $2B
    lda _main__local_20
    ldx _main__local_20+1
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
    jsr _assert_fail
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@tern_end10:
    .loc "test_cc_zp_variadic.c", 33
    lda #255
    ldx #0
    sta $20
    stx $21
    lda _main__local_0
    ldx _main__local_0+1
    and $20
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    lda _results
    ldx _results+1
    sta $20
    stx $21
    lda #0
    sta $24
    sta $25
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
    .loc "test_cc_zp_variadic.c", 34
    lda _main__local_0
    ldx _main__local_0+1
    txa
    ldx #0
    sta $20
    stx $21
    lda #255
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    and $22
    sta $24
    stx $25
    lda $24
    ldx $25
    sta $20
    lda _results
    ldx _results+1
    sta $20
    stx $21
    lda #1
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
    .loc "test_cc_zp_variadic.c", 37
    lda #7
    ldx #0
    sta $20
    stx $21
    lda #3
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    sta $24
    stx $25
    lda $22
    ldx $23
    sta $24
    stx $25
    .loc "test_cc_zp_variadic.c", 19
    lda $20
    clc
    adc #3
    sta $24
    lda $21
    adc #0
    sta $25
    lda $20
    ldx $21
    sta $26
    stx $27
    lda $22
    ldx $23
    sta $20
    stx $21
    lda $24
    ldx $25
    sta $20
    stx $21
    .loc "test_cc_zp_variadic.c", 13
    lda $24
    ldx $25
    sta $20
    stx $21
    lda $24
    clc
    adc $24
    sta $20
    lda $25
    adc $24+1
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end14:
    .loc "test_cc_zp_variadic.c", 19
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
    .loc "test_cc_zp_variadic.c", 20
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end13:
    .loc "test_cc_zp_variadic.c", 37
    lda $22
    ldx $23
    sta _main__local_36
    stx _main__local_36+1
    .loc "test_cc_zp_variadic.c", 38
    lda _main__local_36
    ldx _main__local_36+1
    cmp.16 .AX, #20
    beq @tern_then17
    bra @tern_else18
@tern_then17:
    bra @tern_end19
@tern_else18:
    ldax #__str_20
    sta _main__local_56
    stx _main__local_56+1
    ldax #__str_21
    sta _main__local_57
    stx _main__local_57+1
    lda #38
    ldx #0
    sta _main__local_58
    stx _main__local_58+1
    lda _main__local_58
    ldx _main__local_58+1
    sta $28
    stx $29
    lda _main__local_57
    ldx _main__local_57+1
    sta $2A
    stx $2B
    lda _main__local_56
    ldx _main__local_56+1
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
    jsr _assert_fail
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@tern_end19:
    .loc "test_cc_zp_variadic.c", 39
    lda #255
    ldx #0
    sta $20
    stx $21
    lda _main__local_36
    ldx _main__local_36+1
    and $20
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    lda _results
    ldx _results+1
    sta $20
    stx $21
    lda #2
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
    .loc "test_cc_zp_variadic.c", 40
    lda _main__local_36
    ldx _main__local_36+1
    txa
    ldx #0
    sta $20
    stx $21
    lda #255
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    and $22
    sta $24
    stx $25
    lda $24
    ldx $25
    sta $20
    lda _results
    ldx _results+1
    sta $20
    stx $21
    lda #3
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
    .loc "test_cc_zp_variadic.c", 43
    lda #100
    ldx #0
    sta $20
    stx $21
    lda #56
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    sta $24
    stx $25
    lda $22
    ldx $23
    sta $24
    stx $25
    .loc "test_cc_zp_variadic.c", 19
    lda $20
    clc
    adc #56
    sta $24
    lda $21
    adc #0
    sta $25
    lda $20
    ldx $21
    sta $26
    stx $27
    lda $22
    ldx $23
    sta $20
    stx $21
    lda $24
    ldx $25
    sta $20
    stx $21
    .loc "test_cc_zp_variadic.c", 13
    lda $24
    ldx $25
    sta $20
    stx $21
    lda $24
    clc
    adc $24
    sta $20
    lda $25
    adc $24+1
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end23:
    .loc "test_cc_zp_variadic.c", 19
    lda $20
    ldx $21
    sta _main__local_78
    stx _main__local_78+1
    lda _main__local_78
    ldx _main__local_78+1
    sta $20
    stx $21
    .loc "test_cc_zp_variadic.c", 20
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end22:
    .loc "test_cc_zp_variadic.c", 43
    lda $22
    ldx $23
    sta _main__local_72
    stx _main__local_72+1
    .loc "test_cc_zp_variadic.c", 44
    lda _main__local_72
    ldx _main__local_72+1
    cmp.16 .AX, #312
    beq @tern_then26
    bra @tern_else27
@tern_then26:
    bra @tern_end28
@tern_else27:
    ldax #__str_29
    sta _main__local_92
    stx _main__local_92+1
    ldax #__str_30
    sta _main__local_93
    stx _main__local_93+1
    lda #44
    ldx #0
    sta _main__local_94
    stx _main__local_94+1
    lda _main__local_94
    ldx _main__local_94+1
    sta $28
    stx $29
    lda _main__local_93
    ldx _main__local_93+1
    sta $2A
    stx $2B
    lda _main__local_92
    ldx _main__local_92+1
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
    jsr _assert_fail
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@tern_end28:
    .loc "test_cc_zp_variadic.c", 45
    lda #255
    ldx #0
    sta $20
    stx $21
    lda _main__local_72
    ldx _main__local_72+1
    and $20
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    lda _results
    ldx _results+1
    sta $20
    stx $21
    lda #4
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
    .loc "test_cc_zp_variadic.c", 46
    lda _main__local_72
    ldx _main__local_72+1
    txa
    ldx #0
    sta $20
    stx $21
    lda #255
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    and $22
    sta $24
    stx $25
    lda $24
    ldx $25
    sta $20
    lda _results
    ldx _results+1
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
    .loc "test_cc_zp_variadic.c", 49
    lda #255
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
    .loc "test_cc_zp_variadic.c", 51
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 12
    endproc


    .segment "data"
__str_11:
    .text "r1 == 30"
    .byte 0
__str_12:
    .text "test_cc_zp_variadic.c"
    .byte 0
__str_20:
    .text "r2 == 20"
    .byte 0
__str_21:
    .text "test_cc_zp_variadic.c"
    .byte 0
__str_29:
    .text "r3 == 312"
    .byte 0
__str_30:
    .text "test_cc_zp_variadic.c"
    .byte 0

__zp_save_buf:

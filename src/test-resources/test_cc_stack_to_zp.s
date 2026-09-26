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
    .global _zp_add
    .global _zp_char_mul
    .global _main

    .segment "data"
    .byte 0
_results:
; .debug_var: @global _results offset=0 size=2 type=ptr scope=global
    .word 16384

    .segment "code"

; function _zp_add
; SAC inline storage: 4 bytes
    .global _zp_add__param_a
    _zp_add__param_a: .word 0
    .global _zp_add__param_b
    _zp_add__param_b: .word 0
    _zp_add__local_0: .word 0
    _zp_add__local_1: .word 0
    proc _zp_add, W#@_p_a, W#@_p_b
    .sac
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/assert.h", 6
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __zp_add @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __zp_add @_p_b offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_cc_stack_to_zp.c", 11
    lda _zp_add__param_b
    ldx _zp_add__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _zp_add__param_a
    ldx _zp_add__param_a+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _zp_char_mul
; SAC inline storage: 4 bytes
    .global _zp_char_mul__param_x
    _zp_char_mul__param_x: .word 0
    .global _zp_char_mul__param_y
    _zp_char_mul__param_y: .word 0
    _zp_char_mul__local_0: .word 0
    _zp_char_mul__local_1: .word 0
    proc _zp_char_mul, B#@_p_x, B#@_p_y
    .sac
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/assert.h", 11
    .var @_p_x = 2
    .var @_p_y = 4
; .debug_var: __zp_char_mul @_p_x offset=2 size=2 type=int8 scope=parameter
; .debug_var: __zp_char_mul @_p_y offset=4 size=2 type=int8 scope=parameter

@entry:
    lda _zp_char_mul__param_x
    ldx #0
    sta $20
    stx $21
    lda _zp_char_mul__param_y
    ldx #0
    sta $22
    stx $23
    .loc "test_cc_stack_to_zp.c", 16
    lda $20
    ldx $21
    mul.16 .AX, $22
    sta $24
    stx $25
    lda $24
    ldx $25
    sta $20
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
; SAC inline storage: 6 bytes
    _main__local_0: .word 0
    _main__local_10: .word 0
    _main__local_12: .word 0
    _main__local_13: .word 0
    _main__local_14: .word 0
    _main__local_28: .word 0
    _main__local_38: .word 0
    _main__local_40: .word 0
    _main__local_41: .word 0
    _main__local_42: .word 0
    _main__local_56: .word 0
    _main__local_72: .word 0
    _main__local_74: .word 0
    _main__local_75: .word 0
    _main__local_76: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/assert.h", 16
    .local @_l_r1 = 0
    .local @_l_r2 = 2
    .local @_l_r3 = 4
; .debug_var: __main @_l_r1 offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_r2 offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_r3 offset=4 size=2 type=int8 scope=local

@entry:
    .loc "test_cc_stack_to_zp.c", 22
    lda #10
    ldx #0
    sta $20
    stx $21
    lda #20
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
    .loc "test_cc_stack_to_zp.c", 11
    lda $20
    clc
    adc #20
    sta $24
    lda $21
    adc #0
    sta $25
    lda $24
    ldx $25
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end0:
    .loc "test_cc_stack_to_zp.c", 22
    lda $22
    ldx $23
    sta _main__local_0
    stx _main__local_0+1
    .loc "test_cc_stack_to_zp.c", 23
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, #30
    beq @tern_then2
    bra @tern_else3
@tern_then2:
    bra @tern_end4
@tern_else3:
    ldax #__str_5
    sta _main__local_12
    stx _main__local_12+1
    ldax #__str_6
    sta _main__local_13
    stx _main__local_13+1
    lda #23
    ldx #0
    sta _main__local_14
    stx _main__local_14+1
    lda _main__local_14
    ldx _main__local_14+1
    sta $28
    stx $29
    lda _main__local_13
    ldx _main__local_13+1
    sta $2A
    stx $2B
    lda _main__local_12
    ldx _main__local_12+1
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
@tern_end4:
    .loc "test_cc_stack_to_zp.c", 24
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
    .loc "test_cc_stack_to_zp.c", 25
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
    .loc "test_cc_stack_to_zp.c", 28
    lda #100
    ldx #0
    sta $20
    stx $21
    lda #55
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
    .loc "test_cc_stack_to_zp.c", 11
    lda $20
    clc
    adc #55
    sta $24
    lda $21
    adc #0
    sta $25
    lda $24
    ldx $25
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end7:
    .loc "test_cc_stack_to_zp.c", 28
    lda $22
    ldx $23
    sta _main__local_28
    stx _main__local_28+1
    .loc "test_cc_stack_to_zp.c", 29
    lda _main__local_28
    ldx _main__local_28+1
    cmp.16 .AX, #155
    beq @tern_then9
    bra @tern_else10
@tern_then9:
    bra @tern_end11
@tern_else10:
    ldax #__str_12
    sta _main__local_40
    stx _main__local_40+1
    ldax #__str_13
    sta _main__local_41
    stx _main__local_41+1
    lda #29
    ldx #0
    sta _main__local_42
    stx _main__local_42+1
    lda _main__local_42
    ldx _main__local_42+1
    sta $28
    stx $29
    lda _main__local_41
    ldx _main__local_41+1
    sta $2A
    stx $2B
    lda _main__local_40
    ldx _main__local_40+1
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
@tern_end11:
    .loc "test_cc_stack_to_zp.c", 30
    lda #255
    ldx #0
    sta $20
    stx $21
    lda _main__local_28
    ldx _main__local_28+1
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
    .loc "test_cc_stack_to_zp.c", 31
    lda _main__local_28
    ldx _main__local_28+1
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
    .loc "test_cc_stack_to_zp.c", 34
    lda #5
    ldx #0
    sta $20
    stx $21
    lda #7
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    sta $24
    lda $22
    ldx $23
    sta $20
    lda $24
    ldx #0
    sta $22
    lda $20
    ldx #0
    sta $22
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    .loc "test_cc_stack_to_zp.c", 16
    lda $22
    ldx $23
    mul.16 .AX, $24
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
@inline_end14:
    .loc "test_cc_stack_to_zp.c", 34
    lda $20
    ldx #0
    sta _main__local_56
    .loc "test_cc_stack_to_zp.c", 35
    lda #35
    sta $20
    lda _main__local_56
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
    beq @tern_then16
    bra @tern_else17
@tern_then16:
    bra @tern_end18
@tern_else17:
    ldax #__str_19
    sta _main__local_74
    stx _main__local_74+1
    ldax #__str_20
    sta _main__local_75
    stx _main__local_75+1
    lda #35
    ldx #0
    sta _main__local_76
    stx _main__local_76+1
    lda _main__local_76
    ldx _main__local_76+1
    sta $28
    stx $29
    lda _main__local_75
    ldx _main__local_75+1
    sta $2A
    stx $2B
    lda _main__local_74
    ldx _main__local_74+1
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
@tern_end18:
    .loc "test_cc_stack_to_zp.c", 36
    lda _results
    ldx _results+1
    sta $20
    stx $21
    lda #4
    ldx #0
    sta $22
    stx $23
    lda _main__local_56
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
    .loc "test_cc_stack_to_zp.c", 39
    lda #255
    sta $20
    lda _results
    ldx _results+1
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
    .loc "test_cc_stack_to_zp.c", 41
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc


    .segment "data"
__str_5:
    .text "r1 == 30"
    .byte 0
__str_6:
    .text "test_cc_stack_to_zp.c"
    .byte 0
__str_12:
    .text "r2 == 155"
    .byte 0
__str_13:
    .text "test_cc_stack_to_zp.c"
    .byte 0
__str_19:
    .text "r3 == 35"
    .byte 0
__str_20:
    .text "test_cc_stack_to_zp.c"
    .byte 0

__zp_save_buf:

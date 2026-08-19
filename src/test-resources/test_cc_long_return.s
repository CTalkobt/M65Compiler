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
    .global _stack_get_long
    .global _zp_get_long
    .global _main

    .segment "data"
    .byte 0
_results:
; .debug_var: @global _results offset=0 size=2 type=ptr scope=global
    .word 16384

    .segment "code"

; function _stack_get_long

    ; Static buffer for struct return from _stack_get_long
    _stack_get_long__struct_buf:
    .byte 0, 0, 0, 0

; SAC inline storage: 4 bytes
    .global _stack_get_long__param_a
    _stack_get_long__param_a: .word 0
    .global _stack_get_long__param_b
    _stack_get_long__param_b: .word 0
    _stack_get_long__local_0: .word 0
    _stack_get_long__local_1: .word 0
    proc _stack_get_long, W#@_p_a, W#@_p_b
    .sac
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/assert.h", 6
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __stack_get_long @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __stack_get_long @_p_b offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_cc_long_return.c", 11
    lda _stack_get_long__param_a
    ldx _stack_get_long__param_a+1
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
    tay
    txa
    taz
    lda #0
    ldx #0
    sta $24
    stx $25
    sty $26
    stz $27
    lda _stack_get_long__param_b
    ldx _stack_get_long__param_b+1
    ldy #0
    ldz #0
    sta $20
    stx $21
    sty $22
    stz $23
    lda $24
    ldx $25
    ldy $26
    ldz $27
    ora.32 .AXYZ, $20
    sta $28
    stx $29
    sty $2A
    stz $2B
    lda $28
    ldx $29
    ldy $2A
    ldz $2B
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z
    .frame_size 4
    endproc

; function _zp_get_long

    ; Static buffer for struct return from _zp_get_long
    _zp_get_long__struct_buf:
    .byte 0, 0, 0, 0

; SAC inline storage: 4 bytes
    .global _zp_get_long__param_a
    _zp_get_long__param_a: .word 0
    .global _zp_get_long__param_b
    _zp_get_long__param_b: .word 0
    _zp_get_long__local_0: .word 0
    _zp_get_long__local_1: .word 0
    proc _zp_get_long, W#@_p_a, W#@_p_b
    .sac
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/assert.h", 11
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __zp_get_long @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __zp_get_long @_p_b offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_cc_long_return.c", 16
    lda _zp_get_long__param_a
    ldx _zp_get_long__param_a+1
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
    tay
    txa
    taz
    lda #0
    ldx #0
    sta $24
    stx $25
    sty $26
    stz $27
    lda _zp_get_long__param_b
    ldx _zp_get_long__param_b+1
    ldy #0
    ldz #0
    sta $20
    stx $21
    sty $22
    stz $23
    lda $24
    ldx $25
    ldy $26
    ldz $27
    ora.32 .AXYZ, $20
    sta $28
    stx $29
    sty $2A
    stz $2B
    lda $28
    ldx $29
    ldy $2A
    ldz $2B
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z
    .frame_size 4
    endproc

; function _main
; SAC inline storage: 8 bytes
    _main__local_0: .long 0
    _main__local_13: .word 0
    _main__local_15: .word 0
    _main__local_16: .word 0
    _main__local_17: .word 0
    _main__local_49: .long 0
    _main__local_62: .word 0
    _main__local_64: .word 0
    _main__local_65: .word 0
    _main__local_66: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/assert.h", 16
    .local @_l_r1 = 0
    .local @_l_r2 = 4
; .debug_var: __main @_l_r1 offset=0 size=4 type=int32 scope=local
; .debug_var: __main @_l_r2 offset=4 size=4 type=int32 scope=local

@entry:
    .loc "test_cc_long_return.c", 22
    lda #52
    ldx #18
    sta $20
    stx $21
    lda #120
    ldx #86
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
    .loc "test_cc_long_return.c", 11
    lda $20
    ldx $21
    ldy #0
    ldz #0
    sta $24
    stx $25
    sty $26
    stz $27
    lda $24
    ldx $25
    ldy $26
    ldz $27
    tay
    txa
    taz
    lda #0
    ldx #0
    sta $28
    stx $29
    sty $2A
    stz $2B
    lda $22
    ldx $23
    ldy #0
    ldz #0
    sta $24
    stx $25
    sty $26
    stz $27
    lda $28
    ldx $29
    ldy $2A
    ldz $2B
    ora.32 .AXYZ, $24
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
    .loc "test_cc_long_return.c", 22
    lda $20
    ldx $21
    ldy $22
    ldz $23
    sta _main__local_0
    stx _main__local_0+1
    sty _main__local_0+2
    stz _main__local_0+3
    .loc "test_cc_long_return.c", 23
    lda _main__local_0
    ldx _main__local_0+1
    ldy _main__local_0+2
    ldz _main__local_0+3
    cmp.32 .AXYZ, #305419896
    beq @tern_then2
    bra @tern_else3
@tern_then2:
    bra @tern_end4
@tern_else3:
    ldax #__str_5
    sta _main__local_15
    stx _main__local_15+1
    ldax #__str_6
    sta _main__local_16
    stx _main__local_16+1
    lda #23
    ldx #0
    sta _main__local_17
    stx _main__local_17+1
    lda _main__local_17
    ldx _main__local_17+1
    sta $28
    stx $29
    lda _main__local_16
    ldx _main__local_16+1
    sta $2A
    stx $2B
    lda _main__local_15
    ldx _main__local_15+1
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
    .loc "test_cc_long_return.c", 24
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
    lda _main__local_0
    ldx _main__local_0+1
    ldy _main__local_0+2
    ldz _main__local_0+3
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
    lda _results
    ldx _results+1
    sta $20
    stx $21
    lda #0
    sta $22
    sta $23
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
    .loc "test_cc_long_return.c", 25
    lda _main__local_0
    ldx _main__local_0+1
    ldy _main__local_0+2
    ldz _main__local_0+3
    txa
    pha
    tya
    tax
    tza
    tay
    ldz #0
    pla
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
    lda _results
    ldx _results+1
    sta $20
    stx $21
    lda #1
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
    .loc "test_cc_long_return.c", 26
    lda _main__local_0
    ldx _main__local_0+1
    ldy _main__local_0+2
    ldz _main__local_0+3
    tza
    tax
    tya
    ldy #0
    ldz #0
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
    lda _results
    ldx _results+1
    sta $20
    stx $21
    lda #2
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
    .loc "test_cc_long_return.c", 27
    lda _main__local_0
    ldx _main__local_0+1
    ldy _main__local_0+2
    ldz _main__local_0+3
    tza
    ldx #0
    ldy #0
    ldz #0
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
    lda _results
    ldx _results+1
    sta $20
    stx $21
    lda #3
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
    .loc "test_cc_long_return.c", 30
    lda #205
    ldx #171
    sta $20
    stx $21
    lda #1
    ldx #239
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
    .loc "test_cc_long_return.c", 16
    lda $20
    ldx $21
    ldy #0
    ldz #0
    sta $24
    stx $25
    sty $26
    stz $27
    lda $24
    ldx $25
    ldy $26
    ldz $27
    tay
    txa
    taz
    lda #0
    ldx #0
    sta $28
    stx $29
    sty $2A
    stz $2B
    lda $22
    ldx $23
    ldy #0
    ldz #0
    sta $24
    stx $25
    sty $26
    stz $27
    lda $28
    ldx $29
    ldy $2A
    ldz $2B
    ora.32 .AXYZ, $24
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
@inline_end7:
    .loc "test_cc_long_return.c", 30
    lda $20
    ldx $21
    ldy $22
    ldz $23
    sta _main__local_49
    stx _main__local_49+1
    sty _main__local_49+2
    stz _main__local_49+3
    .loc "test_cc_long_return.c", 31
    lda _main__local_49
    ldx _main__local_49+1
    ldy _main__local_49+2
    ldz _main__local_49+3
    cmp.32 .AXYZ, #-1412567295
    beq @tern_then9
    bra @tern_else10
@tern_then9:
    bra @tern_end11
@tern_else10:
    ldax #__str_12
    sta _main__local_64
    stx _main__local_64+1
    ldax #__str_13
    sta _main__local_65
    stx _main__local_65+1
    lda #31
    ldx #0
    sta _main__local_66
    stx _main__local_66+1
    lda _main__local_66
    ldx _main__local_66+1
    sta $28
    stx $29
    lda _main__local_65
    ldx _main__local_65+1
    sta $2A
    stx $2B
    lda _main__local_64
    ldx _main__local_64+1
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
    .loc "test_cc_long_return.c", 32
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
    lda _main__local_49
    ldx _main__local_49+1
    ldy _main__local_49+2
    ldz _main__local_49+3
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
    lda _results
    ldx _results+1
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
    .loc "test_cc_long_return.c", 33
    lda _main__local_49
    ldx _main__local_49+1
    ldy _main__local_49+2
    ldz _main__local_49+3
    txa
    pha
    tya
    tax
    tza
    tay
    ldz #0
    pla
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
    lda _results
    ldx _results+1
    sta $20
    stx $21
    lda #5
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
    .loc "test_cc_long_return.c", 34
    lda _main__local_49
    ldx _main__local_49+1
    ldy _main__local_49+2
    ldz _main__local_49+3
    tza
    tax
    tya
    ldy #0
    ldz #0
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
    lda _results
    ldx _results+1
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
    .loc "test_cc_long_return.c", 35
    lda _main__local_49
    ldx _main__local_49+1
    ldy _main__local_49+2
    ldz _main__local_49+3
    tza
    ldx #0
    ldy #0
    ldz #0
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
    lda _results
    ldx _results+1
    sta $20
    stx $21
    lda #7
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
    .loc "test_cc_long_return.c", 38
    lda #255
    sta $20
    lda _results
    ldx _results+1
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
    .loc "test_cc_long_return.c", 40
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 8
    endproc


    .segment "data"
__str_5:
    .text "r1 == 0x12345678"
    .byte 0
__str_6:
    .text "test_cc_long_return.c"
    .byte 0
__str_12:
    .text "r2 == 0xABCDEF01"
    .byte 0
__str_13:
    .text "test_cc_long_return.c"
    .byte 0

__zp_save_buf:

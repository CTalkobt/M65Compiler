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
    .global _stack_get_point
    .global _zp_get_rect
    .global _main

    .segment "data"
    .byte 0
_results:
; .debug_var: @global _results offset=0 size=2 type=ptr scope=global
    .word 16384

    .segment "code"

; function _stack_get_point
; SAC inline storage: 6 bytes
    .global _stack_get_point__param_px
    _stack_get_point__param_px: .word 0
    .global _stack_get_point__param_py
    _stack_get_point__param_py: .word 0
    _stack_get_point__local_0: .word 0
    _stack_get_point__local_1: .word 0
    _stack_get_point__local_2: .word 0
    _stack_get_point__local_4: .word 0
    proc _stack_get_point, B#@_p_px, B#@_p_py
    .sac
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/assert.h", 16
    .local @_l_p = 4
; .debug_var: __stack_get_point @_l_p offset=4 size=2 type=int16 scope=local
    .var @_p_px = 2
    .var @_p_py = 4
; .debug_var: __stack_get_point @_p_px offset=2 size=2 type=int8 scope=parameter
; .debug_var: __stack_get_point @_p_py offset=4 size=2 type=int8 scope=parameter

@entry:
    .loc "test_cc_struct_return.c", 21
    leax.local 4
    sta $20
    stx $21
    lda.local 0
    ldy #0
    sta ($20),y
    .loc "test_cc_struct_return.c", 22
    leax.local 4
    sta $20
    stx $21
    lda $20
    ldx $21
    add.16 .AX, #1
    sta $22
    stx $23
    lda.local 2
    ldy #0
    sta ($22),y
    .loc "test_cc_struct_return.c", 23
    lda _stack_get_point__local_2
    ldx _stack_get_point__local_2+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _zp_get_rect

    ; Static buffer for struct return from _zp_get_rect
    _zp_get_rect__struct_buf:
    .byte 0, 0, 0, 0

; SAC inline storage: 8 bytes
    .global _zp_get_rect__param_l
    _zp_get_rect__param_l: .word 0
    .global _zp_get_rect__param_t
    _zp_get_rect__param_t: .word 0
    _zp_get_rect__local_0: .word 0
    _zp_get_rect__local_1: .word 0
    _zp_get_rect__local_2: .long 0
    _zp_get_rect__local_4: .word 0
    proc _zp_get_rect, W#@_p_l, W#@_p_t
    .sac
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/assert.h", 24
    .local @_l_r = 4
; .debug_var: __zp_get_rect @_l_r offset=4 size=4 type=int32 scope=local
    .var @_p_l = 2
    .var @_p_t = 4
; .debug_var: __zp_get_rect @_p_l offset=2 size=2 type=int16 scope=parameter
; .debug_var: __zp_get_rect @_p_t offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_cc_struct_return.c", 29
    leax.local 4
    sta $20
    stx $21
    lda _zp_get_rect__param_l
    ldx _zp_get_rect__param_l+1
    ldy #0
    sta ($20),y
    txa
    iny
    sta ($20),y
    .loc "test_cc_struct_return.c", 30
    leax.local 4
    sta $20
    stx $21
    lda $20
    ldx $21
    add.16 .AX, #2
    sta $22
    stx $23
    lda _zp_get_rect__param_t
    ldx _zp_get_rect__param_t+1
    ldy #0
    sta ($22),y
    txa
    iny
    sta ($22),y
    .loc "test_cc_struct_return.c", 31
    lda _zp_get_rect__local_2
    ldx _zp_get_rect__local_2+1
    ldy _zp_get_rect__local_2+2
    ldz _zp_get_rect__local_2+3
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 8
    endproc

; function _main
; SAC inline storage: 16 bytes
    _main__local_0: .word 0
    _main__local_5: .word 0
    _main__local_6: .word 0
    _main__local_7: .word 0
    _main__local_8: .word 0
    _main__local_10: .word 0
    _main__local_15: .word 0
    _main__local_21: .word 0
    _main__local_23: .word 0
    _main__local_24: .word 0
    _main__local_25: .word 0
    _main__local_33: .word 0
    _main__local_35: .word 0
    _main__local_36: .word 0
    _main__local_37: .word 0
    _main__local_39: .word 0
    _main__local_50: .long 0
    _main__local_51: .word 0
    _main__local_54: .word 0
    _main__local_55: .word 0
    _main__local_56: .word 0
    _main__local_57: .long 0
    _main__local_59: .word 0
    _main__local_64: .word 0
    _main__local_68: .word 0
    _main__local_70: .word 0
    _main__local_71: .word 0
    _main__local_72: .word 0
    _main__local_78: .word 0
    _main__local_80: .word 0
    _main__local_81: .word 0
    _main__local_82: .word 0
    _main__local_84: .word 0
    _main__local_88: .word 0
    _main__local_93: .word 0
    _main__local_98: .word 0
    _main__local_107: .word 0
    _main__local_117: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/assert.h", 32
    .local @_l_p = 2
    .local @_l_r = 8
; .debug_var: __main @_l_p offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_r offset=8 size=4 type=int32 scope=local

@entry:
    .loc "test_cc_struct_return.c", 37
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
    lda $22
    ldx $23
    sta $20
    .loc "test_cc_struct_return.c", 21
    leax.local 2
    sta $22
    stx $23
    lda $24
    ldy #0
    sta ($22),y
    .loc "test_cc_struct_return.c", 22
    leax.local 2
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #1
    sta $24
    stx $25
    lda $20
    ldy #0
    sta ($24),y
    lda _main__local_8
    ldx _main__local_8+1
    sta $20
    stx $21
    .loc "test_cc_struct_return.c", 23
@inline_end0:
    .loc "test_cc_struct_return.c", 37
    lda $20
    ldx $21
    sta _main__local_0
    stx _main__local_0+1
    .loc "test_cc_struct_return.c", 38
    leax.local 0
    sta $20
    stx $21
    ldy #0
    lda ($20),y
    ldx #0
    sta $22
    lda #10
    sta $20
    lda $22
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $24
    ldx $25
    cmp.16 .AX, $22
    beq @tern_then2
    bra @tern_else3
@tern_then2:
    bra @tern_end4
@tern_else3:
    ldax #__str_5
    sta _main__local_23
    stx _main__local_23+1
    ldax #__str_6
    sta _main__local_24
    stx _main__local_24+1
    lda #38
    ldx #0
    sta _main__local_25
    stx _main__local_25+1
    lda _main__local_25
    ldx _main__local_25+1
    sta $28
    stx $29
    lda _main__local_24
    ldx _main__local_24+1
    sta $2A
    stx $2B
    lda _main__local_23
    ldx _main__local_23+1
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
    .loc "test_cc_struct_return.c", 39
    leax.local 0
    sta $20
    stx $21
    lda $20
    ldx $21
    add.16 .AX, #1
    sta $22
    stx $23
    ldy #0
    lda ($22),y
    ldx #0
    sta $20
    lda #20
    sta $22
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx #0
    ldx #0
    sta $20
    stx $21
    lda $24
    ldx $25
    cmp.16 .AX, $20
    beq @tern_then7
    bra @tern_else8
@tern_then7:
    bra @tern_end9
@tern_else8:
    ldax #__str_10
    sta _main__local_35
    stx _main__local_35+1
    ldax #__str_11
    sta _main__local_36
    stx _main__local_36+1
    lda #39
    ldx #0
    sta _main__local_37
    stx _main__local_37+1
    lda _main__local_37
    ldx _main__local_37+1
    sta $28
    stx $29
    lda _main__local_36
    ldx _main__local_36+1
    sta $2A
    stx $2B
    lda _main__local_35
    ldx _main__local_35+1
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
@tern_end9:
    .loc "test_cc_struct_return.c", 40
    leax.local 0
    sta $20
    stx $21
    ldy #0
    lda ($20),y
    ldx #0
    sta $22
    lda _results
    ldx _results+1
    sta $20
    stx $21
    lda #0
    sta $24
    sta $25
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
    .loc "test_cc_struct_return.c", 41
    leax.local 0
    sta $20
    stx $21
    lda $20
    ldx $21
    add.16 .AX, #1
    sta $22
    stx $23
    ldy #0
    lda ($22),y
    ldx #0
    sta $20
    lda _results
    ldx _results+1
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
    .loc "test_cc_struct_return.c", 44
    lda #0
    ldx #1
    sta $20
    stx $21
    lda #0
    ldx #2
    sta $22
    stx $23
    .loc "test_cc_struct_return.c", 29
    leax.local 8
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_cc_struct_return.c", 30
    leax.local 8
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
    lda _main__local_57
    ldx _main__local_57+1
    ldy _main__local_57+2
    ldz _main__local_57+3
    sta $20
    stx $21
    sty $22
    stz $23
    .loc "test_cc_struct_return.c", 31
@inline_end12:
    .loc "test_cc_struct_return.c", 44
    lda $20
    ldx $21
    ldy $22
    ldz $23
    sta _main__local_50
    stx _main__local_50+1
    sty _main__local_50+2
    stz _main__local_50+3
    .loc "test_cc_struct_return.c", 45
    leax.local 12
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
    cmp.16 .AX, #256
    beq @tern_then14
    bra @tern_else15
@tern_then14:
    bra @tern_end16
@tern_else15:
    ldax #__str_17
    sta _main__local_70
    stx _main__local_70+1
    ldax #__str_18
    sta _main__local_71
    stx _main__local_71+1
    lda #45
    ldx #0
    sta _main__local_72
    stx _main__local_72+1
    lda _main__local_72
    ldx _main__local_72+1
    sta $28
    stx $29
    lda _main__local_71
    ldx _main__local_71+1
    sta $2A
    stx $2B
    lda _main__local_70
    ldx _main__local_70+1
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
@tern_end16:
    .loc "test_cc_struct_return.c", 46
    leax.local 12
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
    cmp.16 .AX, #512
    beq @tern_then19
    bra @tern_else20
@tern_then19:
    bra @tern_end21
@tern_else20:
    ldax #__str_22
    sta _main__local_80
    stx _main__local_80+1
    ldax #__str_23
    sta _main__local_81
    stx _main__local_81+1
    lda #46
    ldx #0
    sta _main__local_82
    stx _main__local_82+1
    lda _main__local_82
    ldx _main__local_82+1
    sta $28
    stx $29
    lda _main__local_81
    ldx _main__local_81+1
    sta $2A
    stx $2B
    lda _main__local_80
    ldx _main__local_80+1
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
@tern_end21:
    .loc "test_cc_struct_return.c", 47
    leax.local 12
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
    lda #255
    ldx #0
    sta $20
    stx $21
    lda $22
    ldx $23
    and $20
    sta $24
    stx $25
    lda _results
    ldx _results+1
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
    .loc "test_cc_struct_return.c", 48
    leax.local 12
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
    .loc "test_cc_struct_return.c", 49
    leax.local 12
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
    lda #255
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    and $22
    sta $24
    stx $25
    lda _results
    ldx _results+1
    sta $20
    stx $21
    lda #4
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
    .loc "test_cc_struct_return.c", 50
    leax.local 12
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
    txa
    ldx #0
    sta $22
    stx $23
    lda #255
    ldx #0
    sta $20
    stx $21
    lda $22
    ldx $23
    and $20
    sta $24
    stx $25
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
    .loc "test_cc_struct_return.c", 53
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
    .loc "test_cc_struct_return.c", 55
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 16
    endproc


    .segment "data"
__str_5:
    .text "p.x == 10"
    .byte 0
__str_6:
    .text "test_cc_struct_return.c"
    .byte 0
__str_10:
    .text "p.y == 20"
    .byte 0
__str_11:
    .text "test_cc_struct_return.c"
    .byte 0
__str_17:
    .text "r.left == 256"
    .byte 0
__str_18:
    .text "test_cc_struct_return.c"
    .byte 0
__str_22:
    .text "r.top == 512"
    .byte 0
__str_23:
    .text "test_cc_struct_return.c"
    .byte 0

__zp_save_buf:

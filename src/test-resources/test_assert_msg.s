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

    .extern _assert_msg_fail

    .global _results
    .global _main

    .segment "data"
    .byte 0
_results:
; .debug_var: @global _results offset=0 size=2 type=ptr scope=global
    .word 16384

    .segment "code"

; function _main
; SAC inline storage: 6 bytes
    _main__local_0: .word 0
    _main__local_4: .word 0
    _main__local_6: .word 0
    _main__local_7: .word 0
    _main__local_8: .word 0
    _main__local_9: .word 0
    _main__local_14: .word 0
    _main__local_18: .word 0
    _main__local_20: .word 0
    _main__local_21: .word 0
    _main__local_22: .word 0
    _main__local_23: .word 0
    _main__local_28: .word 0
    _main__local_32: .word 0
    _main__local_34: .word 0
    _main__local_35: .word 0
    _main__local_36: .word 0
    _main__local_37: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/assert.h", 5
    .local @_l_name = 4
    .local @_l_x = 0
    .local @_l_y = 2
; .debug_var: __main @_l_name offset=4 size=2 type=ptr scope=local
; .debug_var: __main @_l_x offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_y offset=2 size=2 type=int16 scope=local

@entry:
    .loc "test_assert_msg.c", 10
    lda #10
    sta _main__local_0
    lda #0
    sta _main__local_0+1
    .loc "test_assert_msg.c", 11
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, #0
    beq @tern_else1
    bcs @tern_then0
    bra @tern_else1
@tern_then0:
    bra @tern_end2
@tern_else1:
    ldax #__str_3
    sta _main__local_6
    stx _main__local_6+1
    ldax #__str_4
    sta _main__local_7
    stx _main__local_7+1
    lda #11
    ldx #0
    sta _main__local_8
    stx _main__local_8+1
    ldax #__str_5
    sta _main__local_9
    stx _main__local_9+1
    lda _main__local_9
    ldx _main__local_9+1
    sta $28
    stx $29
    lda _main__local_8
    ldx _main__local_8+1
    sta $2A
    stx $2B
    lda _main__local_7
    ldx _main__local_7+1
    sta $2C
    stx $2D
    lda _main__local_6
    ldx _main__local_6+1
    sta $2E
    stx $2F
    lda $28
    ldx $29
    push .ax
    lda $2A
    ldx $2B
    push .ax
    lda $2C
    ldx $2D
    push .ax
    lda $2E
    ldx $2F
    push .ax
    jsr _assert_msg_fail
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@tern_end2:
    .loc "test_assert_msg.c", 12
    lda #1
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
    .loc "test_assert_msg.c", 15
    lda #42
    sta _main__local_14
    lda #0
    sta _main__local_14+1
    .loc "test_assert_msg.c", 16
    lda _main__local_14
    ldx _main__local_14+1
    cmp.16 .AX, #42
    beq @tern_then6
    bra @tern_else7
@tern_then6:
    bra @tern_end8
@tern_else7:
    ldax #__str_9
    sta _main__local_20
    stx _main__local_20+1
    ldax #__str_10
    sta _main__local_21
    stx _main__local_21+1
    lda #16
    ldx #0
    sta _main__local_22
    stx _main__local_22+1
    ldax #__str_11
    sta _main__local_23
    stx _main__local_23+1
    lda _main__local_23
    ldx _main__local_23+1
    sta $28
    stx $29
    lda _main__local_22
    ldx _main__local_22+1
    sta $2A
    stx $2B
    lda _main__local_21
    ldx _main__local_21+1
    sta $2C
    stx $2D
    lda _main__local_20
    ldx _main__local_20+1
    sta $2E
    stx $2F
    lda $28
    ldx $29
    push .ax
    lda $2A
    ldx $2B
    push .ax
    lda $2C
    ldx $2D
    push .ax
    lda $2E
    ldx $2F
    push .ax
    jsr _assert_msg_fail
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@tern_end8:
    .loc "test_assert_msg.c", 17
    lda #1
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
    .loc "test_assert_msg.c", 20
    ldax #__str_12
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_28
    stx _main__local_28+1
    .loc "test_assert_msg.c", 21
    lda _NULL
    ldx _NULL+1
    sta $20
    stx $21
    lda _main__local_28
    ldx _main__local_28+1
    cmp.16 .AX, $20
    bne @tern_then13
    bra @tern_else14
@tern_then13:
    bra @tern_end15
@tern_else14:
    ldax #__str_16
    sta _main__local_34
    stx _main__local_34+1
    ldax #__str_17
    sta _main__local_35
    stx _main__local_35+1
    lda #21
    ldx #0
    sta _main__local_36
    stx _main__local_36+1
    ldax #__str_18
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
    lda _main__local_34
    ldx _main__local_34+1
    sta $2E
    stx $2F
    lda $28
    ldx $29
    push .ax
    lda $2A
    ldx $2B
    push .ax
    lda $2C
    ldx $2D
    push .ax
    lda $2E
    ldx $2F
    push .ax
    jsr _assert_msg_fail
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@tern_end15:
    .loc "test_assert_msg.c", 22
    lda #1
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
    .loc "test_assert_msg.c", 25
    lda #255
    sta $20
    lda _results
    ldx _results+1
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
    .loc "test_assert_msg.c", 27
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
__str_3:
    .text "x > 0"
    .byte 0
__str_4:
    .text "test_assert_msg.c"
    .byte 0
__str_5:
    .text "x must be positive"
    .byte 0
__str_9:
    .text "y == 42"
    .byte 0
__str_10:
    .text "test_assert_msg.c"
    .byte 0
__str_11:
    .text "y should equal 42"
    .byte 0
__str_12:
    .text "test"
    .byte 0
__str_16:
    .text "name != NULL"
    .byte 0
__str_17:
    .text "test_assert_msg.c"
    .byte 0
__str_18:
    .text "name pointer is NULL"
    .byte 0

__zp_save_buf:

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

    .global _main

    .segment "code"

; function _main
; SAC inline storage: 2 bytes
    _main__local_4: .word 0
    _main__local_8: .word 0
    _main__local_10: .word 0
    _main__local_11: .word 0
    _main__local_12: .word 0
    _main__local_15: .word 0
    _main__local_17: .word 0
    _main__local_18: .word 0
    _main__local_19: .word 0
    _main__local_22: .word 0
    _main__local_24: .word 0
    _main__local_25: .word 0
    _main__local_26: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/assert.h", 3
    .local @_l_x = 0
; .debug_var: __main @_l_x offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_assert_pass.c", 8
    .loc "test_assert_pass.c", 9
    .loc "test_assert_pass.c", 10
    .loc "test_assert_pass.c", 11
    .loc "test_assert_pass.c", 14
    lda #42
    sta _main__local_4
    lda #0
    sta _main__local_4+1
    .loc "test_assert_pass.c", 15
    lda _main__local_4
    ldx _main__local_4+1
    cmp.16 .AX, #42
    beq @tern_then0
    bra @tern_else1
@tern_then0:
    bra @tern_end2
@tern_else1:
    ldax #__str_3
    sta _main__local_10
    stx _main__local_10+1
    ldax #__str_4
    sta _main__local_11
    stx _main__local_11+1
    lda #15
    ldx #0
    sta _main__local_12
    stx _main__local_12+1
    lda _main__local_12
    ldx _main__local_12+1
    sta $28
    stx $29
    lda _main__local_11
    ldx _main__local_11+1
    sta $2A
    stx $2B
    lda _main__local_10
    ldx _main__local_10+1
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
@tern_end2:
    .loc "test_assert_pass.c", 16
    lda _main__local_4
    ldx _main__local_4+1
    cmp.16 .AX, #0
    beq @tern_else6
    bcs @tern_then5
    bra @tern_else6
@tern_then5:
    bra @tern_end7
@tern_else6:
    ldax #__str_8
    sta _main__local_17
    stx _main__local_17+1
    ldax #__str_9
    sta _main__local_18
    stx _main__local_18+1
    lda #16
    ldx #0
    sta _main__local_19
    stx _main__local_19+1
    lda _main__local_19
    ldx _main__local_19+1
    sta $28
    stx $29
    lda _main__local_18
    ldx _main__local_18+1
    sta $2A
    stx $2B
    lda _main__local_17
    ldx _main__local_17+1
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
@tern_end7:
    .loc "test_assert_pass.c", 17
    lda _main__local_4
    ldx _main__local_4+1
    cmp.16 .AX, #100
    bcc @tern_then10
    bra @tern_else11
@tern_then10:
    bra @tern_end12
@tern_else11:
    ldax #__str_13
    sta _main__local_24
    stx _main__local_24+1
    ldax #__str_14
    sta _main__local_25
    stx _main__local_25+1
    lda #17
    ldx #0
    sta _main__local_26
    stx _main__local_26+1
    lda _main__local_26
    ldx _main__local_26+1
    sta $28
    stx $29
    lda _main__local_25
    ldx _main__local_25+1
    sta $2A
    stx $2B
    lda _main__local_24
    ldx _main__local_24+1
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
@tern_end12:
    .loc "test_assert_pass.c", 20
    brk
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc


    .segment "data"
__str_3:
    .text "x == 42"
    .byte 0
__str_4:
    .text "test_assert_pass.c"
    .byte 0
__str_8:
    .text "x > 0"
    .byte 0
__str_9:
    .text "test_assert_pass.c"
    .byte 0
__str_13:
    .text "x < 100"
    .byte 0
__str_14:
    .text "test_assert_pass.c"
    .byte 0

__zp_save_buf:

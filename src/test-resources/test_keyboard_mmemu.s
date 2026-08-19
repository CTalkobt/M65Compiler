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
    .global _main

    .segment "bss"
__test_key_pressed__local_bit_table:
; .debug_var: @global __test_key_pressed__local_bit_table offset=0 size=2 type=int8 scope=global
    .res 8
    .segment "data"
    .byte 0
_r:
; .debug_var: @global _r offset=0 size=2 type=ptr scope=global
    .word 16384

    .segment "bss"
__main__local_bit_table:
; .debug_var: @global __main__local_bit_table offset=0 size=2 type=int8 scope=global
    .res 8

    .segment "code"

; function _main
; SAC inline storage: 36 bytes
    _main__local_25: .word 0
    _main__local_26: .word 0
    _main__local_34: .word 0
    _main__local_43: .word 0
    _main__local_57: .word 0
    _main__local_66: .word 0
    _main__local_67: .word 0
    _main__local_75: .word 0
    _main__local_84: .word 0
    _main__local_98: .word 0
    _main__local_110: .word 0
    _main__local_111: .word 0
    _main__local_119: .word 0
    _main__local_128: .word 0
    _main__local_142: .word 0
    _main__local_154: .word 0
    _main__local_155: .word 0
    _main__local_163: .word 0
    _main__local_172: .word 0
    _main__local_186: .word 0
    _main__local_198: .word 0
    _main__local_199: .word 0
    _main__local_207: .word 0
    _main__local_216: .word 0
    _main__local_230: .word 0
    _main__local_239: .word 0
    _main__local_240: .word 0
    _main__local_241: .word 0
    _main__local_246: .word 0
    _main__local_248: .word 0
    _main__local_257: .word 0
    _main__local_260: .word 0
    _main__local_271: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/mega65.h", 15
    .local @_l_col = 30
    .local @_l_row = 32
    .local @_l_val = 34
; .debug_var: __main @_l_col offset=30 size=2 type=int8 scope=local
; .debug_var: __main @_l_row offset=32 size=2 type=int8 scope=local
; .debug_var: __main @_l_val offset=34 size=2 type=int8 scope=local

@entry:
    .loc "test_keyboard_mmemu.c", 25
    lda #39
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
    .loc "test_keyboard_mmemu.c", 26
    lda #8
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
    .loc "test_keyboard_mmemu.c", 27
    lda #17
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
    .loc "test_keyboard_mmemu.c", 28
    lda #32
    sta $20
    lda _r
    ldx _r+1
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
    .loc "test_keyboard_mmemu.c", 29
    lda #63
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
    .loc "test_keyboard_mmemu.c", 34
    lda #255
    sta $20
    lda #1
    ldx #220
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    lda $20
    ldy #0
    sta ($24),y
    .loc "test_keyboard_mmemu.c", 36
    lda #39
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda $22
    ldx #0
    ldx #0
    sta $20
    stx $21
    .loc "test_keyboard_mmemu.c", 14
    lda $20
    ldx $21
    lsr.16 .AX
    lsr.16 .AX
    lsr.16 .AX
    sta $22
    stx $23
    lda #7
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    and.16 .AX, $24
    sta $26
    stx $27
    lda $26
    ldx $27
    sta $22
    .loc "test_keyboard_mmemu.c", 15
    lda #7
    sta $24
    lda $24
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $20
    ldx $21
    and.16 .AX, $26
    sta $24
    stx $25
    lda $24
    ldx $25
    sta $20
    .loc "test_keyboard_mmemu.c", 16
    lda $22
    ldx #0
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #__main__local_bit_table
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
    sta $26
    lda #255
    sta $22
    lda $26
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $24
    ldx $25
    eor $26
    sta $22
    stx $23
    lda #0
    ldx #220
    sta $24
    stx $25
    lda $24
    ldx $25
    sta $26
    stx $27
    lda $22
    ldy #0
    sta ($26),y
    .loc "test_keyboard_mmemu.c", 17
    lda #1
    ldx #220
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
    lda $24
    ldx $25
    sta $22
    .loc "test_keyboard_mmemu.c", 18
    lda #255
    sta $24
    lda $22
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $26
    ldx $27
    eor.16 .AX, $22
    sta $24
    stx $25
    lda $20
    ldx #0
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #__main__local_bit_table
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
    sta $26
    lda $26
    ldx #0
    ldx #0
    sta $20
    stx $21
    lda $24
    ldx $25
    and.16 .AX, $20
    sta $22
    stx $23
    lda $22
    ldx $23
    bne @tern_then8
    cmp #$00
    bne @tern_then8
    bra @tern_else9
@tern_then8:
    lda #1
    ldx #0
    sta $20
    stx $21
    bra @tern_end10
@tern_else9:
    lda #0
    sta $20
    sta $21
@tern_end10:
    lda $20
    ldx $21
    sta $22
@inline_end7:
    .loc "test_keyboard_mmemu.c", 36
    lda _r
    ldx _r+1
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
    .loc "test_keyboard_mmemu.c", 37
    lda #17
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda $22
    ldx #0
    ldx #0
    sta $20
    stx $21
    .loc "test_keyboard_mmemu.c", 14
    lda $20
    ldx $21
    lsr.16 .AX
    lsr.16 .AX
    lsr.16 .AX
    sta $22
    stx $23
    lda #7
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    and.16 .AX, $24
    sta $26
    stx $27
    lda $26
    ldx $27
    sta $22
    .loc "test_keyboard_mmemu.c", 15
    lda #7
    sta $24
    lda $24
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $20
    ldx $21
    and.16 .AX, $26
    sta $24
    stx $25
    lda $24
    ldx $25
    sta $20
    .loc "test_keyboard_mmemu.c", 16
    lda $22
    ldx #0
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #__main__local_bit_table
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
    sta $26
    lda #255
    sta $22
    lda $26
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $24
    ldx $25
    eor $26
    sta $22
    stx $23
    lda #0
    ldx #220
    sta $24
    stx $25
    lda $24
    ldx $25
    sta $26
    stx $27
    lda $22
    ldy #0
    sta ($26),y
    .loc "test_keyboard_mmemu.c", 17
    lda #1
    ldx #220
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
    lda $24
    ldx $25
    sta $22
    .loc "test_keyboard_mmemu.c", 18
    lda #255
    sta $24
    lda $22
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $26
    ldx $27
    eor.16 .AX, $22
    sta $24
    stx $25
    lda $20
    ldx #0
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #__main__local_bit_table
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
    sta $26
    lda $26
    ldx #0
    ldx #0
    sta $20
    stx $21
    lda $24
    ldx $25
    and.16 .AX, $20
    sta $22
    stx $23
    lda $22
    ldx $23
    bne @tern_then13
    cmp #$00
    bne @tern_then13
    bra @tern_else14
@tern_then13:
    lda #1
    ldx #0
    sta $20
    stx $21
    bra @tern_end15
@tern_else14:
    lda #0
    sta $20
    sta $21
@tern_end15:
    lda $20
    ldx $21
    sta $22
@inline_end12:
    .loc "test_keyboard_mmemu.c", 37
    lda _r
    ldx _r+1
    sta $20
    stx $21
    lda #6
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
    .loc "test_keyboard_mmemu.c", 44
    lda #127
    sta $20
    lda #1
    ldx #220
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    lda $20
    ldy #0
    sta ($24),y
    .loc "test_keyboard_mmemu.c", 46
    lda #39
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda $22
    ldx #0
    ldx #0
    sta $20
    stx $21
    .loc "test_keyboard_mmemu.c", 14
    lda $20
    ldx $21
    lsr.16 .AX
    lsr.16 .AX
    lsr.16 .AX
    sta $22
    stx $23
    lda #7
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    and.16 .AX, $24
    sta $26
    stx $27
    lda $26
    ldx $27
    sta $22
    .loc "test_keyboard_mmemu.c", 15
    lda #7
    sta $24
    lda $24
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $20
    ldx $21
    and.16 .AX, $26
    sta $24
    stx $25
    lda $24
    ldx $25
    sta $20
    .loc "test_keyboard_mmemu.c", 16
    lda $22
    ldx #0
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #__main__local_bit_table
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
    sta $26
    lda #255
    sta $22
    lda $26
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $24
    ldx $25
    eor $26
    sta $22
    stx $23
    lda #0
    ldx #220
    sta $24
    stx $25
    lda $24
    ldx $25
    sta $26
    stx $27
    lda $22
    ldy #0
    sta ($26),y
    .loc "test_keyboard_mmemu.c", 17
    lda #1
    ldx #220
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
    lda $24
    ldx $25
    sta $22
    .loc "test_keyboard_mmemu.c", 18
    lda #255
    sta $24
    lda $22
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $26
    ldx $27
    eor.16 .AX, $22
    sta $24
    stx $25
    lda $20
    ldx #0
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #__main__local_bit_table
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
    sta $26
    lda $26
    ldx #0
    ldx #0
    sta $20
    stx $21
    lda $24
    ldx $25
    and.16 .AX, $20
    sta $22
    stx $23
    lda $22
    ldx $23
    bne @tern_then18
    cmp #$00
    bne @tern_then18
    bra @tern_else19
@tern_then18:
    lda #1
    ldx #0
    sta $20
    stx $21
    bra @tern_end20
@tern_else19:
    lda #0
    sta $20
    sta $21
@tern_end20:
    lda $20
    ldx $21
    sta $22
@inline_end17:
    .loc "test_keyboard_mmemu.c", 46
    lda _r
    ldx _r+1
    sta $20
    stx $21
    lda #7
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
    .loc "test_keyboard_mmemu.c", 49
    lda #255
    sta $20
    lda #1
    ldx #220
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    lda $20
    ldy #0
    sta ($24),y
    .loc "test_keyboard_mmemu.c", 50
    lda #39
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda $22
    ldx #0
    ldx #0
    sta $20
    stx $21
    .loc "test_keyboard_mmemu.c", 14
    lda $20
    ldx $21
    lsr.16 .AX
    lsr.16 .AX
    lsr.16 .AX
    sta $22
    stx $23
    lda #7
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    and.16 .AX, $24
    sta $26
    stx $27
    lda $26
    ldx $27
    sta $22
    .loc "test_keyboard_mmemu.c", 15
    lda #7
    sta $24
    lda $24
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $20
    ldx $21
    and.16 .AX, $26
    sta $24
    stx $25
    lda $24
    ldx $25
    sta $20
    .loc "test_keyboard_mmemu.c", 16
    lda $22
    ldx #0
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #__main__local_bit_table
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
    sta $26
    lda #255
    sta $22
    lda $26
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $24
    ldx $25
    eor $26
    sta $22
    stx $23
    lda #0
    ldx #220
    sta $24
    stx $25
    lda $24
    ldx $25
    sta $26
    stx $27
    lda $22
    ldy #0
    sta ($26),y
    .loc "test_keyboard_mmemu.c", 17
    lda #1
    ldx #220
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
    lda $24
    ldx $25
    sta $22
    .loc "test_keyboard_mmemu.c", 18
    lda #255
    sta $24
    lda $22
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $26
    ldx $27
    eor.16 .AX, $22
    sta $24
    stx $25
    lda $20
    ldx #0
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #__main__local_bit_table
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
    sta $26
    lda $26
    ldx #0
    ldx #0
    sta $20
    stx $21
    lda $24
    ldx $25
    and.16 .AX, $20
    sta $22
    stx $23
    lda $22
    ldx $23
    bne @tern_then23
    cmp #$00
    bne @tern_then23
    bra @tern_else24
@tern_then23:
    lda #1
    ldx #0
    sta $20
    stx $21
    bra @tern_end25
@tern_else24:
    lda #0
    sta $20
    sta $21
@tern_end25:
    lda $20
    ldx $21
    sta $22
@inline_end22:
    .loc "test_keyboard_mmemu.c", 50
    lda _r
    ldx _r+1
    sta $20
    stx $21
    lda #8
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
    .loc "test_keyboard_mmemu.c", 54
    lda #253
    sta $20
    lda #1
    ldx #220
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    lda $20
    ldy #0
    sta ($24),y
    .loc "test_keyboard_mmemu.c", 56
    lda #17
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda $22
    ldx #0
    ldx #0
    sta $20
    stx $21
    .loc "test_keyboard_mmemu.c", 14
    lda $20
    ldx $21
    lsr.16 .AX
    lsr.16 .AX
    lsr.16 .AX
    sta $22
    stx $23
    lda #7
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    and.16 .AX, $24
    sta $26
    stx $27
    lda $26
    ldx $27
    sta $22
    .loc "test_keyboard_mmemu.c", 15
    lda #7
    sta $24
    lda $24
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $20
    ldx $21
    and.16 .AX, $26
    sta $24
    stx $25
    lda $24
    ldx $25
    sta $20
    .loc "test_keyboard_mmemu.c", 16
    lda $22
    ldx #0
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #__main__local_bit_table
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
    sta $26
    lda #255
    sta $22
    lda $26
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $24
    ldx $25
    eor $26
    sta $22
    stx $23
    lda #0
    ldx #220
    sta $24
    stx $25
    lda $24
    ldx $25
    sta $26
    stx $27
    lda $22
    ldy #0
    sta ($26),y
    .loc "test_keyboard_mmemu.c", 17
    lda #1
    ldx #220
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
    lda $24
    ldx $25
    sta $22
    .loc "test_keyboard_mmemu.c", 18
    lda #255
    sta $24
    lda $22
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $26
    ldx $27
    eor.16 .AX, $22
    sta $24
    stx $25
    lda $20
    ldx #0
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #__main__local_bit_table
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
    sta $26
    lda $26
    ldx #0
    ldx #0
    sta $20
    stx $21
    lda $24
    ldx $25
    and.16 .AX, $20
    sta $22
    stx $23
    lda $22
    ldx $23
    bne @tern_then28
    cmp #$00
    bne @tern_then28
    bra @tern_else29
@tern_then28:
    lda #1
    ldx #0
    sta $20
    stx $21
    bra @tern_end30
@tern_else29:
    lda #0
    sta $20
    sta $21
@tern_end30:
    lda $20
    ldx $21
    sta $22
@inline_end27:
    .loc "test_keyboard_mmemu.c", 56
    lda _r
    ldx _r+1
    sta $20
    stx $21
    lda #9
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
    .loc "test_keyboard_mmemu.c", 57
    lda #39
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda $22
    ldx #0
    ldx #0
    sta $20
    stx $21
    .loc "test_keyboard_mmemu.c", 14
    lda $20
    ldx $21
    lsr.16 .AX
    lsr.16 .AX
    lsr.16 .AX
    sta $22
    stx $23
    lda #7
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    and.16 .AX, $24
    sta $26
    stx $27
    lda $26
    ldx $27
    sta _main__local_241
    .loc "test_keyboard_mmemu.c", 15
    lda #7
    sta $22
    lda $22
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx $21
    and.16 .AX, $24
    sta $22
    stx $23
    lda $22
    ldx $23
    sta _main__local_246
    .loc "test_keyboard_mmemu.c", 16
    lda _main__local_241
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #__main__local_bit_table
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
    sta $22
    lda #255
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
    eor $22
    sta $20
    stx $21
    lda #0
    ldx #220
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    lda $20
    ldy #0
    sta ($24),y
    .loc "test_keyboard_mmemu.c", 17
    lda #1
    ldx #220
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
    sta _main__local_260
    .loc "test_keyboard_mmemu.c", 18
    lda #255
    sta $20
    lda _main__local_260
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
    eor.16 .AX, $24
    sta $20
    stx $21
    lda _main__local_246
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #__main__local_bit_table
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
    sta $24
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    and.16 .AX, $22
    sta $24
    stx $25
    lda $24
    ldx $25
    bne @tern_then33
    cmp #$00
    bne @tern_then33
    bra @tern_else34
@tern_then33:
    lda #1
    ldx #0
    sta $20
    stx $21
    bra @tern_end35
@tern_else34:
    lda #0
    sta $20
    sta $21
@tern_end35:
    lda $20
    ldx $21
    sta $22
@inline_end32:
    .loc "test_keyboard_mmemu.c", 57
    lda _r
    ldx _r+1
    sta $20
    stx $21
    lda #10
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
    .loc "test_keyboard_mmemu.c", 59
    brk
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 36
    endproc


__zp_save_buf:

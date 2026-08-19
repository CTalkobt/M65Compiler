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

    .segment "data"
    .byte 0
_r:
; .debug_var: @global _r offset=0 size=2 type=ptr scope=global
    .word 16384

    .segment "code"

; function _main
; SAC inline storage: 0 bytes
    _main__local_7: .word 0
    _main__local_73: .word 0
    _main__local_161: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/mega65.h", 5

@entry:
    .loc "test_sid_cia_mmemu.c", 11
    lda #12
    sta $20
    lda #0
    ldx #212
    sta $22
    stx $23
    lda #0
    sta $24
    sta $25
    lda #7
    ldx #0
    sta $26
    stx $27
    lda $24
    ldx $25
    mul.16 .AX, $26
    sta $28
    stx $29
    lda $22
    clc
    adc $28
    sta $24
    lda $23
    adc $28+1
    sta $25
    lda $24
    ldx $25
    sta $22
    stx $23
    lda $20
    ldy #0
    sta ($22),y
    .loc "test_sid_cia_mmemu.c", 12
    lda #17
    sta $20
    lda #0
    ldx #212
    sta $22
    stx $23
    lda #0
    sta $24
    sta $25
    lda #7
    ldx #0
    sta $26
    stx $27
    lda $24
    ldx $25
    mul.16 .AX, $26
    sta $28
    stx $29
    lda $22
    clc
    adc $28
    sta $24
    lda $23
    adc $28+1
    sta $25
    lda $24
    ldx $25
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
    .loc "test_sid_cia_mmemu.c", 13
    lda #17
    sta $20
    lda #0
    ldx #212
    sta $22
    stx $23
    lda #0
    sta $24
    sta $25
    lda #7
    ldx #0
    sta $26
    stx $27
    lda $24
    ldx $25
    mul.16 .AX, $26
    sta $28
    stx $29
    lda $22
    clc
    adc $28
    sta $24
    lda $23
    adc $28+1
    sta $25
    lda $24
    ldx $25
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #4
    sta $24
    stx $25
    lda $20
    ldy #0
    sta ($24),y
    .loc "test_sid_cia_mmemu.c", 14
    lda #9
    sta $20
    lda #0
    ldx #212
    sta $22
    stx $23
    lda #0
    sta $24
    sta $25
    lda #7
    ldx #0
    sta $26
    stx $27
    lda $24
    ldx $25
    mul.16 .AX, $26
    sta $28
    stx $29
    lda $22
    clc
    adc $28
    sta $24
    lda $23
    adc $28+1
    sta $25
    lda $24
    ldx $25
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #5
    sta $24
    stx $25
    lda $20
    ldy #0
    sta ($24),y
    .loc "test_sid_cia_mmemu.c", 17
    lda #15
    sta $20
    lda #0
    ldx #212
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    lda $24
    ldx $25
    add.16 .AX, #24
    sta $22
    stx $23
    lda $20
    ldy #0
    sta ($22),y
    .loc "test_sid_cia_mmemu.c", 19
    lda #0
    ldx #212
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
    .loc "test_sid_cia_mmemu.c", 20
    lda #1
    ldx #212
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
    .loc "test_sid_cia_mmemu.c", 21
    lda #4
    ldx #212
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
    .loc "test_sid_cia_mmemu.c", 22
    lda #5
    ldx #212
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
    .loc "test_sid_cia_mmemu.c", 23
    lda #24
    ldx #212
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
    .loc "test_sid_cia_mmemu.c", 26
    lda #170
    sta $20
    lda #0
    ldx #212
    sta $22
    stx $23
    lda #1
    ldx #0
    sta $24
    stx $25
    lda #7
    ldx #0
    sta $26
    stx $27
    lda $24
    ldx $25
    mul.16 .AX, $26
    sta $28
    stx $29
    lda $22
    clc
    adc $28
    sta $24
    lda $23
    adc $28+1
    sta $25
    lda $24
    ldx $25
    sta $22
    stx $23
    lda $20
    ldy #0
    sta ($22),y
    .loc "test_sid_cia_mmemu.c", 27
    lda #187
    sta $20
    lda #0
    ldx #212
    sta $22
    stx $23
    lda #1
    ldx #0
    sta $24
    stx $25
    lda #7
    ldx #0
    sta $26
    stx $27
    lda $24
    ldx $25
    mul.16 .AX, $26
    sta $28
    stx $29
    lda $22
    clc
    adc $28
    sta $24
    lda $23
    adc $28+1
    sta $25
    lda $24
    ldx $25
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
    .loc "test_sid_cia_mmemu.c", 29
    lda #7
    ldx #212
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
    .loc "test_sid_cia_mmemu.c", 30
    lda #8
    ldx #212
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
    .loc "test_sid_cia_mmemu.c", 33
    lda #128
    sta $20
    lda #0
    ldx #212
    sta $22
    stx $23
    lda #2
    ldx #0
    sta $24
    stx $25
    lda #7
    ldx #0
    sta $26
    stx $27
    lda $24
    ldx $25
    mul.16 .AX, $26
    sta $28
    stx $29
    lda $22
    clc
    adc $28
    sta $24
    lda $23
    adc $28+1
    sta $25
    lda $24
    ldx $25
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #4
    sta $24
    stx $25
    lda $20
    ldy #0
    sta ($24),y
    .loc "test_sid_cia_mmemu.c", 35
    lda #18
    ldx #212
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
    .loc "test_sid_cia_mmemu.c", 38
    lda #5
    sta $20
    lda #32
    ldx #212
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    lda $24
    ldx $25
    add.16 .AX, #24
    sta $22
    stx $23
    lda $20
    ldy #0
    sta ($22),y
    .loc "test_sid_cia_mmemu.c", 39
    lda #56
    ldx #212
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
    .loc "test_sid_cia_mmemu.c", 42
    lda #255
    sta $20
    lda #0
    ldx #220
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    lda $24
    ldx $25
    add.16 .AX, #2
    sta $22
    stx $23
    lda $20
    ldy #0
    sta ($22),y
    .loc "test_sid_cia_mmemu.c", 43
    lda #232
    sta $20
    lda #0
    ldx #220
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    lda $24
    ldx $25
    add.16 .AX, #4
    sta $22
    stx $23
    lda $20
    ldy #0
    sta ($22),y
    .loc "test_sid_cia_mmemu.c", 44
    lda #3
    sta $20
    lda #0
    ldx #220
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    lda $24
    ldx $25
    add.16 .AX, #5
    sta $22
    stx $23
    lda $20
    ldy #0
    sta ($22),y
    .loc "test_sid_cia_mmemu.c", 45
    lda #9
    sta $20
    lda #0
    ldx #220
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    lda $24
    ldx $25
    add.16 .AX, #14
    sta $22
    stx $23
    lda $20
    ldy #0
    sta ($22),y
    .loc "test_sid_cia_mmemu.c", 47
    lda #2
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
    .loc "test_sid_cia_mmemu.c", 48
    lda #4
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
    .loc "test_sid_cia_mmemu.c", 49
    lda #5
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
    .loc "test_sid_cia_mmemu.c", 50
    lda #14
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
    sta $20
    lda _r
    ldx _r+1
    sta $22
    stx $23
    lda #12
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
    .loc "test_sid_cia_mmemu.c", 53
    lda #3
    sta $20
    lda #0
    ldx #221
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    lda $20
    ldy #0
    sta ($24),y
    .loc "test_sid_cia_mmemu.c", 54
    lda #85
    sta $20
    lda #0
    ldx #221
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    lda $24
    ldx $25
    add.16 .AX, #3
    sta $22
    stx $23
    lda $20
    ldy #0
    sta ($22),y
    .loc "test_sid_cia_mmemu.c", 56
    lda #0
    ldx #221
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
    sta $20
    lda _r
    ldx _r+1
    sta $22
    stx $23
    lda #13
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
    .loc "test_sid_cia_mmemu.c", 57
    lda #3
    ldx #221
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
    sta $20
    lda _r
    ldx _r+1
    sta $22
    stx $23
    lda #14
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
    .loc "test_sid_cia_mmemu.c", 60
    lda #17
    sta $20
    lda _r
    ldx _r+1
    sta $22
    stx $23
    lda #15
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
    .loc "test_sid_cia_mmemu.c", 61
    lda #17
    sta $20
    lda _r
    ldx _r+1
    sta $22
    stx $23
    lda #16
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
    .loc "test_sid_cia_mmemu.c", 62
    lda #1
    sta $20
    lda _r
    ldx _r+1
    sta $22
    stx $23
    lda #17
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
    .loc "test_sid_cia_mmemu.c", 64
    brk
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 0
    endproc


__zp_save_buf:

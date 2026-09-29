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
    _main__local_93: .word 0
    _main__local_123: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/mega65.h", 5

@entry:
    .loc "test_devices_mmemu.c", 11
    lda #10
    sta $20
    lda #128
    ldx #208
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
    .loc "test_devices_mmemu.c", 12
    lda #5
    sta $20
    lda #128
    ldx #208
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
    .loc "test_devices_mmemu.c", 13
    lda #1
    sta $20
    lda #128
    ldx #208
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    lda $24
    ldx $25
    add.16 .AX, #6
    sta $22
    stx $23
    lda $20
    ldy #0
    sta ($22),y
    .loc "test_devices_mmemu.c", 14
    lda #204
    sta $20
    lda #128
    ldx #208
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    lda $24
    ldx $25
    add.16 .AX, #7
    sta $22
    stx $23
    lda $20
    ldy #0
    sta ($22),y
    .loc "test_devices_mmemu.c", 16
    lda #132
    ldx #208
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
    .loc "test_devices_mmemu.c", 17
    lda #133
    ldx #208
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
    .loc "test_devices_mmemu.c", 18
    lda #134
    ldx #208
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
    .loc "test_devices_mmemu.c", 19
    lda #135
    ldx #208
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
    .loc "test_devices_mmemu.c", 22
    lda #16
    sta $20
    lda #128
    ldx #214
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
    .loc "test_devices_mmemu.c", 23
    lda #32
    sta $20
    lda #128
    ldx #214
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
    .loc "test_devices_mmemu.c", 24
    lda #48
    sta $20
    lda #128
    ldx #214
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
    .loc "test_devices_mmemu.c", 25
    lda #64
    sta $20
    lda #128
    ldx #214
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
    .loc "test_devices_mmemu.c", 26
    lda #1
    sta $20
    lda #128
    ldx #214
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    lda $24
    ldx $25
    add.16 .AX, #1
    sta $22
    stx $23
    lda $20
    ldy #0
    sta ($22),y
    .loc "test_devices_mmemu.c", 28
    lda #130
    ldx #214
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
    .loc "test_devices_mmemu.c", 29
    lda #131
    ldx #214
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
    .loc "test_devices_mmemu.c", 30
    lda #132
    ldx #214
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
    .loc "test_devices_mmemu.c", 31
    lda #133
    ldx #214
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
    .loc "test_devices_mmemu.c", 32
    lda #129
    ldx #214
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
    .loc "test_devices_mmemu.c", 35
    lda #6
    sta $20
    lda #224
    ldx #214
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    lda $20
    ldy #0
    sta ($24),y
    .loc "test_devices_mmemu.c", 36
    lda #90
    sta $20
    lda #224
    ldx #214
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    lda $24
    ldx $25
    add.16 .AX, #1
    sta $22
    stx $23
    lda $20
    ldy #0
    sta ($22),y
    .loc "test_devices_mmemu.c", 37
    lda #0
    sta $20
    lda #224
    ldx #214
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
    .loc "test_devices_mmemu.c", 39
    lda #224
    ldx #214
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
    .loc "test_devices_mmemu.c", 40
    lda #225
    ldx #214
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
    .loc "test_devices_mmemu.c", 41
    lda #226
    ldx #214
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
    .loc "test_devices_mmemu.c", 44
    lda #66
    sta $20
    lda #64
    ldx #214
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    lda #0
    sta $22
    sta $23
    lda $20
    ldx #0
    pha
    lda $22
    ldx $23
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $24
    ldx $24+1
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
    .loc "test_devices_mmemu.c", 46
    lda #64
    ldx #214
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
    .loc "test_devices_mmemu.c", 49
    lda #64
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
    .loc "test_devices_mmemu.c", 50
    lda #1
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
    .loc "test_devices_mmemu.c", 51
    lda #6
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
    .loc "test_devices_mmemu.c", 53
    brk
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 0
    endproc


__zp_save_buf:

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
; SAC zero-alloc leaf: no storage overhead
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/mega65.h", 5

@entry:
    .loc "test_vic4_mmemu.c", 11
    lda #170
    sta $20
    lda #0
    ldx #208
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    lda $24
    ldx $25
    add.16 .AX, #32
    sta $22
    stx $23
    lda $20
    ldy #0
    sta ($22),y
    .loc "test_vic4_mmemu.c", 12
    lda #187
    sta $20
    lda #0
    ldx #208
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    lda $24
    ldx $25
    add.16 .AX, #33
    sta $22
    stx $23
    lda $20
    ldy #0
    sta ($22),y
    .loc "test_vic4_mmemu.c", 13
    lda #204
    sta $20
    lda #0
    ldx #208
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    lda $24
    ldx $25
    add.16 .AX, #34
    sta $22
    stx $23
    lda $20
    ldy #0
    sta ($22),y
    .loc "test_vic4_mmemu.c", 14
    lda #221
    sta $20
    lda #0
    ldx #208
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    lda $24
    ldx $25
    add.16 .AX, #35
    sta $22
    stx $23
    lda $20
    ldy #0
    sta ($22),y
    .loc "test_vic4_mmemu.c", 15
    lda #15
    sta $20
    lda #0
    ldx #208
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    lda $24
    ldx $25
    add.16 .AX, #21
    sta $22
    stx $23
    lda $20
    ldy #0
    sta ($22),y
    .loc "test_vic4_mmemu.c", 18
    lda #32
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
    .loc "test_vic4_mmemu.c", 19
    lda #33
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
    .loc "test_vic4_mmemu.c", 20
    lda #34
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
    .loc "test_vic4_mmemu.c", 21
    lda #35
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
    .loc "test_vic4_mmemu.c", 22
    lda #21
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
    .loc "test_vic4_mmemu.c", 25
    lda #17
    sta $20
    lda #32
    ldx #208
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    lda $20
    ldy #0
    sta ($24),y
    .loc "test_vic4_mmemu.c", 26
    lda #34
    sta $20
    lda #33
    ldx #208
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    lda $20
    ldy #0
    sta ($24),y
    .loc "test_vic4_mmemu.c", 28
    lda #32
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
    .loc "test_vic4_mmemu.c", 29
    lda #33
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
    .loc "test_vic4_mmemu.c", 33
    lda #225
    sta $20
    lda #32
    ldx #208
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    lda $20
    ldy #0
    sta ($24),y
    .loc "test_vic4_mmemu.c", 34
    lda #226
    sta $20
    lda #33
    ldx #208
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    lda $20
    ldy #0
    sta ($24),y
    .loc "test_vic4_mmemu.c", 35
    lda #227
    sta $20
    lda #34
    ldx #208
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    lda $20
    ldy #0
    sta ($24),y
    .loc "test_vic4_mmemu.c", 38
    lda #255
    sta $20
    lda #33
    ldx #208
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    lda $20
    ldy #0
    sta ($24),y
    .loc "test_vic4_mmemu.c", 40
    lda #32
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
    .loc "test_vic4_mmemu.c", 41
    lda #33
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
    .loc "test_vic4_mmemu.c", 42
    lda #34
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
    .loc "test_vic4_mmemu.c", 45
    lda #193
    sta $20
    lda #0
    ldx #208
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    lda $24
    ldx $25
    add.16 .AX, #32
    sta $22
    stx $23
    lda $20
    ldy #0
    sta ($22),y
    .loc "test_vic4_mmemu.c", 46
    lda #194
    sta $20
    lda #0
    ldx #208
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    lda $24
    ldx $25
    add.16 .AX, #33
    sta $22
    stx $23
    lda $20
    ldy #0
    sta ($22),y
    .loc "test_vic4_mmemu.c", 47
    lda #195
    sta $20
    lda #0
    ldx #208
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    lda $24
    ldx $25
    add.16 .AX, #34
    sta $22
    stx $23
    lda $20
    ldy #0
    sta ($22),y
    .loc "test_vic4_mmemu.c", 50
    lda #153
    sta $20
    lda #0
    ldx #208
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    lda $24
    ldx $25
    add.16 .AX, #33
    sta $22
    stx $23
    lda $20
    ldy #0
    sta ($22),y
    .loc "test_vic4_mmemu.c", 52
    lda #32
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
    .loc "test_vic4_mmemu.c", 53
    lda #33
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
    .loc "test_vic4_mmemu.c", 54
    lda #34
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
    .loc "test_vic4_mmemu.c", 57
    lda #0
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
    .loc "test_vic4_mmemu.c", 58
    lda #6
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
    .loc "test_vic4_mmemu.c", 59
    lda #15
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
    .loc "test_vic4_mmemu.c", 60
    lda #16
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
    .loc "test_vic4_mmemu.c", 61
    lda #64
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
    .loc "test_vic4_mmemu.c", 62
    lda #4
    sta $20
    lda _r
    ldx _r+1
    sta $22
    stx $23
    lda #18
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
    .loc "test_vic4_mmemu.c", 64
    brk
@__return:
    rts
    .func_flags stack_call, static_alloc, zeroalloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 0
    endproc


__zp_save_buf:

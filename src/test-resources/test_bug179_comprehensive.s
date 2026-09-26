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

    .extern _mktime

    .global _result_addr
    .global _main

    .segment "data"
    .byte 0
_result_addr:
; .debug_var: @global _result_addr offset=0 size=2 type=ptr scope=global
    .word 16384

    .segment "code"

; function _main
; SAC inline storage: 100 bytes
    _main__local_0: .word 0
    _main__local_1: .long 0
    _main__local_2: .long 0
    _main__local_3: .long 0
    _main__local_4: .word 0
    _main__local_24: .word 0
    _main__local_34: .word 0
    _main__local_39: .word 0
    _main__local_64: .word 0
    _main__local_65: .word 0
    _main__local_70: .word 0
    _main__local_95: .word 0
    _main__local_96: .word 0
    _main__local_101: .word 0
    _main__local_108: .word 0
    _main__local_112: .word 0
    _main__local_120: .word 0
    _main__local_138: .word 0
    _main__local_148: .long 0
    _main__local_149: .word 0
    _main__local_154: .word 0
    _main__local_162: .word 0
    _main__local_180: .word 0
    _main__local_190: .long 0
    _main__local_191: .word 0
    _main__local_193: .word 0
    _main__local_211: .word 0
    _main__local_221: .long 0
    _main__local_222: .word 0
    _main__local_233: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/time.h", 5
    .local @_l_different = 2
    .local @_l_r1 = 20
    .local @_l_r2 = 24
    .local @_l_result1 = 4
    .local @_l_result2 = 8
    .local @_l_result3 = 12
    .local @_l_result4 = 16
    .local @_l_t = 28
    .local @_l_t2 = 46
    .local @_l_t3 = 64
    .local @_l_t4 = 82
    .local @_l_test_num = 0
; .debug_var: __main @_l_different offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_r1 offset=20 size=4 type=int32 scope=local
; .debug_var: __main @_l_r2 offset=24 size=4 type=int32 scope=local
; .debug_var: __main @_l_result1 offset=4 size=4 type=int32 scope=local
; .debug_var: __main @_l_result2 offset=8 size=4 type=int32 scope=local
; .debug_var: __main @_l_result3 offset=12 size=4 type=int32 scope=local
; .debug_var: __main @_l_result4 offset=16 size=4 type=int32 scope=local
; .debug_var: __main @_l_t offset=28 size=2 type=int16 scope=local
; .debug_var: __main @_l_t2 offset=46 size=2 type=int16 scope=local
; .debug_var: __main @_l_t3 offset=64 size=2 type=int16 scope=local
; .debug_var: __main @_l_t4 offset=82 size=2 type=int16 scope=local
; .debug_var: __main @_l_test_num offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_bug179_comprehensive.c", 10
    .loc "test_bug179_comprehensive.c", 13
    .loc "test_bug179_comprehensive.c", 14
    lda #126
    ldx #0
    sta $20
    stx $21
    leax.local 28
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #10
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_bug179_comprehensive.c", 15
    lda #6
    ldx #0
    sta $20
    stx $21
    leax.local 28
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #8
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_bug179_comprehensive.c", 16
    lda #3
    ldx #0
    sta $20
    stx $21
    leax.local 28
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #6
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_bug179_comprehensive.c", 17
    lda #12
    ldx #0
    sta $20
    stx $21
    leax.local 28
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #4
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_bug179_comprehensive.c", 18
    lda #0
    sta $20
    sta $21
    leax.local 28
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #2
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_bug179_comprehensive.c", 19
    lda #0
    sta $20
    sta $21
    leax.local 28
    sta $22
    stx $23
    lda $20
    ldx $21
    ldy #0
    sta ($22),y
    txa
    iny
    sta ($22),y
    .loc "test_bug179_comprehensive.c", 20
    lda #0
    sta $20
    sta $21
    leax.local 28
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #12
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_bug179_comprehensive.c", 21
    lda #0
    sta $20
    sta $21
    leax.local 28
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #14
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_bug179_comprehensive.c", 22
    lda #0
    sta $20
    sta $21
    leax.local 28
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #16
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_bug179_comprehensive.c", 23
    leax.local 28
    sta _main__local_34
    stx _main__local_34+1
    lda _main__local_34
    ldx _main__local_34+1
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    jsr _mktime
    sta __zp_scratch4
    stx __zp_scratch4+1
    sty __zp_scratch4+2
    stz __zp_scratch4+3
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    ldy __zp_scratch4+2
    ldz __zp_scratch4+3
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    sta _main__local_1
    stx _main__local_1+1
    sty _main__local_1+2
    stz _main__local_1+3
    .loc "test_bug179_comprehensive.c", 24
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    ldy #0
    ldz #0
    sta $22
    stx $23
    sty $24
    stz $25
    lda _main__local_1
    ldx _main__local_1+1
    ldy _main__local_1+2
    ldz _main__local_1+3
    cmp.32 .AXYZ, $22
    bne @tern_then0
    bra @tern_else1
@tern_then0:
    lda #170
    ldx #0
    sta $20
    stx $21
    bra @tern_end2
@tern_else1:
    lda #1
    ldx #0
    sta $20
    stx $21
@tern_end2:
    lda $20
    ldx $21
    sta $22
    lda _result_addr
    ldx _result_addr+1
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
    .loc "test_bug179_comprehensive.c", 27
    .loc "test_bug179_comprehensive.c", 28
    lda #125
    ldx #0
    sta $20
    stx $21
    leax.local 28
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #10
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_bug179_comprehensive.c", 29
    lda #0
    sta $20
    sta $21
    leax.local 28
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #8
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_bug179_comprehensive.c", 30
    lda #1
    ldx #0
    sta $20
    stx $21
    leax.local 28
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #6
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_bug179_comprehensive.c", 31
    lda #0
    sta $20
    sta $21
    leax.local 28
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #4
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_bug179_comprehensive.c", 32
    lda #0
    sta $20
    sta $21
    leax.local 28
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #2
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_bug179_comprehensive.c", 33
    lda #0
    sta $20
    sta $21
    leax.local 28
    sta $22
    stx $23
    lda $20
    ldx $21
    ldy #0
    sta ($22),y
    txa
    iny
    sta ($22),y
    .loc "test_bug179_comprehensive.c", 34
    leax.local 28
    sta _main__local_65
    stx _main__local_65+1
    lda _main__local_65
    ldx _main__local_65+1
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    jsr _mktime
    sta __zp_scratch4
    stx __zp_scratch4+1
    sty __zp_scratch4+2
    stz __zp_scratch4+3
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    ldy __zp_scratch4+2
    ldz __zp_scratch4+3
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    sta _main__local_2
    stx _main__local_2+1
    sty _main__local_2+2
    stz _main__local_2+3
    .loc "test_bug179_comprehensive.c", 35
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    ldy #0
    ldz #0
    sta $22
    stx $23
    sty $24
    stz $25
    lda _main__local_2
    ldx _main__local_2+1
    ldy _main__local_2+2
    ldz _main__local_2+3
    cmp.32 .AXYZ, $22
    bne @tern_then3
    bra @tern_else4
@tern_then3:
    lda #187
    ldx #0
    sta $20
    stx $21
    bra @tern_end5
@tern_else4:
    lda #2
    ldx #0
    sta $20
    stx $21
@tern_end5:
    lda $20
    ldx $21
    sta $22
    lda _result_addr
    ldx _result_addr+1
    sta $20
    stx $21
    lda #1
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
    .loc "test_bug179_comprehensive.c", 38
    .loc "test_bug179_comprehensive.c", 39
    lda #124
    ldx #0
    sta $20
    stx $21
    leax.local 28
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #10
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_bug179_comprehensive.c", 40
    lda #11
    ldx #0
    sta $20
    stx $21
    leax.local 28
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #8
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_bug179_comprehensive.c", 41
    lda #31
    ldx #0
    sta $20
    stx $21
    leax.local 28
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #6
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_bug179_comprehensive.c", 42
    lda #23
    ldx #0
    sta $20
    stx $21
    leax.local 28
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #4
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_bug179_comprehensive.c", 43
    lda #59
    ldx #0
    sta $20
    stx $21
    leax.local 28
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #2
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_bug179_comprehensive.c", 44
    lda #59
    ldx #0
    sta $20
    stx $21
    leax.local 28
    sta $22
    stx $23
    lda $20
    ldx $21
    ldy #0
    sta ($22),y
    txa
    iny
    sta ($22),y
    .loc "test_bug179_comprehensive.c", 45
    leax.local 28
    sta _main__local_96
    stx _main__local_96+1
    lda _main__local_96
    ldx _main__local_96+1
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    jsr _mktime
    sta __zp_scratch4
    stx __zp_scratch4+1
    sty __zp_scratch4+2
    stz __zp_scratch4+3
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    ldy __zp_scratch4+2
    ldz __zp_scratch4+3
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    sta _main__local_3
    stx _main__local_3+1
    sty _main__local_3+2
    stz _main__local_3+3
    .loc "test_bug179_comprehensive.c", 46
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    ldy #0
    ldz #0
    sta $22
    stx $23
    sty $24
    stz $25
    lda _main__local_3
    ldx _main__local_3+1
    ldy _main__local_3+2
    ldz _main__local_3+3
    cmp.32 .AXYZ, $22
    bne @tern_then6
    bra @tern_else7
@tern_then6:
    lda #204
    ldx #0
    sta $20
    stx $21
    bra @tern_end8
@tern_else7:
    lda #3
    ldx #0
    sta $20
    stx $21
@tern_end8:
    lda $20
    ldx $21
    sta $22
    lda _result_addr
    ldx _result_addr+1
    sta $20
    stx $21
    lda #2
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
    .loc "test_bug179_comprehensive.c", 49
    lda _main__local_2
    ldx _main__local_2+1
    ldy _main__local_2+2
    ldz _main__local_2+3
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_1
    ldx _main__local_1+1
    ldy _main__local_1+2
    ldz _main__local_1+3
    cmp.32 .AXYZ, __zp_scratch2
    bne @sc_merge9
    bra @sc_short10
@sc_merge9:
    lda _main__local_3
    ldx _main__local_3+1
    ldy _main__local_3+2
    ldz _main__local_3+3
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_2
    ldx _main__local_2+1
    ldy _main__local_2+2
    ldz _main__local_2+3
    cmp.32 .AXYZ, __zp_scratch2
    bne @__cmp_set_0
    bra @__cmp_zero_0
@__cmp_set_0:
    lda #1
    ldx #0
    bra @__cmp_done_0
@__cmp_zero_0:
    lda #0
    ldx #0
@__cmp_done_0:
    sta $20
    bra @sc_done11
@sc_short10:
@sc_done11:
    lda $20
    ldx #0
    sta _main__local_108
    stx _main__local_108+1
    .loc "test_bug179_comprehensive.c", 50
    lda _main__local_108
    ldx _main__local_108+1
    bne @tern_then12
    cmp #$00
    bne @tern_then12
    bra @tern_else13
@tern_then12:
    lda #221
    ldx #0
    sta $20
    stx $21
    bra @tern_end14
@tern_else13:
    lda #4
    ldx #0
    sta $20
    stx $21
@tern_end14:
    lda $20
    ldx $21
    sta $22
    lda _result_addr
    ldx _result_addr+1
    sta $20
    stx $21
    lda #3
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
    .loc "test_bug179_comprehensive.c", 53
    .loc "test_bug179_comprehensive.c", 55
    lda #126
    ldx #0
    sta $20
    stx $21
    leax.local 46
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #10
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_bug179_comprehensive.c", 56
    lda #6
    ldx #0
    sta $20
    stx $21
    leax.local 46
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #8
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_bug179_comprehensive.c", 57
    lda #3
    ldx #0
    sta $20
    stx $21
    leax.local 46
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #6
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_bug179_comprehensive.c", 58
    lda #12
    ldx #0
    sta $20
    stx $21
    leax.local 46
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #4
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_bug179_comprehensive.c", 59
    lda #30
    ldx #0
    sta $20
    stx $21
    leax.local 46
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #2
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_bug179_comprehensive.c", 60
    lda #45
    ldx #0
    sta $20
    stx $21
    leax.local 46
    sta $22
    stx $23
    lda $20
    ldx $21
    ldy #0
    sta ($22),y
    txa
    iny
    sta ($22),y
    .loc "test_bug179_comprehensive.c", 61
    lda #0
    sta $20
    sta $21
    leax.local 46
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #12
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_bug179_comprehensive.c", 62
    lda #0
    sta $20
    sta $21
    leax.local 46
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #14
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_bug179_comprehensive.c", 63
    lda #0
    sta $20
    sta $21
    leax.local 46
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #16
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_bug179_comprehensive.c", 64
    leax.local 46
    sta _main__local_149
    stx _main__local_149+1
    lda _main__local_149
    ldx _main__local_149+1
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    jsr _mktime
    sta __zp_scratch4
    stx __zp_scratch4+1
    sty __zp_scratch4+2
    stz __zp_scratch4+3
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    ldy __zp_scratch4+2
    ldz __zp_scratch4+3
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    sta _main__local_148
    stx _main__local_148+1
    sty _main__local_148+2
    stz _main__local_148+3
    .loc "test_bug179_comprehensive.c", 65
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    ldy #0
    ldz #0
    sta $22
    stx $23
    sty $24
    stz $25
    lda _main__local_148
    ldx _main__local_148+1
    ldy _main__local_148+2
    ldz _main__local_148+3
    cmp.32 .AXYZ, $22
    bne @tern_then15
    bra @tern_else16
@tern_then15:
    lda #238
    ldx #0
    sta $20
    stx $21
    bra @tern_end17
@tern_else16:
    lda #5
    ldx #0
    sta $20
    stx $21
@tern_end17:
    lda $20
    ldx $21
    sta $22
    lda _result_addr
    ldx _result_addr+1
    sta $20
    stx $21
    lda #4
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
    .loc "test_bug179_comprehensive.c", 68
    .loc "test_bug179_comprehensive.c", 70
    lda #126
    ldx #0
    sta $20
    stx $21
    leax.local 64
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #10
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_bug179_comprehensive.c", 71
    lda #0
    sta $20
    sta $21
    leax.local 64
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #8
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_bug179_comprehensive.c", 72
    lda #1
    ldx #0
    sta $20
    stx $21
    leax.local 64
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #6
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_bug179_comprehensive.c", 73
    lda #0
    sta $20
    sta $21
    leax.local 64
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #4
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_bug179_comprehensive.c", 74
    lda #0
    sta $20
    sta $21
    leax.local 64
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #2
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_bug179_comprehensive.c", 75
    lda #0
    sta $20
    sta $21
    leax.local 64
    sta $22
    stx $23
    lda $20
    ldx $21
    ldy #0
    sta ($22),y
    txa
    iny
    sta ($22),y
    .loc "test_bug179_comprehensive.c", 76
    lda #0
    sta $20
    sta $21
    leax.local 64
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #12
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_bug179_comprehensive.c", 77
    lda #0
    sta $20
    sta $21
    leax.local 64
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #14
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_bug179_comprehensive.c", 78
    lda #0
    sta $20
    sta $21
    leax.local 64
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #16
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_bug179_comprehensive.c", 79
    leax.local 64
    sta _main__local_191
    stx _main__local_191+1
    lda _main__local_191
    ldx _main__local_191+1
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    jsr _mktime
    sta __zp_scratch4
    stx __zp_scratch4+1
    sty __zp_scratch4+2
    stz __zp_scratch4+3
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    ldy __zp_scratch4+2
    ldz __zp_scratch4+3
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    sta _main__local_190
    stx _main__local_190+1
    sty _main__local_190+2
    stz _main__local_190+3
    .loc "test_bug179_comprehensive.c", 82
    lda #126
    ldx #0
    sta $20
    stx $21
    leax.local 82
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #10
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_bug179_comprehensive.c", 83
    lda #0
    sta $20
    sta $21
    leax.local 82
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #8
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_bug179_comprehensive.c", 84
    lda #2
    ldx #0
    sta $20
    stx $21
    leax.local 82
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #6
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_bug179_comprehensive.c", 85
    lda #0
    sta $20
    sta $21
    leax.local 82
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #4
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_bug179_comprehensive.c", 86
    lda #0
    sta $20
    sta $21
    leax.local 82
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #2
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_bug179_comprehensive.c", 87
    lda #0
    sta $20
    sta $21
    leax.local 82
    sta $22
    stx $23
    lda $20
    ldx $21
    ldy #0
    sta ($22),y
    txa
    iny
    sta ($22),y
    .loc "test_bug179_comprehensive.c", 88
    lda #0
    sta $20
    sta $21
    leax.local 82
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #12
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_bug179_comprehensive.c", 89
    lda #0
    sta $20
    sta $21
    leax.local 82
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #14
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_bug179_comprehensive.c", 90
    lda #0
    sta $20
    sta $21
    leax.local 82
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #16
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_bug179_comprehensive.c", 91
    leax.local 82
    sta _main__local_222
    stx _main__local_222+1
    lda _main__local_222
    ldx _main__local_222+1
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    jsr _mktime
    sta __zp_scratch4
    stx __zp_scratch4+1
    sty __zp_scratch4+2
    stz __zp_scratch4+3
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    ldy __zp_scratch4+2
    ldz __zp_scratch4+3
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    sta _main__local_221
    stx _main__local_221+1
    sty _main__local_221+2
    stz _main__local_221+3
    .loc "test_bug179_comprehensive.c", 93
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    ldy #0
    ldz #0
    sta $22
    stx $23
    sty $24
    stz $25
    lda _main__local_190
    ldx _main__local_190+1
    ldy _main__local_190+2
    ldz _main__local_190+3
    cmp.32 .AXYZ, $22
    bne @sc_merge20
    bra @sc_short21
@sc_merge20:
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    ldy #0
    ldz #0
    sta $22
    stx $23
    sty $24
    stz $25
    lda _main__local_221
    ldx _main__local_221+1
    ldy _main__local_221+2
    ldz _main__local_221+3
    cmp.32 .AXYZ, $22
    bne @__cmp_set_1
    bra @__cmp_zero_1
@__cmp_set_1:
    lda #1
    ldx #0
    bra @__cmp_done_1
@__cmp_zero_1:
    lda #0
    ldx #0
@__cmp_done_1:
    sta $20
    bra @sc_done22
@sc_short21:
@sc_done22:
    lda $20
    bne @sc_merge18
    bra @sc_short19
@sc_merge18:
    lda _main__local_190
    ldx _main__local_190+1
    ldy _main__local_190+2
    ldz _main__local_190+3
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_221
    ldx _main__local_221+1
    ldy _main__local_221+2
    ldz _main__local_221+3
    cmp.32 .AXYZ, __zp_scratch2
    beq @__cmp_zero_2
    bcs @__cmp_set_2
    bra @__cmp_zero_2
@__cmp_set_2:
    lda #1
    ldx #0
    bra @__cmp_done_2
@__cmp_zero_2:
    lda #0
    ldx #0
@__cmp_done_2:
    sta $20
    bra @sc_done23
@sc_short19:
@sc_done23:
    lda $20
    bne @tern_then24
    bra @tern_else25
@tern_then24:
    lda #255
    ldx #0
    sta $20
    stx $21
    bra @tern_end26
@tern_else25:
    lda #6
    ldx #0
    sta $20
    stx $21
@tern_end26:
    lda $20
    ldx $21
    sta $22
    lda _result_addr
    ldx _result_addr+1
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
    .loc "test_bug179_comprehensive.c", 96
    lda #170
    sta $20
    lda _result_addr
    ldx _result_addr+1
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
    .loc "test_bug179_comprehensive.c", 98
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 100
    endproc


__zp_save_buf:

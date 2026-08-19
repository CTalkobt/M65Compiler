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

    .global _test_result
    .global _test_count
    .global _main

    .segment "data"
    .byte 0
_test_result:
; .debug_var: @global _test_result offset=0 size=2 type=ptr scope=global
    .word 16384
_test_count:
; .debug_var: @global _test_count offset=0 size=2 type=int16 scope=global
    .word 0

    .segment "code"

; function _main
; SAC inline storage: 22 bytes
    _main__local_0: .word 0
    _main__local_1: .long 0
    _main__local_19: .word 0
    _main__local_29: .word 0
    _main__local_35: .word 0
    _main__local_39: .word 0
    _main__local_47: .word 0
    _main__local_55: .word 0
    _main__local_63: .word 0
    _main__local_70: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/time.h", 6
    .local @_l_result = 0
    .local @_l_t = 4
; .debug_var: __main @_l_result offset=0 size=4 type=int32 scope=local
; .debug_var: __main @_l_t offset=4 size=2 type=int16 scope=local

@entry:
    .loc "test_mktime_fix.c", 12
    lda #126
    ldx #0
    sta $20
    stx $21
    leax.local 4
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
    .loc "test_mktime_fix.c", 13
    lda #6
    ldx #0
    sta $20
    stx $21
    leax.local 4
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
    .loc "test_mktime_fix.c", 14
    lda #3
    ldx #0
    sta $20
    stx $21
    leax.local 4
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
    .loc "test_mktime_fix.c", 15
    lda #12
    ldx #0
    sta $20
    stx $21
    leax.local 4
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
    .loc "test_mktime_fix.c", 16
    lda #0
    sta $20
    sta $21
    leax.local 4
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
    .loc "test_mktime_fix.c", 17
    lda #0
    sta $20
    sta $21
    leax.local 4
    sta $22
    stx $23
    lda $20
    ldx $21
    ldy #0
    sta ($22),y
    txa
    iny
    sta ($22),y
    .loc "test_mktime_fix.c", 18
    lda #0
    sta $20
    sta $21
    leax.local 4
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
    .loc "test_mktime_fix.c", 19
    lda #0
    sta $20
    sta $21
    leax.local 4
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
    .loc "test_mktime_fix.c", 20
    lda #0
    sta $20
    sta $21
    leax.local 4
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
    .loc "test_mktime_fix.c", 22
    leax.local 4
    sta _main__local_29
    stx _main__local_29+1
    lda _main__local_29
    ldx _main__local_29+1
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
    .loc "test_mktime_fix.c", 25
    lda #170
    sta $20
    lda _test_result
    ldx _test_result+1
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
    .loc "test_mktime_fix.c", 28
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
    lda _main__local_1
    ldx _main__local_1+1
    ldy _main__local_1+2
    ldz _main__local_1+3
    and $22
    sta $26
    stx $27
    sty $28
    stz $29
    lda _test_result
    ldx _test_result+1
    sta $20
    stx $21
    lda #1
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
    .loc "test_mktime_fix.c", 29
    lda _main__local_1
    ldx _main__local_1+1
    ldy _main__local_1+2
    ldz _main__local_1+3
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
    lda _test_result
    ldx _test_result+1
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
    .loc "test_mktime_fix.c", 30
    lda _main__local_1
    ldx _main__local_1+1
    ldy _main__local_1+2
    ldz _main__local_1+3
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
    lda _test_result
    ldx _test_result+1
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
    .loc "test_mktime_fix.c", 31
    lda _main__local_1
    ldx _main__local_1+1
    ldy _main__local_1+2
    ldz _main__local_1+3
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
    lda _test_result
    ldx _test_result+1
    sta $20
    stx $21
    lda #4
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
    .loc "test_mktime_fix.c", 34
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
    lda #187
    ldx #0
    sta $20
    stx $21
    bra @tern_end2
@tern_else1:
    lda #204
    ldx #0
    sta $20
    stx $21
@tern_end2:
    lda $20
    ldx $21
    sta $22
    lda _test_result
    ldx _test_result+1
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
    .loc "test_mktime_fix.c", 37
    lda #255
    sta $20
    lda _test_result
    ldx _test_result+1
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
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 22
    endproc


__zp_save_buf:

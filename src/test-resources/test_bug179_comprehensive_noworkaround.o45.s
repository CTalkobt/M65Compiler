    .o45
    .extern __sp_base
    .weak __static_chain
    .weak __zp_scratch
    .weak __zp_scratch2
    .weak __zp_scratch3
    .weak __zp_scratch4
    __static_chain = $06
    __zp_scratch = $08
    __zp_scratch2 = $0A
    __zp_scratch3 = $0C
    __zp_scratch4 = $0E

    .extern _mktime

    .global _result_addr
    .global _main

    .segment "data"
    .byte 0
_result_addr:
    .word 16384

    .segment "code"

; function _main
    proc _main
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler.dev_v1.0.5/bin/../lib/include/time.h", 5
; frame: 112 bytes (frame-allocated vRegs only)
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    tsx
    txa
    clc
    adc #1
    sta $FD
    lda #$01
    adc #0
    sta $FE
    .local __vr0 = 26
    .local __vr1 = 10
    .local __vr2 = 14
    .local __vr3 = 18
    .local __vr4 = 98
    .local __vr34 = 100
    .local __vr65 = 102
    .local __vr96 = 104
    .local __vr108 = 0
    .local __vr120 = 44
    .local __vr148 = 22
    .local __vr149 = 106
    .local __vr162 = 62
    .local __vr190 = 2
    .local __vr191 = 108
    .local __vr193 = 80
    .local __vr221 = 6
    .local __vr222 = 110

@entry:
    .loc "test_bug179_comprehensive.c", 10
    .loc "test_bug179_comprehensive.c", 13
    .loc "test_bug179_comprehensive.c", 14
    lda #126
    ldx #0
    sta $20
    stx $21
    leax.fp 26
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
    leax.fp 26
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
    leax.fp 26
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
    leax.fp 26
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
    leax.fp 26
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
    leax.fp 26
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
    leax.fp 26
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
    leax.fp 26
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
    leax.fp 26
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
    leax.fp 26
    stax.fp __vr34
    ldax.fp __vr34
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _mktime
    phx
    pha
    tsx
    txa
    clc
    adc #1
    sta $FD
    lda #$01
    adc #0
    sta $FE
    pla
    plx
    stz @__restore_caller_z_0+1
    plz
    plz
@__restore_caller_z_0:
    ldz #0
    .var _fp = _fp - 2
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    staxyz.fp __vr1
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
    ldaxyz.fp __vr1
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
    .noopt_start
    addr_elem.16 __zp_scratch, $20, $24, #1
    ldy #0
    .noopt_end
    pla
    sta (__zp_scratch),y
    .loc "test_bug179_comprehensive.c", 27
    .loc "test_bug179_comprehensive.c", 28
    lda #125
    ldx #0
    sta $20
    stx $21
    leax.fp 26
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
    leax.fp 26
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
    leax.fp 26
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
    leax.fp 26
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
    leax.fp 26
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
    leax.fp 26
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
    leax.fp 26
    stax.fp __vr65
    ldax.fp __vr65
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _mktime
    phx
    pha
    tsx
    txa
    clc
    adc #1
    sta $FD
    lda #$01
    adc #0
    sta $FE
    pla
    plx
    stz @__restore_caller_z_1+1
    plz
    plz
@__restore_caller_z_1:
    ldz #0
    .var _fp = _fp - 2
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    staxyz.fp __vr2
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
    ldaxyz.fp __vr2
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
    .noopt_start
    addr_elem.16 __zp_scratch, $20, $24, #1
    ldy #0
    .noopt_end
    pla
    sta (__zp_scratch),y
    .loc "test_bug179_comprehensive.c", 38
    .loc "test_bug179_comprehensive.c", 39
    lda #124
    ldx #0
    sta $20
    stx $21
    leax.fp 26
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
    leax.fp 26
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
    leax.fp 26
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
    leax.fp 26
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
    leax.fp 26
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
    leax.fp 26
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
    leax.fp 26
    stax.fp __vr96
    ldax.fp __vr96
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _mktime
    phx
    pha
    tsx
    txa
    clc
    adc #1
    sta $FD
    lda #$01
    adc #0
    sta $FE
    pla
    plx
    stz @__restore_caller_z_2+1
    plz
    plz
@__restore_caller_z_2:
    ldz #0
    .var _fp = _fp - 2
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    staxyz.fp __vr3
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
    ldaxyz.fp __vr3
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
    .noopt_start
    addr_elem.16 __zp_scratch, $20, $24, #1
    ldy #0
    .noopt_end
    pla
    sta (__zp_scratch),y
    .loc "test_bug179_comprehensive.c", 49
    ldaxyz.fp __vr2
    sta __zp_scratch2
    stx __zp_scratch2+1
    ldaxyz.fp __vr1
    cmp.32 .AXYZ, __zp_scratch2
    bne @sc_merge9
    bra @sc_short10
@sc_merge9:
    ldaxyz.fp __vr3
    sta __zp_scratch2
    stx __zp_scratch2+1
    ldaxyz.fp __vr2
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
    stax.fp __vr108
    .loc "test_bug179_comprehensive.c", 50
    ldax.fp __vr108
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
    .noopt_start
    addr_elem.16 __zp_scratch, $20, $24, #1
    ldy #0
    .noopt_end
    pla
    sta (__zp_scratch),y
    .loc "test_bug179_comprehensive.c", 53
    .loc "test_bug179_comprehensive.c", 55
    lda #126
    ldx #0
    sta $20
    stx $21
    leax.fp 44
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
    leax.fp 44
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
    leax.fp 44
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
    leax.fp 44
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
    leax.fp 44
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
    leax.fp 44
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
    leax.fp 44
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
    leax.fp 44
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
    leax.fp 44
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
    leax.fp 44
    stax.fp __vr149
    ldax.fp __vr149
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _mktime
    phx
    pha
    tsx
    txa
    clc
    adc #1
    sta $FD
    lda #$01
    adc #0
    sta $FE
    pla
    plx
    stz @__restore_caller_z_3+1
    plz
    plz
@__restore_caller_z_3:
    ldz #0
    .var _fp = _fp - 2
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    staxyz.fp __vr148
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
    ldaxyz.fp __vr148
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
    .noopt_start
    addr_elem.16 __zp_scratch, $20, $24, #1
    ldy #0
    .noopt_end
    pla
    sta (__zp_scratch),y
    .loc "test_bug179_comprehensive.c", 68
    .loc "test_bug179_comprehensive.c", 70
    lda #126
    ldx #0
    sta $20
    stx $21
    leax.fp 62
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
    leax.fp 62
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
    leax.fp 62
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
    leax.fp 62
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
    leax.fp 62
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
    leax.fp 62
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
    leax.fp 62
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
    leax.fp 62
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
    leax.fp 62
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
    leax.fp 62
    stax.fp __vr191
    ldax.fp __vr191
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _mktime
    phx
    pha
    tsx
    txa
    clc
    adc #1
    sta $FD
    lda #$01
    adc #0
    sta $FE
    pla
    plx
    stz @__restore_caller_z_4+1
    plz
    plz
@__restore_caller_z_4:
    ldz #0
    .var _fp = _fp - 2
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    staxyz.fp __vr190
    .loc "test_bug179_comprehensive.c", 82
    lda #126
    ldx #0
    sta $20
    stx $21
    leax.fp 80
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
    leax.fp 80
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
    leax.fp 80
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
    leax.fp 80
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
    leax.fp 80
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
    leax.fp 80
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
    leax.fp 80
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
    leax.fp 80
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
    leax.fp 80
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
    leax.fp 80
    stax.fp __vr222
    ldax.fp __vr222
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _mktime
    phx
    pha
    tsx
    txa
    clc
    adc #1
    sta $FD
    lda #$01
    adc #0
    sta $FE
    pla
    plx
    stz @__restore_caller_z_5+1
    plz
    plz
@__restore_caller_z_5:
    ldz #0
    .var _fp = _fp - 2
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    staxyz.fp __vr221
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
    ldaxyz.fp __vr190
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
    ldaxyz.fp __vr221
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
    ldaxyz.fp __vr190
    sta __zp_scratch2
    stx __zp_scratch2+1
    ldaxyz.fp __vr221
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
    .noopt_start
    addr_elem.16 __zp_scratch, $20, $24, #1
    ldy #0
    .noopt_end
    pla
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
    .noopt_start
    addr_elem.16 __zp_scratch, $22, $24, #1
    ldy #0
    .noopt_end
    pla
    sta (__zp_scratch),y
    .loc "test_bug179_comprehensive.c", 98
    lda #0
    ldx #0
@__return:
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    .func_flags stack_call
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc


__zp_save_buf:
    .res 248

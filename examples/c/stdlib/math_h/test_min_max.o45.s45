    .o45
    .extern __sp_base
    __zp_scratch = $08
    __zp_scratch2 = $0A
    __zp_scratch3 = $0C
    __zp_scratch4 = $0E

    .extern _max
    .extern _min

    .global _main

    .segment "code"

; function _main
    proc _main
    .var _fp = 0
; frame: 72 bytes (frame-allocated vRegs only)
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
    .local __vr0 = 0
    .local __vr1 = 2
    .local __vr5 = 4
    .local __vr6 = 6
    .local __vr10 = 8
    .local __vr11 = 10
    .local __vr15 = 12
    .local __vr16 = 14
    .local __vr20 = 16
    .local __vr21 = 18
    .local __vr25 = 20
    .local __vr26 = 22
    .local __vr30 = 24
    .local __vr31 = 26
    .local __vr35 = 28
    .local __vr36 = 30
    .local __vr40 = 32
    .local __vr41 = 34
    .local __vr45 = 36
    .local __vr46 = 38
    .local __vr50 = 40
    .local __vr51 = 42
    .local __vr55 = 44
    .local __vr56 = 46
    .local __vr60 = 48
    .local __vr61 = 50
    .local __vr65 = 52
    .local __vr66 = 54
    .local __vr70 = 56
    .local __vr71 = 58
    .local __vr75 = 60
    .local __vr76 = 62
    .local __vr80 = 64
    .local __vr81 = 66
    .local __vr85 = 68
    .local __vr86 = 70

@entry:
    .loc "test_min_max.c", 91
    lda #3
    ldx #0
    stax.fp __vr0
    lda #5
    ldx #0
    stax.fp __vr1
    ldax.fp __vr1
    sta $28
    stx $29
    ldax.fp __vr0
    sta $2C
    stx $2D
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    lda $2C
    ldx $2D
    push .ax
    .var _fp = _fp + 2
    jsr _min
    .var _fp = _fp - 4
    sta $20
    stx $21
    lda #3
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    cmp.16 .AX, #3
    bne @if_then0
    bra @if_end2
@if_then0:
    lda #1
    ldx #0
    bra @__return
@if_end2:
    .loc "test_min_max.c", 92
    lda #5
    ldx #0
    stax.fp __vr5
    lda #3
    ldx #0
    stax.fp __vr6
    ldax.fp __vr6
    sta $28
    stx $29
    ldax.fp __vr5
    sta $2C
    stx $2D
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    lda $2C
    ldx $2D
    push .ax
    .var _fp = _fp + 2
    jsr _min
    .var _fp = _fp - 4
    sta $20
    stx $21
    lda #3
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    cmp.16 .AX, #3
    bne @if_then3
    bra @if_end5
@if_then3:
    lda #2
    ldx #0
    bra @__return
@if_end5:
    .loc "test_min_max.c", 93
    lda #4
    ldx #0
    stax.fp __vr10
    lda #4
    ldx #0
    stax.fp __vr11
    ldax.fp __vr11
    sta $28
    stx $29
    ldax.fp __vr10
    sta $2C
    stx $2D
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    lda $2C
    ldx $2D
    push .ax
    .var _fp = _fp + 2
    jsr _min
    .var _fp = _fp - 4
    sta $20
    stx $21
    lda #4
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    cmp.16 .AX, #4
    bne @if_then6
    bra @if_end8
@if_then6:
    lda #3
    ldx #0
    bra @__return
@if_end8:
    .loc "test_min_max.c", 96
    lda #255
    ldx #255
    stax.fp __vr15
    lda #1
    ldx #0
    stax.fp __vr16
    ldax.fp __vr16
    sta $28
    stx $29
    ldax.fp __vr15
    sta $2C
    stx $2D
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    lda $2C
    ldx $2D
    push .ax
    .var _fp = _fp + 2
    jsr _min
    .var _fp = _fp - 4
    sta $20
    stx $21
    lda #255
    ldx #255
    sta $22
    stx $23
    lda $20
    ldx $21
    cmp.16 .AX, #-1
    bne @if_then9
    bra @if_end11
@if_then9:
    lda #4
    ldx #0
    bra @__return
@if_end11:
    .loc "test_min_max.c", 97
    lda #246
    ldx #255
    stax.fp __vr20
    lda #251
    ldx #255
    stax.fp __vr21
    ldax.fp __vr21
    sta $28
    stx $29
    ldax.fp __vr20
    sta $2C
    stx $2D
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    lda $2C
    ldx $2D
    push .ax
    .var _fp = _fp + 2
    jsr _min
    .var _fp = _fp - 4
    sta $20
    stx $21
    lda #246
    ldx #255
    sta $22
    stx $23
    lda $20
    ldx $21
    cmp.16 .AX, #-10
    bne @if_then12
    bra @if_end14
@if_then12:
    lda #5
    ldx #0
    bra @__return
@if_end14:
    .loc "test_min_max.c", 100
    lda #0
    ldx #0
    stax.fp __vr25
    lda #0
    ldx #0
    stax.fp __vr26
    ldax.fp __vr26
    sta $28
    stx $29
    ldax.fp __vr25
    sta $2C
    stx $2D
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    lda $2C
    ldx $2D
    push .ax
    .var _fp = _fp + 2
    jsr _min
    .var _fp = _fp - 4
    sta $20
    stx $21
    lda #0
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    cmp.16 .AX, #0
    bne @if_then15
    bra @if_end17
@if_then15:
    lda #6
    ldx #0
    bra @__return
@if_end17:
    .loc "test_min_max.c", 101
    lda #0
    ldx #0
    stax.fp __vr30
    lda #1
    ldx #0
    stax.fp __vr31
    ldax.fp __vr31
    sta $28
    stx $29
    ldax.fp __vr30
    sta $2C
    stx $2D
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    lda $2C
    ldx $2D
    push .ax
    .var _fp = _fp + 2
    jsr _min
    .var _fp = _fp - 4
    sta $20
    stx $21
    lda #0
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    cmp.16 .AX, #0
    bne @if_then18
    bra @if_end20
@if_then18:
    lda #7
    ldx #0
    bra @__return
@if_end20:
    .loc "test_min_max.c", 102
    lda #255
    ldx #255
    stax.fp __vr35
    lda #0
    ldx #0
    stax.fp __vr36
    ldax.fp __vr36
    sta $28
    stx $29
    ldax.fp __vr35
    sta $2C
    stx $2D
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    lda $2C
    ldx $2D
    push .ax
    .var _fp = _fp + 2
    jsr _min
    .var _fp = _fp - 4
    sta $20
    stx $21
    lda #255
    ldx #255
    sta $22
    stx $23
    lda $20
    ldx $21
    cmp.16 .AX, #-1
    bne @if_then21
    bra @if_end23
@if_then21:
    lda #8
    ldx #0
    bra @__return
@if_end23:
    .loc "test_min_max.c", 105
    lda #3
    ldx #0
    stax.fp __vr40
    lda #5
    ldx #0
    stax.fp __vr41
    ldax.fp __vr41
    sta $28
    stx $29
    ldax.fp __vr40
    sta $2C
    stx $2D
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    lda $2C
    ldx $2D
    push .ax
    .var _fp = _fp + 2
    jsr _max
    .var _fp = _fp - 4
    sta $20
    stx $21
    lda #5
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    cmp.16 .AX, #5
    bne @if_then24
    bra @if_end26
@if_then24:
    lda #10
    ldx #0
    bra @__return
@if_end26:
    .loc "test_min_max.c", 106
    lda #5
    ldx #0
    stax.fp __vr45
    lda #3
    ldx #0
    stax.fp __vr46
    ldax.fp __vr46
    sta $28
    stx $29
    ldax.fp __vr45
    sta $2C
    stx $2D
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    lda $2C
    ldx $2D
    push .ax
    .var _fp = _fp + 2
    jsr _max
    .var _fp = _fp - 4
    sta $20
    stx $21
    lda #5
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    cmp.16 .AX, #5
    bne @if_then27
    bra @if_end29
@if_then27:
    lda #11
    ldx #0
    bra @__return
@if_end29:
    .loc "test_min_max.c", 107
    lda #4
    ldx #0
    stax.fp __vr50
    lda #4
    ldx #0
    stax.fp __vr51
    ldax.fp __vr51
    sta $28
    stx $29
    ldax.fp __vr50
    sta $2C
    stx $2D
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    lda $2C
    ldx $2D
    push .ax
    .var _fp = _fp + 2
    jsr _max
    .var _fp = _fp - 4
    sta $20
    stx $21
    lda #4
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    cmp.16 .AX, #4
    bne @if_then30
    bra @if_end32
@if_then30:
    lda #12
    ldx #0
    bra @__return
@if_end32:
    .loc "test_min_max.c", 110
    lda #255
    ldx #255
    stax.fp __vr55
    lda #1
    ldx #0
    stax.fp __vr56
    ldax.fp __vr56
    sta $28
    stx $29
    ldax.fp __vr55
    sta $2C
    stx $2D
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    lda $2C
    ldx $2D
    push .ax
    .var _fp = _fp + 2
    jsr _max
    .var _fp = _fp - 4
    sta $20
    stx $21
    lda #1
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    cmp.16 .AX, #1
    bne @if_then33
    bra @if_end35
@if_then33:
    lda #13
    ldx #0
    bra @__return
@if_end35:
    .loc "test_min_max.c", 111
    lda #246
    ldx #255
    stax.fp __vr60
    lda #251
    ldx #255
    stax.fp __vr61
    ldax.fp __vr61
    sta $28
    stx $29
    ldax.fp __vr60
    sta $2C
    stx $2D
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    lda $2C
    ldx $2D
    push .ax
    .var _fp = _fp + 2
    jsr _max
    .var _fp = _fp - 4
    sta $20
    stx $21
    lda #251
    ldx #255
    sta $22
    stx $23
    lda $20
    ldx $21
    cmp.16 .AX, #-5
    bne @if_then36
    bra @if_end38
@if_then36:
    lda #14
    ldx #0
    bra @__return
@if_end38:
    .loc "test_min_max.c", 114
    lda #0
    ldx #0
    stax.fp __vr65
    lda #0
    ldx #0
    stax.fp __vr66
    ldax.fp __vr66
    sta $28
    stx $29
    ldax.fp __vr65
    sta $2C
    stx $2D
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    lda $2C
    ldx $2D
    push .ax
    .var _fp = _fp + 2
    jsr _max
    .var _fp = _fp - 4
    sta $20
    stx $21
    lda #0
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    cmp.16 .AX, #0
    bne @if_then39
    bra @if_end41
@if_then39:
    lda #15
    ldx #0
    bra @__return
@if_end41:
    .loc "test_min_max.c", 115
    lda #0
    ldx #0
    stax.fp __vr70
    lda #1
    ldx #0
    stax.fp __vr71
    ldax.fp __vr71
    sta $28
    stx $29
    ldax.fp __vr70
    sta $2C
    stx $2D
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    lda $2C
    ldx $2D
    push .ax
    .var _fp = _fp + 2
    jsr _max
    .var _fp = _fp - 4
    sta $20
    stx $21
    lda #1
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    cmp.16 .AX, #1
    bne @if_then42
    bra @if_end44
@if_then42:
    lda #16
    ldx #0
    bra @__return
@if_end44:
    .loc "test_min_max.c", 116
    lda #255
    ldx #255
    stax.fp __vr75
    lda #0
    ldx #0
    stax.fp __vr76
    ldax.fp __vr76
    sta $28
    stx $29
    ldax.fp __vr75
    sta $2C
    stx $2D
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    lda $2C
    ldx $2D
    push .ax
    .var _fp = _fp + 2
    jsr _max
    .var _fp = _fp - 4
    sta $20
    stx $21
    lda #0
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    cmp.16 .AX, #0
    bne @if_then45
    bra @if_end47
@if_then45:
    lda #17
    ldx #0
    bra @__return
@if_end47:
    .loc "test_min_max.c", 119
    lda #255
    ldx #127
    stax.fp __vr80
    lda #0
    ldx #128
    stax.fp __vr81
    ldax.fp __vr81
    sta $28
    stx $29
    ldax.fp __vr80
    sta $2C
    stx $2D
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    lda $2C
    ldx $2D
    push .ax
    .var _fp = _fp + 2
    jsr _min
    .var _fp = _fp - 4
    sta $20
    stx $21
    lda #0
    ldx #128
    sta $22
    stx $23
    lda $20
    ldx $21
    cmp.16 .AX, #-32768
    bne @if_then48
    bra @if_end50
@if_then48:
    lda #20
    ldx #0
    bra @__return
@if_end50:
    .loc "test_min_max.c", 120
    lda #255
    ldx #127
    stax.fp __vr85
    lda #0
    ldx #128
    stax.fp __vr86
    ldax.fp __vr86
    sta $28
    stx $29
    ldax.fp __vr85
    sta $2C
    stx $2D
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    lda $2C
    ldx $2D
    push .ax
    .var _fp = _fp + 2
    jsr _max
    .var _fp = _fp - 4
    sta $20
    stx $21
    lda #255
    ldx #127
    sta $22
    stx $23
    lda $20
    ldx $21
    cmp.16 .AX, #32767
    bne @if_then51
    bra @if_end53
@if_then51:
    lda #21
    ldx #0
    bra @__return
@if_end53:
    .loc "test_min_max.c", 122
    lda #0
    ldx #0
@__return:
    sta __zp_scratch
    stx __zp_scratch+1
    sty __zp_scratch3
    stz __zp_scratch3+1
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    pla
    lda __zp_scratch
    ldx __zp_scratch+1
    ldy __zp_scratch3
    ldz __zp_scratch3+1
    .func_flags stack_call
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc


__zp_save_buf:

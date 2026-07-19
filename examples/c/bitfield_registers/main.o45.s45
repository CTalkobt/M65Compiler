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

    .extern _printf
    .extern _puts

    .global _VICIV_CTRL_A
    .global _VICIV_CTRL_B
    .global _SPR_ENABLE
    .global _SCREEN
    .global _main

    .segment "data"
    .byte 0
_VICIV_CTRL_A:
    .word 53297
_VICIV_CTRL_B:
    .word 53332
_SPR_ENABLE:
    .word 53269
_SCREEN:
    .word 2048

    .segment "code"

; function _unlock_mega65_io
    proc _unlock_mega65_io
    .var _fp = 0
    .loc "main.c", 46
    tsx
    txa
    clc
    adc #1
    sta $FD
    lda #$01
    adc #0
    sta $FE

@entry:
    .loc "main.c", 66
    lda #$47
    .loc "main.c", 67
    sta $d02f
    .loc "main.c", 68
    lda #$53
    .loc "main.c", 69
    sta $d02f
@__return:
    .func_flags stack_call, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc

; function _print_ctrl_a
    proc _print_ctrl_a
    .var _fp = 0
    .loc "main.c", 53
; frame: 20 bytes (frame-allocated vRegs only)
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
    .local __vr0 = 0
    .local __vr3 = 2
    .local __vr5 = 4
    .local __vr9 = 6
    .local __vr13 = 8
    .local __vr17 = 10
    .local __vr19 = 12
    .local __vr23 = 14
    .local __vr27 = 16
    .local __vr31 = 18
    .local @_l_reg = 0

@entry:
    .loc "main.c", 75
    lda _VICIV_CTRL_A
    ldx _VICIV_CTRL_A+1
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
    stax.fp __vr0
    .loc "main.c", 77
    ldax #__str_0
    stax.fp __vr3
    ldax.fp __vr3
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _printf
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
    plz
    plz
    .var _fp = _fp - 2
    .loc "main.c", 78
    ldax #__str_1
    stax.fp __vr5
    .loc "main.c", 79
    leax.fp 0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    ldy #0
    lda ($20),y
    ldx #0
    sta $22
    lda $22
    ldx #0
    bfext #0, #1
    sta.fp __vr9
    leax.fp 0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    ldy #0
    lda ($20),y
    ldx #0
    sta $22
    lda $22
    ldx #0
    bfext #1, #1
    sta.fp __vr13
    leax.fp 0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    ldy #0
    lda ($20),y
    ldx #0
    sta $22
    lda $22
    ldx #0
    bfext #2, #1
    sta.fp __vr17
    .loc "main.c", 78
    lda.fp __vr17
    sta $28
    stx $29
    lda.fp __vr13
    sta $2A
    stx $2B
    lda.fp __vr9
    sta $2C
    stx $2D
    ldax.fp __vr5
    sta $2E
    stx $2F
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    lda $2A
    ldx $2B
    push .ax
    .var _fp = _fp + 2
    lda $2C
    ldx $2D
    push .ax
    .var _fp = _fp + 2
    lda $2E
    ldx $2F
    push .ax
    .var _fp = _fp + 2
    jsr _printf
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
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    .var _fp = _fp - 8
    .loc "main.c", 80
    ldax #__str_2
    stax.fp __vr19
    .loc "main.c", 81
    leax.fp 0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    ldy #0
    lda ($20),y
    ldx #0
    sta $22
    lda $22
    ldx #0
    bfext #3, #1
    sta.fp __vr23
    leax.fp 0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    ldy #0
    lda ($20),y
    ldx #0
    sta $22
    lda $22
    ldx #0
    bfext #4, #1
    sta.fp __vr27
    leax.fp 0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    ldy #0
    lda ($20),y
    ldx #0
    sta $22
    lda $22
    ldx #0
    bfext #5, #1
    sta.fp __vr31
    .loc "main.c", 80
    lda.fp __vr31
    sta $28
    stx $29
    lda.fp __vr27
    sta $2A
    stx $2B
    lda.fp __vr23
    sta $2C
    stx $2D
    ldax.fp __vr19
    sta $2E
    stx $2F
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    lda $2A
    ldx $2B
    push .ax
    .var _fp = _fp + 2
    lda $2C
    ldx $2D
    push .ax
    .var _fp = _fp + 2
    lda $2E
    ldx $2F
    push .ax
    .var _fp = _fp + 2
    jsr _printf
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
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    .var _fp = _fp - 8
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
    .func_flags stack_call
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc

; function _print_ctrl_b
    proc _print_ctrl_b
    .var _fp = 0
    .loc "main.c", 65
; frame: 16 bytes (frame-allocated vRegs only)
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
    .local __vr0 = 0
    .local __vr3 = 2
    .local __vr5 = 4
    .local __vr9 = 6
    .local __vr13 = 8
    .local __vr15 = 10
    .local __vr19 = 12
    .local __vr23 = 14
    .local @_l_reg = 0

@entry:
    .loc "main.c", 86
    lda _VICIV_CTRL_B
    ldx _VICIV_CTRL_B+1
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
    stax.fp __vr0
    .loc "main.c", 88
    ldax #__str_3
    stax.fp __vr3
    ldax.fp __vr3
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _printf
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
    plz
    plz
    .var _fp = _fp - 2
    .loc "main.c", 89
    ldax #__str_4
    stax.fp __vr5
    .loc "main.c", 90
    leax.fp 0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    ldy #0
    lda ($20),y
    ldx #0
    sta $22
    lda $22
    ldx #0
    bfext #0, #2
    sta.fp __vr9
    leax.fp 0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    ldy #0
    lda ($20),y
    ldx #0
    sta $22
    lda $22
    ldx #0
    bfext #2, #1
    sta.fp __vr13
    .loc "main.c", 89
    lda.fp __vr13
    sta $28
    stx $29
    lda.fp __vr9
    sta $2A
    stx $2B
    ldax.fp __vr5
    sta $2C
    stx $2D
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    lda $2A
    ldx $2B
    push .ax
    .var _fp = _fp + 2
    lda $2C
    ldx $2D
    push .ax
    .var _fp = _fp + 2
    jsr _printf
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
    plz
    plz
    plz
    plz
    plz
    plz
    .var _fp = _fp - 6
    .loc "main.c", 91
    ldax #__str_5
    stax.fp __vr15
    .loc "main.c", 92
    leax.fp 0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    ldy #0
    lda ($20),y
    ldx #0
    sta $22
    lda $22
    ldx #0
    bfext #7, #1
    sta.fp __vr19
    leax.fp 0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    ldy #0
    lda ($20),y
    ldx #0
    sta $22
    lda $22
    ldx #0
    bfext #6, #1
    sta.fp __vr23
    .loc "main.c", 91
    lda.fp __vr23
    sta $28
    stx $29
    lda.fp __vr19
    sta $2A
    stx $2B
    ldax.fp __vr15
    sta $2C
    stx $2D
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    lda $2A
    ldx $2B
    push .ax
    .var _fp = _fp + 2
    lda $2C
    ldx $2D
    push .ax
    .var _fp = _fp + 2
    jsr _printf
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
    plz
    plz
    plz
    plz
    plz
    plz
    .var _fp = _fp - 6
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
    .func_flags stack_call
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc

; function _main
    proc _main
    .var _fp = 0
    .loc "main.c", 76
; frame: 16 bytes (frame-allocated vRegs only)
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
    .local __vr0 = 0
    .local __vr2 = 2
    .local __vr4 = 4
    .local __vr9 = 6
    .local __vr17 = 8
    .local __vr19 = 10
    .local __vr21 = 12
    .local __vr63 = 14

@entry:
    .loc "main.c", 96
    jsr _unlock_mega65_io
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
    .loc "main.c", 98
    ldax #__str_6
    stax.fp __vr0
    ldax.fp __vr0
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _puts
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
    plz
    plz
    .var _fp = _fp - 2
    .loc "main.c", 99
    ldax #__str_7
    stax.fp __vr2
    ldax.fp __vr2
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _puts
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
    plz
    plz
    .var _fp = _fp - 2
    .loc "main.c", 102
    jsr _print_ctrl_a
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
    .loc "main.c", 103
    jsr _print_ctrl_b
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
    .loc "main.c", 106
    ldax #__str_8
    stax.fp __vr4
    ldax.fp __vr4
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _printf
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
    plz
    plz
    .var _fp = _fp - 2
    .loc "main.c", 107
    lda #3
    sta $20
    lda _VICIV_CTRL_B
    ldx _VICIV_CTRL_B+1
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    lda $22
    ldx $23
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx #0
    bfins __zp_scratch2, #0, #2
    .loc "main.c", 109
    jsr _print_ctrl_b
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
    .loc "main.c", 112
    ldax #__str_9
    stax.fp __vr9
    ldax.fp __vr9
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _printf
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
    plz
    plz
    .var _fp = _fp - 2
    .loc "main.c", 113
    lda #1
    sta $20
    lda _SPR_ENABLE
    ldx _SPR_ENABLE+1
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    lda $22
    ldx $23
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx #0
    bfins __zp_scratch2, #0, #1
    .loc "main.c", 114
    lda #1
    sta $20
    lda _SPR_ENABLE
    ldx _SPR_ENABLE+1
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    lda $22
    ldx $23
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx #0
    bfins __zp_scratch2, #2, #1
    .loc "main.c", 115
    ldax #__str_10
    stax.fp __vr17
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
    stax.fp __vr19
    ldax.fp __vr19
    sta $28
    stx $29
    ldax.fp __vr17
    sta $2A
    stx $2B
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    lda $2A
    ldx $2B
    push .ax
    .var _fp = _fp + 2
    jsr _printf
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
    plz
    plz
    plz
    plz
    .var _fp = _fp - 4
    .loc "main.c", 118
    ldax #__str_11
    stax.fp __vr21
    ldax.fp __vr21
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _printf
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
    plz
    plz
    .var _fp = _fp - 2
    .loc "main.c", 119
    lda #1
    sta $20
    lda _SCREEN
    ldx _SCREEN+1
    sta $22
    stx $23
    lda #0
    sta $24
    sta $25
    addr_elem.16 $26, $22, $24, #2
    lda $26
    ldx $27
    sta $22
    stx $23
    lda $20
    ldy #0
    sta ($26),y
    .loc "main.c", 120
    lda #2
    sta $20
    lda _SCREEN
    ldx _SCREEN+1
    sta $22
    stx $23
    lda #0
    sta $24
    sta $25
    addr_elem.16 __zp_scratch, $22, $24, #2
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $28
    stx $29
    lda $28
    ldx $29
    add.16 .AX, #1
    sta $22
    stx $23
    lda $22
    ldx $23
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx #0
    bfins __zp_scratch2, #0, #4
    .loc "main.c", 121
    lda #1
    sta $20
    lda _SCREEN
    ldx _SCREEN+1
    sta $22
    stx $23
    lda #0
    sta $24
    sta $25
    addr_elem.16 __zp_scratch, $22, $24, #2
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $28
    stx $29
    lda $28
    ldx $29
    add.16 .AX, #1
    sta $22
    stx $23
    lda $22
    ldx $23
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx #0
    bfins __zp_scratch2, #6, #1
    .loc "main.c", 122
    lda #1
    sta $20
    lda _SCREEN
    ldx _SCREEN+1
    sta $22
    stx $23
    lda #0
    sta $24
    sta $25
    addr_elem.16 __zp_scratch, $22, $24, #2
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $28
    stx $29
    lda $28
    ldx $29
    add.16 .AX, #1
    sta $22
    stx $23
    lda $22
    ldx $23
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx #0
    bfins __zp_scratch2, #7, #1
    .loc "main.c", 124
    lda #2
    sta $20
    lda _SCREEN
    ldx _SCREEN+1
    sta $22
    stx $23
    lda #1
    ldx #0
    sta $24
    stx $25
    addr_elem.16 $26, $22, $24, #2
    lda $26
    ldx $27
    sta $22
    stx $23
    lda $20
    ldy #0
    sta ($26),y
    .loc "main.c", 125
    lda #5
    sta $20
    lda _SCREEN
    ldx _SCREEN+1
    sta $22
    stx $23
    lda #1
    ldx #0
    sta $24
    stx $25
    addr_elem.16 __zp_scratch, $22, $24, #2
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $28
    stx $29
    lda $28
    ldx $29
    add.16 .AX, #1
    sta $22
    stx $23
    lda $22
    ldx $23
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx #0
    bfins __zp_scratch2, #0, #4
    .loc "main.c", 126
    lda #1
    sta $20
    lda _SCREEN
    ldx _SCREEN+1
    sta $22
    stx $23
    lda #1
    ldx #0
    sta $24
    stx $25
    addr_elem.16 __zp_scratch, $22, $24, #2
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $28
    stx $29
    lda $28
    ldx $29
    add.16 .AX, #1
    sta $22
    stx $23
    lda $22
    ldx $23
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx #0
    bfins __zp_scratch2, #5, #1
    .loc "main.c", 128
    ldax #__str_12
    stax.fp __vr63
    ldax.fp __vr63
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _puts
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
    plz
    plz
    .var _fp = _fp - 2
    .loc "main.c", 129
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
    .func_flags stack_call
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc


    .segment "data"
__str_0:
    .text "vic ctrl a ($d031):
"
    .byte 0
__str_1:
    .text "  h640=%d fast=%d attr=%d
"
    .byte 0
__str_2:
    .text "  bpm=%d v400=%d chr16=%d
"
    .byte 0
__str_3:
    .text "vic ctrl b ($d054):
"
    .byte 0
__str_4:
    .text "  fclksel=%d vfast=%d
"
    .byte 0
__str_5:
    .text "  alpha=%d rst_emu=%d
"
    .byte 0
__str_6:
    .text "bitfield register demo"
    .byte 0
__str_7:
    .text "----------------------"
    .byte 0
__str_8:
    .text "
setting 40mhz...
"
    .byte 0
__str_9:
    .text "
enabling sprites 0,2...
"
    .byte 0
__str_10:
    .text "spr enable: $%x
"
    .byte 0
__str_11:
    .text "
writing styled chars...
"
    .byte 0
__str_12:
    .text "
done."
    .byte 0

__zp_save_buf:

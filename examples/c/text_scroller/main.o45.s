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

    .extern _clear_screen
    .extern _scroll_line_left
    .extern _set_text_color
    .extern _strlen
    .extern _wait_vsync

    .global _message
    .global _palette
    .global _palette_len
    .global _init_palette
    .global _draw_banner
    .global _main

    .segment "data"
    .byte 0
_message:
    .word __str_0

    .segment "bss"
_palette:
    .res 7
_palette_len:
    .res 2

    .segment "code"

; function _cbm_chrout
    proc _cbm_chrout, B#@_p_c
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler.dev_v1.0.5/bin/../lib/include/cbm.h", 48
; frame: 2 bytes (frame-allocated vRegs only)
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
    .var @_p_c = 4

    ldax.fp @_p_c
    sta.fp __vr0
@entry:
    .loc "main.c", 60
    lda.sp @_p_c
    .loc "main.c", 61
    jsr $FFD2
@__return:
    plz
    plz
    .func_flags stack_call, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc

; function _cbm_stop
    proc _cbm_stop
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler.dev_v1.0.5/bin/../lib/include/cbm.h", 151
    tsx
    txa
    clc
    adc #1
    sta $FD
    lda #$01
    adc #0
    sta $FE

@entry:
    .loc "main.c", 163
    jsr $FFE1
    .loc "main.c", 165
    php
    .loc "main.c", 166
    ldx #$00
    .loc "main.c", 167
    plp
    .loc "main.c", 168
    bne *+3
    .loc "main.c", 169
    inx
    .loc "main.c", 170
    txa
    .loc "main.c", 171
@__return:
    .func_flags stack_call, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc

; function _cbm_gotoxy
    proc _cbm_gotoxy, B#@_p_col, B#@_p_row
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler.dev_v1.0.5/bin/../lib/include/cbm.h", 168
; frame: 4 bytes (frame-allocated vRegs only)
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
    .local __vr1 = 2
    .var @_p_col = 6
    .var @_p_row = 8

    ldax.fp @_p_col
    sta.fp __vr0
    ldax.fp @_p_row
    sta.fp __vr1
@entry:
    .loc "main.c", 180
    clc
    .loc "main.c", 181
    lda.sp @_p_row
    .loc "main.c", 182
    tax
    .loc "main.c", 183
    lda.sp @_p_col
    .loc "main.c", 184
    tay
    .loc "main.c", 185
    jsr $FFF0
@__return:
    plz
    plz
    plz
    plz
    .func_flags stack_call, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc

; function _init_palette
    proc _init_palette
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler.dev_v1.0.5/bin/../lib/include/cbm.h", 34
    tsx
    txa
    clc
    adc #1
    sta $FD
    lda #$01
    adc #0
    sta $FE

@entry:
    .loc "main.c", 46
    lda #3
    sta $20
    lda #0
    sta $22
    sta $23
    lda $20
    ldx #0
    pha
    addr_elem.16 __zp_scratch, #_palette, $22, #1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "main.c", 47
    lda #14
    sta $20
    lda #1
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    pha
    addr_elem.16 __zp_scratch, #_palette, $22, #1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "main.c", 48
    lda #6
    sta $20
    lda #2
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    pha
    addr_elem.16 __zp_scratch, #_palette, $22, #1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "main.c", 49
    lda #5
    sta $20
    lda #3
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    pha
    addr_elem.16 __zp_scratch, #_palette, $22, #1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "main.c", 50
    lda #13
    sta $20
    lda #4
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    pha
    addr_elem.16 __zp_scratch, #_palette, $22, #1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "main.c", 51
    lda #7
    sta $20
    lda #5
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    pha
    addr_elem.16 __zp_scratch, #_palette, $22, #1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "main.c", 52
    lda #1
    sta $20
    lda #6
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    pha
    addr_elem.16 __zp_scratch, #_palette, $22, #1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "main.c", 53
    lda #7
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _palette_len
    stx _palette_len+1
@__return:
    .func_flags stack_call, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers N, Z
    endproc

; function _draw_banner
    proc _draw_banner, B#@_p_color
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler.dev_v1.0.5/bin/../lib/include/cbm.h", 45
; frame: 10 bytes (frame-allocated vRegs only)
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
    .local __vr1 = 4
    .local __vr2 = 2
    .local __vr19 = 6
    .local __vr20 = 8
    .local @_l_i = 2
    .local @_l_scr = 4
    .var @_p_color = 12

    ldax.fp @_p_color
    sta.fp __vr0
@entry:
    .loc "main.c", 61
    lda #144
    ldx #9
    sta $20
    stx $21
    stax.fp __vr1
    .loc "main.c", 62
    lda #0
    taz
    staz.fp __vr2
@for_cond1:
    ldax.fp __vr2
    cmp.16 .AX, #40
    bcc @for_body2
    bra @for_end4
@for_body2:
    .loc "main.c", 63
    lda #67
    sta $24
    lda $24
    ldx #0
    pha
    ldax.fp __vr1
    sta __zp_scratch2
    stx __zp_scratch2+1
    ldax.fp __vr2
    sta __zp_scratch2
    stx __zp_scratch2+1
    addr_elem.16 __zp_scratch, __zp_scratch2, __zp_scratch2, #1
    pla
    ldy #0
    sta (__zp_scratch),y
@for_inc3:
    .loc "main.c", 62
    ldax.fp __vr2
    sta $28
    stx $29
    lda $28
    clc
    adc #1
    sta $2A
    lda $29
    adc #0
    sta $2B
    lda $2A
    ldx $2B
    stax.fp __vr2
    bra @for_cond1
@for_end4:
    .loc "main.c", 66
    lda #48
    ldx #10
    sta $20
    stx $21
    stax.fp __vr1
    .loc "main.c", 67
    lda #0
    taz
    staz.fp __vr2
@for_cond5:
    ldax.fp __vr2
    cmp.16 .AX, #40
    bcc @for_body6
    bra @for_end8
@for_body6:
    .loc "main.c", 68
    lda #67
    sta $24
    lda $24
    ldx #0
    pha
    ldax.fp __vr1
    sta __zp_scratch2
    stx __zp_scratch2+1
    ldax.fp __vr2
    sta __zp_scratch2
    stx __zp_scratch2+1
    addr_elem.16 __zp_scratch, __zp_scratch2, __zp_scratch2, #1
    pla
    ldy #0
    sta (__zp_scratch),y
@for_inc7:
    .loc "main.c", 67
    ldax.fp __vr2
    sta $28
    stx $29
    lda $28
    clc
    adc #1
    sta $2A
    lda $29
    adc #0
    sta $2B
    lda $2A
    ldx $2B
    stax.fp __vr2
    bra @for_cond5
@for_end8:
    .loc "main.c", 70
    lda #10
    ldx #0
    stax.fp __vr19
    lda.fp __vr0
    sta $28
    stx $29
    ldax.fp __vr19
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
    jsr _set_text_color
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
    .loc "main.c", 71
    lda #14
    ldx #0
    stax.fp __vr20
    lda.fp __vr0
    sta $28
    stx $29
    ldax.fp __vr20
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
    jsr _set_text_color
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
    .func_flags stack_call
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc

; function _main
    proc _main
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler.dev_v1.0.5/bin/../lib/include/cbm.h", 63
; frame: 86 bytes (frame-allocated vRegs only)
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
    .local __vr0 = 2
    .local __vr1 = 0
    .local __vr2 = 6
    .local __vr3 = 10
    .local __vr4 = 8
    .local __vr5 = 4
    .local __vr7 = 16
    .local __vr14 = 18
    .local __vr19 = 20
    .local __vr22 = 22
    .local __vr25 = 24
    .local __vr28 = 26
    .local __vr29 = 28
    .local __vr30 = 14
    .local __vr32 = 12
    .local __vr34 = 30
    .local __vr35 = 32
    .local __vr36 = 34
    .local __vr37 = 36
    .local __vr40 = 42
    .local __vr42 = 44
    .local __vr43 = 46
    .local __vr44 = 48
    .local __vr45 = 50
    .local __vr46 = 52
    .local __vr47 = 54
    .local __vr48 = 40
    .local __vr49 = 56
    .local __vr51 = 58
    .local __vr52 = 60
    .local __vr53 = 62
    .local __vr55 = 64
    .local __vr57 = 66
    .local __vr58 = 68
    .local __vr59 = 70
    .local __vr60 = 72
    .local __vr61 = 74
    .local __vr62 = 76
    .local __vr63 = 78
    .local __vr64 = 80
    .local __vr65 = 82
    .local __vr77 = 84
    .local __vr78 = 38
    .local @_l_msg_len = 0
    .local @_l_msg_pos = 2
    .local @_l_next_char = 4
    .local @_l_pal_idx = 6
    .local @_l_row_ptr = 8
    .local @_l_speed_count = 10
    .local @_l_t = 12
    .local @_l_title = 14

@entry:
    .loc "main.c", 82
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta.fp __vr7
    lda.fp __vr7
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _clear_screen
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
    .loc "main.c", 83
    jsr _init_palette
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
    .loc "main.c", 85
    lda #0
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
    .loc "main.c", 86
    lda #0
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
    .loc "main.c", 88
    lda _message
    ldx _message+1
    stax.fp __vr14
    ldax.fp __vr14
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _strlen
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
    sta $20
    stx $21
    lda $20
    ldx $21
    stax.fp __vr1
    .loc "main.c", 89
    lda #0
    taz
    staz.fp __vr0
    .loc "main.c", 90
    lda #0
    taz
    staz.fp __vr2
    .loc "main.c", 91
    .loc "main.c", 94
    lda #12
    ldx #0
    stax.fp __vr19
    lda #0
    sta $20
    sta $21
    addr_elem.16 __zp_scratch, #_palette, $20, #1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta.fp __vr22
    lda.fp __vr22
    sta $28
    stx $29
    ldax.fp __vr19
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
    jsr _set_text_color
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
    .loc "main.c", 95
    lda #0
    sta $20
    sta $21
    addr_elem.16 __zp_scratch, #_palette, $20, #1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta.fp __vr25
    lda.fp __vr25
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _draw_banner
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
    .loc "main.c", 98
    lda #12
    ldx #0
    sta $20
    stx $21
    lda #2
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    sta.fp __vr28
    lda $22
    ldx $23
    sta.fp __vr29
    lda.fp __vr29
    sta $28
    stx $29
    lda.fp __vr28
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
    jsr _cbm_gotoxy
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
    .loc "main.c", 100
    ldax #__str_9
    sta $20
    stx $21
    lda $20
    ldx $21
    stax.fp __vr30
    .loc "main.c", 102
    lda #0
    taz
    staz.fp __vr32
@for_cond10:
    ldax.fp __vr30
    sta __zp_scratch2
    stx __zp_scratch2+1
    ldax.fp __vr32
    sta __zp_scratch2
    stx __zp_scratch2+1
    addr_elem.16 __zp_scratch, __zp_scratch2, __zp_scratch2, #1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta.fp __vr35
    lda.fp __vr35
    bne @for_body11
    bra @for_end13
@for_body11:
    .loc "main.c", 103
    ldax.fp __vr30
    sta __zp_scratch2
    stx __zp_scratch2+1
    ldax.fp __vr32
    sta __zp_scratch2
    stx __zp_scratch2+1
    addr_elem.16 __zp_scratch, __zp_scratch2, __zp_scratch2, #1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta.fp __vr37
    lda.fp __vr37
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _cbm_chrout
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
@for_inc12:
    .loc "main.c", 102
    ldax.fp __vr32
    sta $20
    stx $21
    lda $20
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    stax.fp __vr32
    bra @for_cond10
@for_end13:
@while_body14_ph:
    .loc "main.c", 121
    lda #40
    ldx #0
    stax.fp __vr78
    ldax.fp __vr78
    sub.16 .AX, #1
    stax.fp __vr48
@while_body14:
    .loc "main.c", 109
    jsr _wait_vsync
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
    jsr _cbm_stop
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
    stax.fp __vr40
    ldax.fp __vr40
    bne @while_end15
    cmp #$00
    bne @while_end15
@if_end18:
    .loc "main.c", 116
    lda #12
    ldx #0
    push .ax
    .var _fp = _fp + 2
    jsr _scroll_line_left
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
    lda _message
    ldx _message+1
    stax.fp __vr42
    ldax.fp __vr42
    sta __zp_scratch2
    stx __zp_scratch2+1
    ldax.fp __vr0
    sta __zp_scratch2
    stx __zp_scratch2+1
    addr_elem.16 __zp_scratch, __zp_scratch2, __zp_scratch2, #1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta.fp __vr44
    lda.fp __vr44
    sta.fp __vr5
    .loc "main.c", 120
    lda #224
    ldx #9
    stax.fp __vr45
    ldax.fp __vr45
    stax.fp __vr4
    .loc "main.c", 121
    lda #40
    ldx #0
    stax.fp __vr46
    lda #1
    ldx #0
    stax.fp __vr47
    lda.fp __vr5
    pha
    ldax.fp __vr4
    sta __zp_scratch2
    stx __zp_scratch2+1
    ldax.fp __vr48
    sta __zp_scratch2
    stx __zp_scratch2+1
    addr_elem.16 __zp_scratch, __zp_scratch2, __zp_scratch2, #1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "main.c", 124
    inc.16f __vr0
    .loc "main.c", 125
    ldax.fp __vr1
    sta __zp_scratch2
    stx __zp_scratch2+1
    ldax.fp __vr0
    cmp.16 .AX, __zp_scratch2
    bcs @if_then19
    bra @if_end21
@if_then19:
    .loc "main.c", 126
    lda #0
    taz
    staz.fp __vr0
@if_end21:
    .loc "main.c", 129
    ldax.fp __vr0
    stx __zp_scratch
    ora __zp_scratch
    beq @if_then22
    bra @while_body14
@if_then22:
    .loc "main.c", 130
    inc.16f __vr2
    .loc "main.c", 131
    lda _palette_len
    ldx _palette_len+1
    stax.fp __vr58
    ldax.fp __vr58
    sta __zp_scratch2
    stx __zp_scratch2+1
    ldax.fp __vr2
    cmp.16 .AX, __zp_scratch2
    bcs @if_then25
    bra @if_end27
@if_then25:
    .loc "main.c", 132
    lda #0
    taz
    staz.fp __vr2
@if_end27:
    .loc "main.c", 133
    lda #12
    ldx #0
    stax.fp __vr61
    ldax.fp __vr2
    sta __zp_scratch2
    stx __zp_scratch2+1
    addr_elem.16 .AX, #_palette, __zp_scratch2, #1
    stax.fp __vr62
    ldax.fp __vr62
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta.fp __vr63
    lda.fp __vr63
    sta $28
    stx $29
    ldax.fp __vr61
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
    jsr _set_text_color
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
    .loc "main.c", 134
    ldax.fp __vr62
    stax.fp __vr64
    ldax.fp __vr62
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta.fp __vr65
    lda.fp __vr65
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _draw_banner
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
    .loc "main.c", 135
    ldax.fp __vr62
    sta $20
    stx $21
    ldax.fp __vr62
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $22
    lda #32
    ldx #208
    sta $24
    stx $25
    lda $24
    ldx $25
    sta $26
    stx $27
    lda $22
    ldy #0
    sta ($26),y
    bra @while_body14_ph
@while_end15:
    .loc "main.c", 140
    lda #14
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
    .loc "main.c", 141
    lda #6
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
    .loc "main.c", 142
    lda #14
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta.fp __vr77
    lda.fp __vr77
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _clear_screen
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
    .loc "main.c", 144
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
    .func_flags stack_call
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc


    .segment "data"
__str_0:
    .text "    welcome to the mega65!    this scroller was built with cc45 -- a c compiler targeting the 45gs02 cpu.    mixed c and assembly makes hardware access easy.    press run/stop to exit...    "
    .byte 0
__str_9:
    .text "text scroller"
    .byte 0

__zp_save_buf:

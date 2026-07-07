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

    .extern _memcpy
    .extern _memset

    .global _screen
    .global _color
    .global _main

    .segment "bss"
_grid:
    .res 1000
_next:
    .res 1000
    .segment "data"
    .byte 0
_screen:
    .word 1024
_color:
    .word 55296

    .segment "code"

; function _clear_grid
    proc _clear_grid
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler.dev_v1.0.5/bin/../lib/include/string.h", 18
    tsx
    txa
    clc
    adc #1
    sta $FD
    lda #$01
    adc #0
    sta $FE

@entry:
    .loc "main.c", 33
    lda #232
    ldx #3
    push .ax
    .var _fp = _fp + 2
    lda #0
    ldx #0
    push .ax
    .var _fp = _fp + 2
    ldax #_grid
    push .ax
    .var _fp = _fp + 2
    jsr _memset
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
    .loc "main.c", 34
    lda #232
    ldx #3
    push .ax
    .var _fp = _fp + 2
    lda #0
    ldx #0
    push .ax
    .var _fp = _fp + 2
    ldax #_next
    push .ax
    .var _fp = _fp + 2
    jsr _memset
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
    .func_flags stack_call
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc

; function _count_neighbors
    proc _count_neighbors, W#@_p_row, W#@_p_col
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler.dev_v1.0.5/bin/../lib/include/string.h", 24
; frame: 14 bytes (frame-allocated vRegs only)
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
    .local __vr0 = 12
    .local __vr1 = 2
    .local __vr2 = 4
    .local __vr3 = 10
    .local __vr4 = 0
    .local __vr5 = 8
    .local __vr6 = 6
    .local @_l_c = 0
    .local @_l_count = 4
    .local @_l_nc = 6
    .local @_l_nr = 8
    .local @_l_r = 10
    .var @_p_row = 16
    .var @_p_col = 18

    ldax.fp @_p_row
    stax.fp __vr0
    ldax.fp @_p_col
    stax.fp __vr1
@entry:
    .loc "main.c", 45
    lda #0
    taz
    staz.fp __vr2
    .loc "main.c", 46
    lda #255
    taz
    staz.fp __vr3
@for_cond3:
    ldax.fp __vr3
    cmp.16 .AX, #1
    bcc @for_body4
    beq @for_body4
    bra @for_end6
@for_body4:
    .loc "main.c", 47
    lda #255
    taz
    staz.fp __vr4
@for_cond7_ph:
    .loc "main.c", 48
    ldax.fp __vr3
    stx __zp_scratch
    ora __zp_scratch
    beq @__cmp_set_0
    bra @__cmp_zero_0
@__cmp_set_0:
    lda #1
    ldx #0
    bra @__cmp_done_0
@__cmp_zero_0:
    lda #0
    ldx #0
@__cmp_done_0:
    sta $28
    .loc "main.c", 49
    ldax.fp __vr3
    sta __zp_scratch2
    stx __zp_scratch2+1
    ldax.fp __vr0
    add.16 .AX, __zp_scratch2
    sta $2A
    stx $2B
@for_cond7:
    .loc "main.c", 47
    ldax.fp __vr4
    cmp.16 .AX, #1
    bcc @for_body8
    beq @for_body8
    bra @for_end10
@for_body8:
    .loc "main.c", 48
    lda #0
    sta $30
    sta $31
    lda $28
    bne @and_rhs14
    bra @if_end13
@and_rhs14:
    ldax.fp __vr4
    stx __zp_scratch
    ora __zp_scratch
    beq @for_inc9
@if_end13:
    .loc "main.c", 49
    lda $2A
    ldx $2B
    stax.fp __vr5
    .loc "main.c", 50
    ldax.fp __vr4
    sta __zp_scratch2
    stx __zp_scratch2+1
    ldax.fp __vr1
    add.16 .AX, __zp_scratch2
    sta $36
    stx $37
    stax.fp __vr6
    .loc "main.c", 52
    ldax.fp __vr5
    cmp.16 .AX, #0
    bcc @if_then15
    bra @if_end17
@if_then15:
    lda #24
    ldz #0
    staz.fp __vr5
@if_end17:
    .loc "main.c", 53
    ldax.fp __vr5
    cmp.16 .AX, #25
    bcs @if_then18
    bra @if_end20
@if_then18:
    lda #0
    taz
    staz.fp __vr5
@if_end20:
    .loc "main.c", 54
    ldax.fp __vr6
    cmp.16 .AX, #0
    bcc @if_then21
    bra @if_end23
@if_then21:
    lda #39
    ldz #0
    staz.fp __vr6
@if_end23:
    .loc "main.c", 55
    ldax.fp __vr6
    cmp.16 .AX, #40
    bcs @if_then24
    bra @if_end26
@if_then24:
    lda #0
    taz
    staz.fp __vr6
@if_end26:
    .loc "main.c", 56
    ldax.fp __vr5
    sta __zp_scratch2
    stx __zp_scratch2+1
    addr_elem.16 $50, #_grid, __zp_scratch2, #40
    ldax.fp __vr6
    sta __zp_scratch2
    stx __zp_scratch2+1
    addr_elem.16 __zp_scratch, $50, __zp_scratch2, #1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $54
    lda $54
    bne @if_then27
    bra @if_end29
@if_then27:
    ldax.fp __vr2
    sta $56
    stx $57
    lda $56
    clc
    adc #1
    sta $58
    lda $57
    adc #0
    sta $59
    lda $58
    ldx $59
    stax.fp __vr2
@if_end29:
@for_inc9:
    .loc "main.c", 47
    ldax.fp __vr4
    sta $5A
    stx $5B
    lda $5A
    clc
    adc #1
    sta $5C
    lda $5B
    adc #0
    sta $5D
    lda $5C
    ldx $5D
    stax.fp __vr4
    bra @for_cond7
@for_end10:
@for_inc5:
    .loc "main.c", 46
    ldax.fp __vr3
    sta $5E
    stx $5F
    lda $5E
    clc
    adc #1
    sta $60
    lda $5F
    adc #0
    sta $61
    lda $60
    ldx $61
    stax.fp __vr3
    bra @for_cond3
@for_end6:
    .loc "main.c", 59
    ldax.fp __vr2
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
    .func_flags stack_call, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    endproc

; function _step
    proc _step
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler.dev_v1.0.5/bin/../lib/include/mega65.h", 10
; frame: 22 bytes (frame-allocated vRegs only)
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
    .local __vr0 = 4
    .local __vr1 = 0
    .local __vr2 = 2
    .local __vr5 = 6
    .local __vr6 = 8
    .local __vr8 = 20
    .local __vr10 = 10
    .local __vr18 = 12
    .local __vr21 = 14
    .local __vr26 = 16
    .local __vr29 = 18
    .local @_l_c = 0
    .local @_l_n = 2
    .local @_l_r = 4

@entry:
    .loc "main.c", 67
    lda #0
    taz
    staz.fp __vr0
@for_cond30:
    ldax.fp __vr0
    cmp.16 .AX, #25
    bcc @for_body31
    bra @for_end33
@for_body31:
    .loc "main.c", 68
    lda #0
    taz
    staz.fp __vr1
@for_cond34_ph:
    .loc "main.c", 70
    ldax.fp __vr0
    sta __zp_scratch2
    stx __zp_scratch2+1
    addr_elem.16 .AX, #_grid, __zp_scratch2, #40
    stax.fp __vr10
    .loc "main.c", 73
    ldax.fp __vr0
    sta __zp_scratch2
    stx __zp_scratch2+1
    addr_elem.16 .AX, #_next, __zp_scratch2, #40
    stax.fp __vr18
    .loc "main.c", 75
    ldax.fp __vr0
    sta __zp_scratch2
    stx __zp_scratch2+1
    addr_elem.16 .AX, #_next, __zp_scratch2, #40
    stax.fp __vr21
    .loc "main.c", 79
    ldax.fp __vr0
    sta __zp_scratch2
    stx __zp_scratch2+1
    addr_elem.16 .AX, #_next, __zp_scratch2, #40
    stax.fp __vr26
    .loc "main.c", 81
    ldax.fp __vr0
    sta __zp_scratch2
    stx __zp_scratch2+1
    addr_elem.16 .AX, #_next, __zp_scratch2, #40
    stax.fp __vr29
@for_cond34:
    .loc "main.c", 68
    ldax.fp __vr1
    cmp.16 .AX, #40
    bcc @for_body35
    bra @for_end37
@for_body35:
    .loc "main.c", 69
    ldax.fp __vr1
    sta $28
    stx $29
    ldax.fp __vr0
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
    jsr _count_neighbors
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
    sta $20
    stx $21
    lda $20
    ldx $21
    stax.fp __vr2
    .loc "main.c", 70
    ldax.fp __vr10
    sta __zp_scratch2
    stx __zp_scratch2+1
    ldax.fp __vr1
    sta __zp_scratch2
    stx __zp_scratch2+1
    addr_elem.16 __zp_scratch, __zp_scratch2, __zp_scratch2, #1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $24
    lda $24
    bne @if_then38
    bra @if_else39
@if_then38:
    .loc "main.c", 72
    ldax.fp __vr2
    cmp.16 .AX, #2
    beq @if_then41
@or_rhs44:
    ldax.fp __vr2
    cmp.16 .AX, #3
    beq @if_then41
    bra @if_else42
@if_then41:
    .loc "main.c", 73
    lda #1
    sta $2E
    lda $2E
    ldx #0
    pha
    ldax.fp __vr18
    sta __zp_scratch2
    stx __zp_scratch2+1
    ldax.fp __vr1
    sta __zp_scratch2
    stx __zp_scratch2+1
    addr_elem.16 __zp_scratch, __zp_scratch2, __zp_scratch2, #1
    pla
    ldy #0
    sta (__zp_scratch),y
    bra @if_end40
@if_else42:
    .loc "main.c", 75
    lda #0
    sta $32
    lda $32
    ldx #0
    pha
    ldax.fp __vr21
    sta __zp_scratch2
    stx __zp_scratch2+1
    ldax.fp __vr1
    sta __zp_scratch2
    stx __zp_scratch2+1
    addr_elem.16 __zp_scratch, __zp_scratch2, __zp_scratch2, #1
    pla
    ldy #0
    sta (__zp_scratch),y
    bra @if_end40
@if_else39:
    .loc "main.c", 78
    ldax.fp __vr2
    cmp.16 .AX, #3
    beq @if_then45
    bra @if_else46
@if_then45:
    .loc "main.c", 79
    lda #1
    sta $3A
    lda $3A
    ldx #0
    pha
    ldax.fp __vr26
    sta __zp_scratch2
    stx __zp_scratch2+1
    ldax.fp __vr1
    sta __zp_scratch2
    stx __zp_scratch2+1
    addr_elem.16 __zp_scratch, __zp_scratch2, __zp_scratch2, #1
    pla
    ldy #0
    sta (__zp_scratch),y
    bra @if_end40
@if_else46:
    .loc "main.c", 81
    lda #0
    sta $3E
    lda $3E
    ldx #0
    pha
    ldax.fp __vr29
    sta __zp_scratch2
    stx __zp_scratch2+1
    ldax.fp __vr1
    sta __zp_scratch2
    stx __zp_scratch2+1
    addr_elem.16 __zp_scratch, __zp_scratch2, __zp_scratch2, #1
    pla
    ldy #0
    sta (__zp_scratch),y
@if_end40:
@for_inc36:
    .loc "main.c", 68
    ldax.fp __vr1
    sta $42
    stx $43
    lda $42
    clc
    adc #1
    sta $44
    lda $43
    adc #0
    sta $45
    lda $44
    ldx $45
    stax.fp __vr1
    bra @for_cond34
@for_end37:
@for_inc32:
    .loc "main.c", 67
    ldax.fp __vr0
    sta $46
    stx $47
    lda $46
    clc
    adc #1
    sta $48
    lda $47
    adc #0
    sta $49
    lda $48
    ldx $49
    stax.fp __vr0
    bra @for_cond30
@for_end33:
    .loc "main.c", 87
    lda #232
    ldx #3
    push .ax
    .var _fp = _fp + 2
    ldax #_next
    push .ax
    .var _fp = _fp + 2
    ldax #_grid
    push .ax
    .var _fp = _fp + 2
    jsr _memcpy
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

; function _draw
    proc _draw
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler.dev_v1.0.5/bin/../lib/include/mega65.h", 38
; frame: 6 bytes (frame-allocated vRegs only)
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
    .local __vr0 = 4
    .local __vr1 = 0
    .local __vr2 = 2
    .local @_l_c = 0
    .local @_l_offset = 2
    .local @_l_r = 4

@entry:
    .loc "main.c", 95
    lda #0
    taz
    staz.fp __vr0
@for_cond48:
    ldax.fp __vr0
    cmp.16 .AX, #25
    bcc @for_body49
    bra @for_end51
@for_body49:
    .loc "main.c", 96
    lda #0
    taz
    staz.fp __vr1
@for_cond52_ph:
    .loc "main.c", 97
    lda #40
    ldx #0
    sta $26
    stx $27
    ldax.fp __vr0
    mul.16 .AX, $26
    sta $28
    stx $29
    .loc "main.c", 98
    ldax.fp __vr0
    sta __zp_scratch2
    stx __zp_scratch2+1
    addr_elem.16 $2A, #_grid, __zp_scratch2, #40
@for_cond52:
    .loc "main.c", 96
    ldax.fp __vr1
    cmp.16 .AX, #40
    bcc @for_body53
    bra @for_end55
@for_body53:
    .loc "main.c", 97
    lda #40
    ldx #0
    sta $30
    stx $31
    ldax.fp __vr1
    add.16 .AX, $28
    sta $32
    stx $33
    stax.fp __vr2
    .loc "main.c", 98
    ldax.fp __vr1
    sta __zp_scratch2
    stx __zp_scratch2+1
    addr_elem.16 __zp_scratch, $2A, __zp_scratch2, #1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $36
    lda $36
    bne @if_then56
    bra @if_else57
@if_then56:
    .loc "main.c", 99
    lda #42
    sta $38
    lda _screen
    ldx _screen+1
    sta $3A
    stx $3B
    lda $38
    ldx #0
    pha
    ldax.fp __vr2
    sta __zp_scratch2
    stx __zp_scratch2+1
    addr_elem.16 __zp_scratch, $3A, __zp_scratch2, #1
    pla
    ldy #0
    sta (__zp_scratch),y
    bra @if_end58
@if_else57:
    .loc "main.c", 101
    lda #32
    sta $3E
    lda _screen
    ldx _screen+1
    sta $40
    stx $41
    lda $3E
    ldx #0
    pha
    ldax.fp __vr2
    sta __zp_scratch2
    stx __zp_scratch2+1
    addr_elem.16 __zp_scratch, $40, __zp_scratch2, #1
    pla
    ldy #0
    sta (__zp_scratch),y
@if_end58:
@for_inc54:
    .loc "main.c", 96
    ldax.fp __vr1
    sta $44
    stx $45
    lda $44
    clc
    adc #1
    sta $46
    lda $45
    adc #0
    sta $47
    lda $46
    ldx $47
    stax.fp __vr1
    bra @for_cond52
@for_end55:
@for_inc50:
    .loc "main.c", 95
    ldax.fp __vr0
    sta $48
    stx $49
    lda $48
    clc
    adc #1
    sta $4A
    lda $49
    adc #0
    sta $4B
    lda $4A
    ldx $4B
    stax.fp __vr0
    bra @for_cond48
@for_end51:
@__return:
    plz
    plz
    plz
    plz
    plz
    plz
    .func_flags stack_call, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    endproc

; function _delay
    proc _delay
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler.dev_v1.0.5/bin/../lib/include/mega65.h", 54
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
    .local @_l_i = 0

@entry:
    .loc "main.c", 108
    lda #0
    taz
    staz.fp __vr0
@for_cond59:
    ldax.fp __vr0
    cmp.16 .AX, #2000
    bcc @for_body60
    bra @for_end62
@for_body60:
@for_inc61:
    ldax.fp __vr0
    sta $24
    stx $25
    lda $24
    clc
    adc #1
    sta $26
    lda $25
    adc #0
    sta $27
    lda $26
    ldx $27
    stax.fp __vr0
    bra @for_cond59
@for_end62:
@__return:
    plz
    plz
    .func_flags stack_call, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    endproc

; function _seed_r_pentomino
    proc _seed_r_pentomino, W#@_p_row, W#@_p_col
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler.dev_v1.0.5/bin/../lib/include/mega65.h", 62
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
    .local __vr0 = 2
    .local __vr1 = 0
    .var @_p_row = 6
    .var @_p_col = 8

    ldax.fp @_p_row
    stax.fp __vr0
    ldax.fp @_p_col
    stax.fp __vr1
@entry:
    .loc "main.c", 115
    lda #1
    sta $20
    ldax.fp __vr0
    sta __zp_scratch2
    stx __zp_scratch2+1
    addr_elem.16 $22, #_grid, __zp_scratch2, #40
    ldax.fp __vr1
    add.16 .AX, #1
    sta $26
    stx $27
    lda $20
    ldx #0
    pha
    addr_elem.16 __zp_scratch, $22, $26, #1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "main.c", 116
    lda #1
    sta $20
    lda $22
    ldx $23
    sta $24
    stx $25
    ldax.fp __vr1
    add.16 .AX, #2
    sta $26
    stx $27
    lda $20
    ldx #0
    pha
    addr_elem.16 __zp_scratch, $22, $26, #1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "main.c", 117
    lda #1
    sta $20
    ldax.fp __vr0
    add.16 .AX, #1
    sta $24
    stx $25
    addr_elem.16 $22, #_grid, $24, #40
    lda $20
    ldx #0
    pha
    ldax.fp __vr1
    sta __zp_scratch2
    stx __zp_scratch2+1
    addr_elem.16 __zp_scratch, $22, __zp_scratch2, #1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "main.c", 118
    lda #1
    sta $20
    ldax.fp __vr0
    add.16 .AX, #1
    sta $24
    stx $25
    addr_elem.16 $22, #_grid, $24, #40
    ldax.fp __vr1
    add.16 .AX, #1
    sta $26
    stx $27
    lda $20
    ldx #0
    pha
    addr_elem.16 __zp_scratch, $22, $26, #1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "main.c", 119
    lda #1
    sta $20
    ldax.fp __vr0
    add.16 .AX, #2
    sta $24
    stx $25
    addr_elem.16 $22, #_grid, $24, #40
    ldax.fp __vr1
    add.16 .AX, #1
    sta $26
    stx $27
    lda $20
    ldx #0
    pha
    addr_elem.16 __zp_scratch, $22, $26, #1
    pla
    ldy #0
    sta (__zp_scratch),y
@__return:
    plz
    plz
    plz
    plz
    .func_flags stack_call, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    endproc

; function _seed_glider
    proc _seed_glider, W#@_p_row, W#@_p_col
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler.dev_v1.0.5/bin/../lib/include/mega65.h", 71
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
    .local __vr0 = 2
    .local __vr1 = 0
    .var @_p_row = 6
    .var @_p_col = 8

    ldax.fp @_p_row
    stax.fp __vr0
    ldax.fp @_p_col
    stax.fp __vr1
@entry:
    .loc "main.c", 124
    lda #1
    sta $20
    ldax.fp __vr0
    sta __zp_scratch2
    stx __zp_scratch2+1
    addr_elem.16 $22, #_grid, __zp_scratch2, #40
    ldax.fp __vr1
    add.16 .AX, #1
    sta $26
    stx $27
    lda $20
    ldx #0
    pha
    addr_elem.16 __zp_scratch, $22, $26, #1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "main.c", 125
    lda #1
    sta $20
    ldax.fp __vr0
    add.16 .AX, #1
    sta $24
    stx $25
    addr_elem.16 $22, #_grid, $24, #40
    ldax.fp __vr1
    add.16 .AX, #2
    sta $26
    stx $27
    lda $20
    ldx #0
    pha
    addr_elem.16 __zp_scratch, $22, $26, #1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "main.c", 126
    lda #1
    sta $20
    ldax.fp __vr0
    add.16 .AX, #2
    sta $24
    stx $25
    addr_elem.16 $22, #_grid, $24, #40
    lda $20
    ldx #0
    pha
    ldax.fp __vr1
    sta __zp_scratch2
    stx __zp_scratch2+1
    addr_elem.16 __zp_scratch, $22, __zp_scratch2, #1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "main.c", 127
    lda #1
    sta $20
    ldax.fp __vr0
    add.16 .AX, #2
    sta $24
    stx $25
    addr_elem.16 $22, #_grid, $24, #40
    ldax.fp __vr1
    add.16 .AX, #1
    sta $26
    stx $27
    lda $20
    ldx #0
    pha
    addr_elem.16 __zp_scratch, $22, $26, #1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "main.c", 128
    lda #1
    sta $20
    ldax.fp __vr0
    add.16 .AX, #2
    sta $24
    stx $25
    addr_elem.16 $22, #_grid, $24, #40
    ldax.fp __vr1
    add.16 .AX, #2
    sta $26
    stx $27
    lda $20
    ldx #0
    pha
    addr_elem.16 __zp_scratch, $22, $26, #1
    pla
    ldy #0
    sta (__zp_scratch),y
@__return:
    plz
    plz
    plz
    plz
    .func_flags stack_call, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    endproc

; function _main
    proc _main
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler.dev_v1.0.5/bin/../lib/include/mega65.h", 79
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
    .local __vr31 = 4
    .local __vr32 = 6
    .local __vr33 = 8
    .local __vr34 = 2
    .local @_l_gen = 0

@entry:
    .loc "main.c", 134
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
    .loc "main.c", 135
    jsr _clear_grid
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
    .loc "main.c", 136
    lda #2
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
    .loc "main.c", 139
    lda #3
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
    .loc "main.c", 140
    lda #18
    ldx #0
    push .ax
    .var _fp = _fp + 2
    lda #10
    ldx #0
    push .ax
    .var _fp = _fp + 2
    jsr _seed_r_pentomino
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
    .loc "main.c", 141
    lda _COLOR_GRAY
    ldx _COLOR_GRAY+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda #32
    ldx #208
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $24
    stx $25
    lda $22
    ldy #0
    sta ($24),y
    .loc "main.c", 142
    lda #2
    ldx #0
    push .ax
    .var _fp = _fp + 2
    lda #2
    ldx #0
    push .ax
    .var _fp = _fp + 2
    jsr _seed_glider
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
    .loc "main.c", 143
    lda #5
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
    .loc "main.c", 144
    lda #30
    ldx #0
    push .ax
    .var _fp = _fp + 2
    lda #3
    ldx #0
    push .ax
    .var _fp = _fp + 2
    jsr _seed_glider
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
    .loc "main.c", 145
    lda #3
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
    .loc "main.c", 150
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
    .loc "main.c", 151
    lda #0
    taz
    staz.fp __vr0
@for_cond63_ph:
    .loc "main.c", 152
    lda #32
    ldx #208
    sta $20
    stx $21
    lda $20
    ldx $21
    stax.fp __vr34
@for_cond63:
    .loc "main.c", 151
    ldax.fp __vr0
    cmp.16 .AX, #500
    bcc @for_body64
    bra @for_end66
@for_body64:
    .loc "main.c", 152
    lda #1
    sta.fp __vr32
    lda #32
    ldx #208
    stax.fp __vr33
    ldax.fp __vr34
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda.fp __vr32
    ldy #0
    sta (__zp_scratch2),y
    .loc "main.c", 153
    jsr _draw
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
    .loc "main.c", 154
    jsr _delay
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
    .loc "main.c", 155
    jsr _step
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
@for_inc65:
    .loc "main.c", 151
    ldax.fp __vr0
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
    stax.fp __vr0
    bra @for_cond63
@for_end66:
    .loc "main.c", 157
    lda #9
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
    .loc "main.c", 159
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
    .func_flags stack_call
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc


__zp_save_buf:

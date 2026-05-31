    .o45
    .extern __sp_base
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
_screen:
    .word 1024
_color:
    .word 55296

    .segment "code"

; function _clear_grid
    proc _clear_grid
    .var _fp = 0
    .loc "main.c", 32

@entry:
    .loc "main.c", 33
    phw #1000
    .var _fp = _fp + 2
    phw #0
    .var _fp = _fp + 2
    phw #_grid
    .var _fp = _fp + 2
    jsr _memset
    .var _fp = _fp - 6
    .loc "main.c", 34
    phw #1000
    .var _fp = _fp + 2
    phw #0
    .var _fp = _fp + 2
    phw #_next
    .var _fp = _fp + 2
    jsr _memset
    .var _fp = _fp - 6
@__return:
    .func_flags stack_call
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc

; function _count_neighbors
    proc _count_neighbors, W#@_p_row, W#@_p_col
    .var _fp = 0
    .loc "main.c", 38
    .var @_l_c = $2A
    .var @_l_count = $2E
    .var @_l_nc = $26
    .var @_l_nr = $28
    .var @_l_r = $2C
    .var @_p_row = 2
    .var @_p_col = 4

    ldax.fp @_p_row
    sta $22
    stx $23
    ldax.fp @_p_col
    sta $20
    stx $21
@entry:
    .loc "main.c", 45
    lda #0
    sta $2E
    sta $2F
    .loc "main.c", 46
    lda #255
    sta $2C
    sta $2D
@for_cond3:
    lda $2C
    ldx $2D
    cmp.16 .AX, #1
    bcc @for_body4
    beq @for_body4
    bra @for_end6
@for_body4:
    .loc "main.c", 47
    lda #255
    sta $2A
    sta $2B
@for_cond7:
    lda $2A
    ldx $2B
    cmp.16 .AX, #1
    bcc @for_body8
    beq @for_body8
    bra @for_end10
@for_body8:
    .loc "main.c", 48
    lda $2C
    ora $2D
    beq @and_rhs14
    bra @if_end13
@and_rhs14:
    lda $2A
    ora $2B
    beq @if_then11
    bra @if_end13
@if_then11:
    bra @for_inc9
@if_end13:
    .loc "main.c", 49
    lda $22
    clc
    adc $2C
    sta $28
    lda $23
    adc $2C+1
    sta $29
    .loc "main.c", 50
    lda $20
    clc
    adc $2A
    sta $26
    lda $21
    adc $2A+1
    sta $27
    .loc "main.c", 52
    lda $28
    ldx $29
    cmp.16 .AX, #0
    bcc @if_then15
    bra @if_end17
@if_then15:
    lda #24
    sta $28
    ldx #0
    stx $29
@if_end17:
    .loc "main.c", 53
    lda $28
    ldx $29
    cmp.16 .AX, #25
    bcs @if_then18
    bra @if_end20
@if_then18:
    lda #0
    sta $28
    sta $29
@if_end20:
    .loc "main.c", 54
    lda $26
    ldx $27
    cmp.16 .AX, #0
    bcc @if_then21
    bra @if_end23
@if_then21:
    lda #39
    sta $26
    ldx #0
    stx $27
@if_end23:
    .loc "main.c", 55
    lda $26
    ldx $27
    cmp.16 .AX, #40
    bcs @if_then24
    bra @if_end26
@if_then24:
    lda #0
    sta $26
    sta $27
@if_end26:
    .loc "main.c", 56
    lda #<_grid
    ldx #>_grid
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $28
    ldx $29
    mul.16 .AX, #40
    clc
    adc __zp_scratch2
    sta $24
    txa
    adc __zp_scratch2+1
    sta $25
    lda $26
    ldx $27
    clc
    adc $24
    sta $30
    txa
    adc $24+1
    sta $31
    ldy #0
    lda ($30),y
    ldx #0
    sta $24
    lda $24
    bne @if_then27
    bra @if_end29
@if_then27:
    inc $2E
    bne *+4
    inc $2F
@if_end29:
@for_inc9:
    .loc "main.c", 47
    inc $2A
    bne *+4
    inc $2B
    bra @for_cond7
@for_end10:
@for_inc5:
    .loc "main.c", 46
    inc $2C
    bne *+4
    inc $2D
    bra @for_cond3
@for_end6:
    .loc "main.c", 59
    lda $2E
    ldx $2F
@__return:
    .func_flags stack_call, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    endproc

; function _step
    proc _step
    .var _fp = 0
    .loc "main.c", 62
; frame: 6 bytes (frame-allocated vRegs only)
    phw #0
    phw #0
    phw #0
    .local __vr0 = 2
    .local __vr1 = 0
    .local __vr2 = 4
    .local @_l_c = 0
    .local @_l_n = 4
    .local @_l_r = 2

@entry:
    .loc "main.c", 67
    lda #0
    sta $20
    sta $21
    stax.fp __vr0
@for_cond30:
    ldax.fp __vr0
    cmp.16 .AX, #25
    bcc @for_body31
    bra @for_end33
@for_body31:
    .loc "main.c", 68
    lda #0
    sta $20
    sta $21
    stax.fp __vr1
@for_cond34:
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
    jsr _count_neighbors
    .var _fp = _fp - 4
    sta $20
    stx $21
    lda $20
    ldx $21
    stax.fp __vr2
    .loc "main.c", 70
    lda #<_grid
    ldx #>_grid
    sta __zp_scratch2
    stx __zp_scratch2+1
    ldax.fp __vr0
    mul.16 .AX, #40
    clc
    adc __zp_scratch2
    sta $20
    txa
    adc __zp_scratch2+1
    sta $21
    ldax.fp __vr1
    clc
    adc $20
    sta $22
    txa
    adc $20+1
    sta $23
    ldy #0
    lda ($22),y
    ldx #0
    sta $20
    lda $20
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
    sta $20
    lda #<_next
    ldx #>_next
    sta __zp_scratch2
    stx __zp_scratch2+1
    ldax.fp __vr0
    mul.16 .AX, #40
    clc
    adc __zp_scratch2
    sta $22
    txa
    adc __zp_scratch2+1
    sta $23
    ldax.fp __vr1
    clc
    adc $22
    sta $24
    txa
    adc $22+1
    sta $25
    lda $20
    ldy #0
    sta ($24),y
    bra @if_end43
@if_else42:
    .loc "main.c", 75
    lda #0
    sta $20
    lda #<_next
    ldx #>_next
    sta __zp_scratch2
    stx __zp_scratch2+1
    ldax.fp __vr0
    mul.16 .AX, #40
    clc
    adc __zp_scratch2
    sta $22
    txa
    adc __zp_scratch2+1
    sta $23
    ldax.fp __vr1
    clc
    adc $22
    sta $24
    txa
    adc $22+1
    sta $25
    lda $20
    ldy #0
    sta ($24),y
@if_end43:
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
    sta $20
    lda #<_next
    ldx #>_next
    sta __zp_scratch2
    stx __zp_scratch2+1
    ldax.fp __vr0
    mul.16 .AX, #40
    clc
    adc __zp_scratch2
    sta $22
    txa
    adc __zp_scratch2+1
    sta $23
    ldax.fp __vr1
    clc
    adc $22
    sta $24
    txa
    adc $22+1
    sta $25
    lda $20
    ldy #0
    sta ($24),y
    bra @if_end47
@if_else46:
    .loc "main.c", 81
    lda #0
    sta $20
    lda #<_next
    ldx #>_next
    sta __zp_scratch2
    stx __zp_scratch2+1
    ldax.fp __vr0
    mul.16 .AX, #40
    clc
    adc __zp_scratch2
    sta $22
    txa
    adc __zp_scratch2+1
    sta $23
    ldax.fp __vr1
    clc
    adc $22
    sta $24
    txa
    adc $22+1
    sta $25
    lda $20
    ldy #0
    sta ($24),y
@if_end47:
@if_end40:
@for_inc36:
    .loc "main.c", 68
    inc.16f __vr1
    bra @for_cond34
@for_end37:
@for_inc32:
    .loc "main.c", 67
    inc.16f __vr0
    bra @for_cond30
@for_end33:
    .loc "main.c", 87
    phw #1000
    .var _fp = _fp + 2
    phw #_next
    .var _fp = _fp + 2
    phw #_grid
    .var _fp = _fp + 2
    jsr _memcpy
    .var _fp = _fp - 6
@__return:
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
    .loc "main.c", 90
    .var @_l_c = $20
    .var @_l_offset = $22
    .var @_l_r = $26

@entry:
    .loc "main.c", 95
    lda #0
    sta $26
    sta $27
@for_cond48:
    lda $26
    ldx $27
    cmp.16 .AX, #25
    bcc @for_body49
    bra @for_end51
@for_body49:
    .loc "main.c", 96
    lda #0
    sta $20
    sta $21
@for_cond52:
    lda $20
    ldx $21
    cmp.16 .AX, #40
    bcc @for_body53
    bra @for_end55
@for_body53:
    .loc "main.c", 97
    lda #40
    ldx #0
    sta $24
    stx $25
    lda $26
    ldx $27
    mul.16 .AX, $24
    sta $28
    stx $29
    lda $28
    clc
    adc $20
    sta $22
    lda $29
    adc $20+1
    sta $23
    .loc "main.c", 98
    lda #<_grid
    ldx #>_grid
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $26
    ldx $27
    mul.16 .AX, #40
    clc
    adc __zp_scratch2
    sta $24
    txa
    adc __zp_scratch2+1
    sta $25
    lda $20
    ldx $21
    clc
    adc $24
    sta $28
    txa
    adc $24+1
    sta $29
    ldy #0
    lda ($28),y
    ldx #0
    sta $24
    lda $24
    bne @if_then56
    bra @if_else57
@if_then56:
    .loc "main.c", 99
    lda #42
    sta $24
    lda _screen
    ldx _screen+1
    sta $28
    stx $29
    lda $22
    ldx $23
    clc
    adc $28
    sta $2A
    txa
    adc $28+1
    sta $2B
    lda $24
    ldy #0
    sta ($2A),y
    bra @if_end58
@if_else57:
    .loc "main.c", 101
    lda #32
    sta $24
    lda _screen
    ldx _screen+1
    sta $28
    stx $29
    lda $22
    ldx $23
    clc
    adc $28
    sta $2A
    txa
    adc $28+1
    sta $2B
    lda $24
    ldy #0
    sta ($2A),y
@if_end58:
@for_inc54:
    .loc "main.c", 96
    inc $20
    bne *+4
    inc $21
    bra @for_cond52
@for_end55:
@for_inc50:
    .loc "main.c", 95
    inc $26
    bne *+4
    inc $27
    bra @for_cond48
@for_end51:
@__return:
    .func_flags stack_call, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    endproc

; function _delay
    proc _delay
    .var _fp = 0
    .loc "main.c", 106
    .var @_l_i = $20

@entry:
    .loc "main.c", 108
    lda #0
    sta $20
    sta $21
@for_cond59:
    lda $20
    ldx $21
    cmp.16 .AX, #2000
    bcc @for_body60
    bra @for_end62
@for_body60:
@for_inc61:
    inc $20
    bne *+4
    inc $21
    bra @for_cond59
@for_end62:
@__return:
    .func_flags stack_call, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    endproc

; function _seed_r_pentomino
    proc _seed_r_pentomino, W#@_p_row, W#@_p_col
    .var _fp = 0
    .loc "main.c", 114
    .var @_p_row = 2
    .var @_p_col = 4

    ldax.fp @_p_row
    sta $24
    stx $25
    ldax.fp @_p_col
    sta $20
    stx $21
@entry:
    .loc "main.c", 115
    lda #1
    sta $22
    lda #<_grid
    ldx #>_grid
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $24
    ldx $25
    mul.16 .AX, #40
    clc
    adc __zp_scratch2
    sta $26
    txa
    adc __zp_scratch2+1
    sta $27
    lda $20
    clc
    adc #1
    sta $2A
    lda $21
    adc #0
    sta $2B
    lda $2A
    ldx $2B
    clc
    adc $26
    sta $28
    txa
    adc $26+1
    sta $29
    lda $22
    ldy #0
    sta ($28),y
    .loc "main.c", 116
    lda #1
    sta $22
    lda #<_grid
    ldx #>_grid
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $24
    ldx $25
    mul.16 .AX, #40
    clc
    adc __zp_scratch2
    sta $26
    txa
    adc __zp_scratch2+1
    sta $27
    lda $20
    clc
    adc #2
    sta $2A
    lda $21
    adc #0
    sta $2B
    lda $2A
    ldx $2B
    clc
    adc $26
    sta $28
    txa
    adc $26+1
    sta $29
    lda $22
    ldy #0
    sta ($28),y
    .loc "main.c", 117
    lda #1
    sta $22
    lda $24
    clc
    adc #1
    sta $28
    lda $25
    adc #0
    sta $29
    lda #<_grid
    ldx #>_grid
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $28
    ldx $29
    mul.16 .AX, #40
    clc
    adc __zp_scratch2
    sta $26
    txa
    adc __zp_scratch2+1
    sta $27
    lda $20
    ldx $21
    clc
    adc $26
    sta $28
    txa
    adc $26+1
    sta $29
    lda $22
    ldy #0
    sta ($28),y
    .loc "main.c", 118
    lda #1
    sta $22
    lda $24
    clc
    adc #1
    sta $28
    lda $25
    adc #0
    sta $29
    lda #<_grid
    ldx #>_grid
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $28
    ldx $29
    mul.16 .AX, #40
    clc
    adc __zp_scratch2
    sta $26
    txa
    adc __zp_scratch2+1
    sta $27
    lda $20
    clc
    adc #1
    sta $2A
    lda $21
    adc #0
    sta $2B
    lda $2A
    ldx $2B
    clc
    adc $26
    sta $28
    txa
    adc $26+1
    sta $29
    lda $22
    ldy #0
    sta ($28),y
    .loc "main.c", 119
    lda #1
    sta $22
    lda $24
    clc
    adc #2
    sta $28
    lda $25
    adc #0
    sta $29
    lda #<_grid
    ldx #>_grid
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $28
    ldx $29
    mul.16 .AX, #40
    clc
    adc __zp_scratch2
    sta $26
    txa
    adc __zp_scratch2+1
    sta $27
    lda $20
    clc
    adc #1
    sta $2A
    lda $21
    adc #0
    sta $2B
    lda $2A
    ldx $2B
    clc
    adc $26
    sta $28
    txa
    adc $26+1
    sta $29
    lda $22
    ldy #0
    sta ($28),y
@__return:
    .func_flags stack_call, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    endproc

; function _seed_glider
    proc _seed_glider, W#@_p_row, W#@_p_col
    .var _fp = 0
    .loc "main.c", 123
    .var @_p_row = 2
    .var @_p_col = 4

    ldax.fp @_p_row
    sta $22
    stx $23
    ldax.fp @_p_col
    sta $20
    stx $21
@entry:
    .loc "main.c", 124
    lda #1
    sta $24
    lda #<_grid
    ldx #>_grid
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $22
    ldx $23
    mul.16 .AX, #40
    clc
    adc __zp_scratch2
    sta $26
    txa
    adc __zp_scratch2+1
    sta $27
    lda $20
    clc
    adc #1
    sta $2A
    lda $21
    adc #0
    sta $2B
    lda $2A
    ldx $2B
    clc
    adc $26
    sta $28
    txa
    adc $26+1
    sta $29
    lda $24
    ldy #0
    sta ($28),y
    .loc "main.c", 125
    lda #1
    sta $24
    lda $22
    clc
    adc #1
    sta $28
    lda $23
    adc #0
    sta $29
    lda #<_grid
    ldx #>_grid
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $28
    ldx $29
    mul.16 .AX, #40
    clc
    adc __zp_scratch2
    sta $26
    txa
    adc __zp_scratch2+1
    sta $27
    lda $20
    clc
    adc #2
    sta $2A
    lda $21
    adc #0
    sta $2B
    lda $2A
    ldx $2B
    clc
    adc $26
    sta $28
    txa
    adc $26+1
    sta $29
    lda $24
    ldy #0
    sta ($28),y
    .loc "main.c", 126
    lda #1
    sta $24
    lda $22
    clc
    adc #2
    sta $28
    lda $23
    adc #0
    sta $29
    lda #<_grid
    ldx #>_grid
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $28
    ldx $29
    mul.16 .AX, #40
    clc
    adc __zp_scratch2
    sta $26
    txa
    adc __zp_scratch2+1
    sta $27
    lda $20
    ldx $21
    clc
    adc $26
    sta $28
    txa
    adc $26+1
    sta $29
    lda $24
    ldy #0
    sta ($28),y
    .loc "main.c", 127
    lda #1
    sta $24
    lda $22
    clc
    adc #2
    sta $28
    lda $23
    adc #0
    sta $29
    lda #<_grid
    ldx #>_grid
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $28
    ldx $29
    mul.16 .AX, #40
    clc
    adc __zp_scratch2
    sta $26
    txa
    adc __zp_scratch2+1
    sta $27
    lda $20
    clc
    adc #1
    sta $2A
    lda $21
    adc #0
    sta $2B
    lda $2A
    ldx $2B
    clc
    adc $26
    sta $28
    txa
    adc $26+1
    sta $29
    lda $24
    ldy #0
    sta ($28),y
    .loc "main.c", 128
    lda #1
    sta $24
    lda $22
    clc
    adc #2
    sta $28
    lda $23
    adc #0
    sta $29
    lda #<_grid
    ldx #>_grid
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $28
    ldx $29
    mul.16 .AX, #40
    clc
    adc __zp_scratch2
    sta $26
    txa
    adc __zp_scratch2+1
    sta $27
    lda $20
    clc
    adc #2
    sta $2A
    lda $21
    adc #0
    sta $2B
    lda $2A
    ldx $2B
    clc
    adc $26
    sta $28
    txa
    adc $26+1
    sta $29
    lda $24
    ldy #0
    sta ($28),y
@__return:
    .func_flags stack_call, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    endproc

; function _main
    proc _main
    .var _fp = 0
    .loc "main.c", 131
; frame: 8 bytes (frame-allocated vRegs only)
    phw #0
    phw #0
    phw #0
    phw #0
    .local __vr0 = 0
    .local __vr25 = 2
    .local __vr27 = 4
    .local __vr28 = 6
    .local @_l_gen = 0

@entry:
    .loc "main.c", 134
    lda #0
    sta $D020
    .loc "main.c", 135
    jsr _clear_grid
    .loc "main.c", 136
    lda #2
    sta $D020
    .loc "main.c", 139
    lda #3
    sta $D020
    .loc "main.c", 140
    phw #18
    .var _fp = _fp + 2
    phw #10
    .var _fp = _fp + 2
    jsr _seed_r_pentomino
    .var _fp = _fp - 4
    .loc "main.c", 141
    lda #4
    sta $D020
    .loc "main.c", 142
    phw #2
    .var _fp = _fp + 2
    phw #2
    .var _fp = _fp + 2
    jsr _seed_glider
    .var _fp = _fp - 4
    .loc "main.c", 143
    lda #5
    sta $D020
    .loc "main.c", 144
    phw #30
    .var _fp = _fp + 2
    phw #3
    .var _fp = _fp + 2
    jsr _seed_glider
    .var _fp = _fp - 4
    .loc "main.c", 145
    lda #6
    sta $D020
    .loc "main.c", 149
    lda _color
    ldx _color+1
    stax.fp __vr25
    lda #5
    sta $20
    lda #232
    ldx #3
    stax.fp __vr27
    lda $20
    ldx #0
    ldx #0
    stax.fp __vr28
    ldax.fp __vr27
    sta $28
    stx $29
    ldax.fp __vr28
    sta $2C
    stx $2D
    ldax.fp __vr25
    sta $30
    stx $31
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    lda $2C
    ldx $2D
    push .ax
    .var _fp = _fp + 2
    lda $30
    ldx $31
    push .ax
    .var _fp = _fp + 2
    jsr _memset
    .var _fp = _fp - 6
    .loc "main.c", 150
    lda #7
    sta $D020
    .loc "main.c", 151
    lda #0
    sta $20
    sta $21
    stax.fp __vr0
@for_cond63:
    ldax.fp __vr0
    cmp.16 .AX, #500
    bcc @for_body64
    bra @for_end66
@for_body64:
    .loc "main.c", 152
    lda #8
    sta $D020
    .loc "main.c", 153
    jsr _draw
    .loc "main.c", 154
    jsr _delay
    .loc "main.c", 155
    jsr _step
@for_inc65:
    .loc "main.c", 151
    inc.16f __vr0
    bra @for_cond63
@for_end66:
    .loc "main.c", 157
    lda #9
    sta $D020
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
    .func_flags stack_call
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc


__zp_save_buf:

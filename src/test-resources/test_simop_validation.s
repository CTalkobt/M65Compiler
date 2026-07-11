* = $2000
    __sp_base = $0101
; Save ZP $08-$FF to BSS buffer
    ldx #0
@__zp_save_loop:
    lda $08,x
    sta __zp_save_buf,x
    inx
    cpx #248
    bne @__zp_save_loop
    jsr _main
    sta $02
    stx $03
; Restore ZP $08-$FF from BSS buffer
    ldx #0
@__zp_restore_loop:
    lda __zp_save_buf,x
    sta $08,x
    inx
    cpx #248
    bne @__zp_restore_loop
    lda $02
    ldx $03
__halt:
    jmp __halt
    .global __static_chain
    .global __zp_scratch
    .global __zp_scratch2
    .global __zp_scratch3
    .global __zp_scratch4
    __static_chain = $06
    __zp_scratch = $08
    __zp_scratch2 = $0A
    __zp_scratch3 = $0C
    __zp_scratch4 = $0E

_output:
    .word 16384
_byte_array:
    .byte 17
    .byte 34
    .byte 51
    .byte 68
    .byte 85
    .byte 102
    .byte 119
    .byte 136
_struct_array:
    .byte 170
    .byte 187
    .byte 204
    .byte 221
    .byte 238
    .byte 255
_test_count:
    .word 0
_errors:
    .word 0


; function _check
    proc _check, W#@_p_actual, W#@_p_expected, W#@_p_test_name
    .var _fp = 0
    .loc "src/test-resources/test_simop_validation.c", 17
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
    .local __vr0 = 0
    .local __vr1 = 2
    .local __vr2 = 4
    .var @_p_actual = 8
    .var @_p_expected = 10
    .var @_p_test_name = 12

    ldax.fp @_p_actual
    stax.fp __vr0
    ldax.fp @_p_expected
    stax.fp __vr1
    ldax.fp @_p_test_name
    stax.fp __vr2
@entry:
    .loc "src/test-resources/test_simop_validation.c", 18
    lda _test_count
    ldx _test_count+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda _output
    ldx _output+1
    sta $20
    stx $21
    lda #32
    ldx #0
    sta $24
    stx $25
    lda _test_count
    ldx _test_count+1
    sta $26
    stx $27
    lda $24
    clc
    adc $26
    sta $28
    lda $25
    adc $26+1
    sta $29
    lda $22
    ldx #0
    pha
    lda $20
    ldx $21
    sta $1F
    stx $1F+1
    lda $28
    ldx $29
    clc
    adc $1F
    pha
    txa
    adc $1F+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    pla
    sta (__zp_scratch),y
    ldax.fp __vr1
    sta $20
    .loc "src/test-resources/test_simop_validation.c", 19
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #48
    ldx #0
    sta $24
    stx $25
    lda _test_count
    ldx _test_count+1
    sta $26
    stx $27
    lda $24
    clc
    adc $26
    sta $28
    lda $25
    adc $26+1
    sta $29
    lda $20
    ldx #0
    pha
    lda $22
    ldx $23
    sta $1F
    stx $1F+1
    lda $28
    ldx $29
    clc
    adc $1F
    pha
    txa
    adc $1F+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    pla
    sta (__zp_scratch),y
    ldax.fp __vr0
    sta $20
    .loc "src/test-resources/test_simop_validation.c", 20
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #64
    ldx #0
    sta $24
    stx $25
    lda _test_count
    ldx _test_count+1
    sta $26
    stx $27
    lda $24
    clc
    adc $26
    sta $28
    lda $25
    adc $26+1
    sta $29
    lda $20
    ldx #0
    pha
    lda $22
    ldx $23
    sta $1F
    stx $1F+1
    lda $28
    ldx $29
    clc
    adc $1F
    pha
    txa
    adc $1F+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    pla
    sta (__zp_scratch),y
    .loc "src/test-resources/test_simop_validation.c", 22
    ldax.fp __vr1
    sta __zp_scratch2
    stx __zp_scratch2+1
    ldax.fp __vr0
    cmp.16 .AX, __zp_scratch2
    bne @if_then0
    bra @if_else1
@if_then0:
    .loc "src/test-resources/test_simop_validation.c", 23
    lda _errors
    ldx _errors+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    clc
    adc #1
    sta $20
    lda $23
    adc #0
    sta $21
    lda $20
    ldx $21
    sta _errors
    stx _errors+1
    .loc "src/test-resources/test_simop_validation.c", 24
    lda #255
    sta $20
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #80
    ldx #0
    sta $24
    stx $25
    lda _test_count
    ldx _test_count+1
    sta $26
    stx $27
    lda $24
    clc
    adc $26
    sta $28
    lda $25
    adc $26+1
    sta $29
    lda $20
    ldx #0
    pha
    lda $22
    ldx $23
    sta $1F
    stx $1F+1
    lda $28
    ldx $29
    clc
    adc $1F
    pha
    txa
    adc $1F+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    pla
    sta (__zp_scratch),y
    bra @if_end2
@if_else1:
    .loc "src/test-resources/test_simop_validation.c", 26
    lda #0
    sta $20
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #80
    ldx #0
    sta $24
    stx $25
    lda _test_count
    ldx _test_count+1
    sta $26
    stx $27
    lda $24
    clc
    adc $26
    sta $28
    lda $25
    adc $26+1
    sta $29
    lda $20
    ldx #0
    pha
    lda $22
    ldx $23
    sta $1F
    stx $1F+1
    lda $28
    ldx $29
    clc
    adc $1F
    pha
    txa
    adc $1F+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    pla
    sta (__zp_scratch),y
@if_end2:
    .loc "src/test-resources/test_simop_validation.c", 29
    lda _test_count
    ldx _test_count+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    clc
    adc #1
    sta $20
    lda $23
    adc #0
    sta $21
    lda $20
    ldx $21
    sta _test_count
    stx _test_count+1
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

; function _main
    proc _main
    .var _fp = 0
    .loc "src/test-resources/test_simop_validation.c", 32
; frame: 62 bytes (frame-allocated vRegs only)
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
    .local __vr0 = 0
    .local __vr14 = 2
    .local __vr15 = 4
    .local __vr16 = 6
    .local __vr20 = 8
    .local __vr21 = 10
    .local __vr22 = 12
    .local __vr26 = 14
    .local __vr27 = 16
    .local __vr28 = 18
    .local __vr32 = 20
    .local __vr33 = 22
    .local __vr34 = 24
    .local __vr38 = 26
    .local __vr39 = 28
    .local __vr40 = 30
    .local __vr44 = 32
    .local __vr45 = 34
    .local __vr46 = 36
    .local __vr50 = 38
    .local __vr51 = 40
    .local __vr52 = 42
    .local __vr56 = 44
    .local __vr57 = 46
    .local __vr58 = 48
    .local __vr62 = 50
    .local __vr63 = 52
    .local __vr64 = 54
    .local __vr68 = 56
    .local __vr69 = 58
    .local __vr70 = 60

@entry:
    .loc "src/test-resources/test_simop_validation.c", 35
    lda #0
    taz
    staz.fp __vr0
@for_cond3:
    ldax.fp __vr0
    cmp.16 .AX, #96
    bcc @for_body4
    bra @for_end6
@for_body4:
    .loc "src/test-resources/test_simop_validation.c", 36
    lda #0
    sta $24
    lda _output
    ldx _output+1
    sta $26
    stx $27
    lda $24
    ldx #0
    pha
    lda $26
    ldx $27
    sta $1F
    stx $1F+1
    ldax.fp __vr0
    clc
    adc $1F
    pha
    txa
    adc $1F+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    pla
    sta (__zp_scratch),y
@for_inc5:
    .loc "src/test-resources/test_simop_validation.c", 35
    ldax.fp __vr0
    sta $2A
    stx $2B
    lda $2A
    clc
    adc #1
    sta $2C
    lda $2B
    adc #0
    sta $2D
    lda $2C
    ldx $2D
    stax.fp __vr0
    bra @for_cond3
@for_end6:
    .loc "src/test-resources/test_simop_validation.c", 39
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta _test_count
    stx _test_count+1
    .loc "src/test-resources/test_simop_validation.c", 40
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta _errors
    stx _errors+1
    .loc "src/test-resources/test_simop_validation.c", 43
    lda #0
    sta $20
    sta $21
    ldax #_byte_array
    sta $1F
    stx $1F+1
    lda $20
    ldx $21
    clc
    adc $1F
    pha
    txa
    adc $1F+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $24
    lda #17
    ldx #0
    stax.fp __vr14
    ldax #__str_7
    stax.fp __vr15
    lda $24
    ldx #0
    ldx #0
    stax.fp __vr16
    ldax.fp __vr15
    sta $28
    stx $29
    ldax.fp __vr14
    sta $2A
    stx $2B
    ldax.fp __vr16
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
    jsr _check
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
    .loc "src/test-resources/test_simop_validation.c", 44
    lda #1
    ldx #0
    sta $20
    stx $21
    ldax #_byte_array
    sta $1F
    stx $1F+1
    lda $20
    ldx $21
    clc
    adc $1F
    pha
    txa
    adc $1F+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $24
    lda #34
    ldx #0
    stax.fp __vr20
    ldax #__str_8
    stax.fp __vr21
    lda $24
    ldx #0
    ldx #0
    stax.fp __vr22
    ldax.fp __vr21
    sta $28
    stx $29
    ldax.fp __vr20
    sta $2A
    stx $2B
    ldax.fp __vr22
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
    jsr _check
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
    .loc "src/test-resources/test_simop_validation.c", 45
    lda #2
    ldx #0
    sta $20
    stx $21
    ldax #_byte_array
    sta $1F
    stx $1F+1
    lda $20
    ldx $21
    clc
    adc $1F
    pha
    txa
    adc $1F+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $24
    lda #51
    ldx #0
    stax.fp __vr26
    ldax #__str_9
    stax.fp __vr27
    lda $24
    ldx #0
    ldx #0
    stax.fp __vr28
    ldax.fp __vr27
    sta $28
    stx $29
    ldax.fp __vr26
    sta $2A
    stx $2B
    ldax.fp __vr28
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
    jsr _check
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
    .loc "src/test-resources/test_simop_validation.c", 46
    lda #3
    ldx #0
    sta $20
    stx $21
    ldax #_byte_array
    sta $1F
    stx $1F+1
    lda $20
    ldx $21
    clc
    adc $1F
    pha
    txa
    adc $1F+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $24
    lda #68
    ldx #0
    stax.fp __vr32
    ldax #__str_10
    stax.fp __vr33
    lda $24
    ldx #0
    ldx #0
    stax.fp __vr34
    ldax.fp __vr33
    sta $28
    stx $29
    ldax.fp __vr32
    sta $2A
    stx $2B
    ldax.fp __vr34
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
    jsr _check
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
    .loc "src/test-resources/test_simop_validation.c", 47
    lda #4
    ldx #0
    sta $20
    stx $21
    ldax #_byte_array
    sta $1F
    stx $1F+1
    lda $20
    ldx $21
    clc
    adc $1F
    pha
    txa
    adc $1F+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $24
    lda #85
    ldx #0
    stax.fp __vr38
    ldax #__str_11
    stax.fp __vr39
    lda $24
    ldx #0
    ldx #0
    stax.fp __vr40
    ldax.fp __vr39
    sta $28
    stx $29
    ldax.fp __vr38
    sta $2A
    stx $2B
    ldax.fp __vr40
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
    jsr _check
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
    .loc "src/test-resources/test_simop_validation.c", 48
    lda #5
    ldx #0
    sta $20
    stx $21
    ldax #_byte_array
    sta $1F
    stx $1F+1
    lda $20
    ldx $21
    clc
    adc $1F
    pha
    txa
    adc $1F+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $24
    lda #102
    ldx #0
    stax.fp __vr44
    ldax #__str_12
    stax.fp __vr45
    lda $24
    ldx #0
    ldx #0
    stax.fp __vr46
    ldax.fp __vr45
    sta $28
    stx $29
    ldax.fp __vr44
    sta $2A
    stx $2B
    ldax.fp __vr46
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
    jsr _check
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
    .loc "src/test-resources/test_simop_validation.c", 51
    lda #0
    sta $20
    sta $21
    ldax #_struct_array
    sta $1F
    stx $1F+1
    lda $20
    ldx $21
    clc
    adc $1F
    pha
    txa
    adc $1F+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $24
    lda #170
    ldx #0
    stax.fp __vr50
    ldax #__str_13
    stax.fp __vr51
    lda $24
    ldx #0
    ldx #0
    stax.fp __vr52
    ldax.fp __vr51
    sta $28
    stx $29
    ldax.fp __vr50
    sta $2A
    stx $2B
    ldax.fp __vr52
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
    jsr _check
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
    .loc "src/test-resources/test_simop_validation.c", 52
    lda #1
    ldx #0
    sta $20
    stx $21
    ldax #_struct_array
    sta $1F
    stx $1F+1
    lda $20
    ldx $21
    clc
    adc $1F
    pha
    txa
    adc $1F+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $24
    lda #187
    ldx #0
    stax.fp __vr56
    ldax #__str_14
    stax.fp __vr57
    lda $24
    ldx #0
    ldx #0
    stax.fp __vr58
    ldax.fp __vr57
    sta $28
    stx $29
    ldax.fp __vr56
    sta $2A
    stx $2B
    ldax.fp __vr58
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
    jsr _check
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
    .loc "src/test-resources/test_simop_validation.c", 53
    lda #2
    ldx #0
    sta $20
    stx $21
    ldax #_struct_array
    sta $1F
    stx $1F+1
    lda $20
    ldx $21
    clc
    adc $1F
    pha
    txa
    adc $1F+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $24
    lda #204
    ldx #0
    stax.fp __vr62
    ldax #__str_15
    stax.fp __vr63
    lda $24
    ldx #0
    ldx #0
    stax.fp __vr64
    ldax.fp __vr63
    sta $28
    stx $29
    ldax.fp __vr62
    sta $2A
    stx $2B
    ldax.fp __vr64
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
    jsr _check
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
    .loc "src/test-resources/test_simop_validation.c", 54
    lda #3
    ldx #0
    sta $20
    stx $21
    ldax #_struct_array
    sta $1F
    stx $1F+1
    lda $20
    ldx $21
    clc
    adc $1F
    pha
    txa
    adc $1F+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $24
    lda #221
    ldx #0
    stax.fp __vr68
    ldax #__str_16
    stax.fp __vr69
    lda $24
    ldx #0
    ldx #0
    stax.fp __vr70
    ldax.fp __vr69
    sta $28
    stx $29
    ldax.fp __vr68
    sta $2A
    stx $2B
    ldax.fp __vr70
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
    jsr _check
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
    .loc "src/test-resources/test_simop_validation.c", 57
    lda #170
    sta $20
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #31
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx #0
    pha
    lda $22
    ldx $23
    sta $1F
    stx $1F+1
    lda $24
    ldx $25
    clc
    adc $1F
    pha
    txa
    adc $1F+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    pla
    sta (__zp_scratch),y
    .loc "src/test-resources/test_simop_validation.c", 60
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    ldx $23
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
    .func_flags stack_call
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc


__str_7:
    .text "array[0]"
    .byte 0
__str_8:
    .text "array[1]"
    .byte 0
__str_9:
    .text "array[2]"
    .byte 0
__str_10:
    .text "array[3]"
    .byte 0
__str_11:
    .text "array[4]"
    .byte 0
__str_12:
    .text "array[5]"
    .byte 0
__str_13:
    .text "struct[0]"
    .byte 0
__str_14:
    .text "struct[1]"
    .byte 0
__str_15:
    .text "struct[2]"
    .byte 0
__str_16:
    .text "struct[3]"
    .byte 0

__zp_save_buf:
    .res 248

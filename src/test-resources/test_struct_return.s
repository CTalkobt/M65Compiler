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

; function _make_point

    ; Static buffer for struct return from _make_point
    _make_point__struct_buf:
    .byte 0, 0, 0, 0

    proc _make_point, W#@_p_x, W#@_p_y
    .var _fp = 0
    .loc "src/test-resources/test_struct_return.c", 12
; frame: 8 bytes (frame-allocated vRegs only)
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
    .local __vr1 = 6
    .local __vr2 = 0
    .var @_p_x = 10
    .var @_p_y = 12

    ldax.fp @_p_x
    stax.fp __vr0
    ldax.fp @_p_y
    stax.fp __vr1
@entry:
    .loc "src/test-resources/test_struct_return.c", 14
    leax.fp 0
    sta $20
    stx $21
    ldax.fp __vr0
    ldy #0
    sta ($20),y
    txa
    iny
    sta ($20),y
    .loc "src/test-resources/test_struct_return.c", 15
    leax.fp 0
    sta $20
    stx $21
    lda $20
    ldx $21
    add.16 .AX, #2
    sta $22
    stx $23
    ldax.fp __vr1
    ldy #0
    sta ($22),y
    txa
    iny
    sta ($22),y
    .loc "src/test-resources/test_struct_return.c", 16
    ldaxyz.fp __vr2
@__return:
    stz @__restore_epilogue_z_0+1
    tsx
    txa
    clc
    adc #8
    tax
    txs
@__restore_epilogue_z_0:
    ldz #0
    .func_flags stack_call, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc

; function _sum_point
    proc _sum_point, W#@_p_p
    .var _fp = 0
    .loc "src/test-resources/test_struct_return.c", 19
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
    .var @_p_p = 4

    ldax.fp @_p_p
    stax.fp __vr0
@entry:
    .loc "src/test-resources/test_struct_return.c", 20
    ldax.fp __vr0
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $20
    stx $21
    ldax.fp __vr0
    add.16 .AX, #2
    sta $22
    stx $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $24
    stx $25
    lda $20
    clc
    adc $24
    sta $22
    lda $21
    adc $24+1
    sta $23
    lda $22
    ldx $23
@__return:
    pla
    pla
    .func_flags stack_call, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    endproc

; function _main
    proc _main
    .var _fp = 0
    .loc "src/test-resources/test_struct_return.c", 23
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
    .local __vr0 = 0
    .local __vr19 = 4
    .local __vr38 = 8
    .local __vr43 = 12

@entry:
    .loc "src/test-resources/test_struct_return.c", 25
    lda #2
    ldx #0
    push .ax
    .var _fp = _fp + 2
    lda #1
    ldx #0
    push .ax
    .var _fp = _fp + 2
    jsr _make_point
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
    pla
    pla
    pla
    pla
    .var _fp = _fp - 4
    sta $20
    stx $21
    lda $20
    ldx $21
    staxyz.fp __vr0
    .loc "src/test-resources/test_struct_return.c", 26
    leax.fp 0
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
    lda _results
    ldx _results+1
    sta $22
    stx $23
    lda #0
    sta $24
    sta $25
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
    .loc "src/test-resources/test_struct_return.c", 27
    leax.fp 0
    sta $20
    stx $21
    lda $20
    ldx $21
    add.16 .AX, #2
    sta $22
    stx $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda _results
    ldx _results+1
    sta $20
    stx $21
    lda #1
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx #0
    pha
    lda $20
    ldx $21
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
    .loc "src/test-resources/test_struct_return.c", 30
    lda #4
    ldx #0
    push .ax
    .var _fp = _fp + 2
    lda #3
    ldx #0
    push .ax
    .var _fp = _fp + 2
    jsr _make_point
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
    pla
    pla
    pla
    pla
    .var _fp = _fp - 4
    sta $20
    stx $21
    lda $20
    ldx $21
    staxyz.fp __vr19
    .loc "src/test-resources/test_struct_return.c", 31
    leax.fp 4
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
    lda _results
    ldx _results+1
    sta $22
    stx $23
    lda #2
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
    .loc "src/test-resources/test_struct_return.c", 32
    leax.fp 4
    sta $20
    stx $21
    lda $20
    ldx $21
    add.16 .AX, #2
    sta $22
    stx $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda _results
    ldx _results+1
    sta $20
    stx $21
    lda #3
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx #0
    pha
    lda $20
    ldx $21
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
    .loc "src/test-resources/test_struct_return.c", 35
    lda #20
    ldx #0
    push .ax
    .var _fp = _fp + 2
    lda #10
    ldx #0
    push .ax
    .var _fp = _fp + 2
    jsr _make_point
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
    pla
    pla
    pla
    pla
    .var _fp = _fp - 4
    sta $20
    stx $21
    lda $20
    ldx $21
    staxyz.fp __vr38
    .loc "src/test-resources/test_struct_return.c", 36
    leax.fp 8
    stax.fp __vr43
    ldax.fp __vr43
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _sum_point
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
    pla
    pla
    .var _fp = _fp - 2
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda _results
    ldx _results+1
    sta $20
    stx $21
    lda #4
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx #0
    pha
    lda $20
    ldx $21
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
    .loc "src/test-resources/test_struct_return.c", 37
    leax.fp 8
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
    lda _results
    ldx _results+1
    sta $22
    stx $23
    lda #4
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
    .loc "src/test-resources/test_struct_return.c", 38
    leax.fp 8
    sta $20
    stx $21
    lda $20
    ldx $21
    add.16 .AX, #2
    sta $22
    stx $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda _results
    ldx _results+1
    sta $20
    stx $21
    lda #5
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx #0
    pha
    lda $20
    ldx $21
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
    .loc "src/test-resources/test_struct_return.c", 41
    lda #170
    sta $20
    lda _results
    ldx _results+1
    sta $22
    stx $23
    lda #6
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
@__return:
    tsx
    txa
    clc
    adc #14
    tax
    txs
    .func_flags stack_call
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc

    .global _results
    .global _make_point
    .global _sum_point
    .global _main

    .segment "data"
    .byte 0
_results:
    .word 16384

    .segment "code"


__zp_save_buf:
    .res 248

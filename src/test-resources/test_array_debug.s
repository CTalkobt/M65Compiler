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


; function _main
    proc _main
    .var _fp = 0
    .loc "src/test-resources/test_array_debug.c", 5
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
    .local __vr59 = 8
    .local __vr73 = 6

@entry:
    .loc "src/test-resources/test_array_debug.c", 7
    leax.fp 0
    sta $20
    stx $21
    lda #100
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    pha
    phx
    struct_elem.16 __zp_scratch, $20, #0
    plx
    pla
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    lda #200
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    pha
    phx
    struct_elem.16 __zp_scratch, $20, #2
    plx
    pla
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    lda #44
    ldx #1
    sta $22
    stx $23
    lda $22
    ldx $23
    pha
    phx
    struct_elem.16 __zp_scratch, $20, #4
    plx
    pla
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    .loc "src/test-resources/test_array_debug.c", 10
    leax.fp 0
    sta $20
    stx $21
    lda #0
    sta $22
    sta $23
    lda $20
    ldx $21
    sta $1F
    stx $1F+1
    lda $22
    ldx $23
    mul.16 .AX, #2
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
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $26
    stx $27
    lda $26
    ldx $27
    sta $20
    lda _output
    ldx _output+1
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
    .loc "src/test-resources/test_array_debug.c", 11
    leax.fp 0
    sta $20
    stx $21
    lda #0
    sta $22
    sta $23
    lda $20
    ldx $21
    sta $1F
    stx $1F+1
    lda $22
    ldx $23
    mul.16 .AX, #2
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
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $26
    stx $27
    lda $26
    ldx $27
    txa
    ldx #0
    sta $20
    stx $21
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #1
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx $21
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
    .loc "src/test-resources/test_array_debug.c", 12
    leax.fp 0
    sta $20
    stx $21
    lda #1
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    sta $1F
    stx $1F+1
    lda $22
    ldx $23
    mul.16 .AX, #2
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
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $26
    stx $27
    lda $26
    ldx $27
    sta $20
    lda _output
    ldx _output+1
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
    .loc "src/test-resources/test_array_debug.c", 13
    leax.fp 0
    sta $20
    stx $21
    lda #1
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    sta $1F
    stx $1F+1
    lda $22
    ldx $23
    mul.16 .AX, #2
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
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $26
    stx $27
    lda $26
    ldx $27
    txa
    ldx #0
    sta $20
    stx $21
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #3
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx $21
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
    .loc "src/test-resources/test_array_debug.c", 14
    leax.fp 0
    sta $20
    stx $21
    lda #2
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    sta $1F
    stx $1F+1
    lda $22
    ldx $23
    mul.16 .AX, #2
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
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $26
    stx $27
    lda $26
    ldx $27
    sta $20
    lda _output
    ldx _output+1
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
    .loc "src/test-resources/test_array_debug.c", 15
    leax.fp 0
    sta $20
    stx $21
    lda #2
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    sta $1F
    stx $1F+1
    lda $22
    ldx $23
    mul.16 .AX, #2
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
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $26
    stx $27
    lda $26
    ldx $27
    txa
    ldx #0
    sta $20
    stx $21
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #5
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx $21
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
    .loc "src/test-resources/test_array_debug.c", 18
    leax.fp 0
    sta $20
    stx $21
    lda #1
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    sta $1F
    stx $1F+1
    lda $22
    ldx $23
    mul.16 .AX, #2
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
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $26
    stx $27
    lda $26
    ldx $27
    stax.fp __vr59
    ldax.fp __vr59
    sta $20
    .loc "src/test-resources/test_array_debug.c", 19
    lda _output
    ldx _output+1
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
    .loc "src/test-resources/test_array_debug.c", 20
    ldax.fp __vr59
    txa
    ldx #0
    sta $20
    stx $21
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #7
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx $21
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
    .loc "src/test-resources/test_array_debug.c", 24
    lda #0
    taz
    staz.fp __vr73
@for_cond0_ph:
    .loc "src/test-resources/test_array_debug.c", 25
    leax.fp 0
    sta $20
    stx $21
    .loc "src/test-resources/test_array_debug.c", 26
    leax.fp 0
    sta $22
    stx $23
@for_cond0:
    .loc "src/test-resources/test_array_debug.c", 24
    ldax.fp __vr73
    cmp.16 .AX, #3
    bcc @for_body1
    bra @for_end3
@for_body1:
    .loc "src/test-resources/test_array_debug.c", 25
    lda $20
    ldx $21
    sta $1F
    stx $1F+1
    ldax.fp __vr73
    mul.16 .AX, #2
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
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $2A
    stx $2B
    lda $2A
    ldx $2B
    sta $2C
    lda _output
    ldx _output+1
    sta $2E
    stx $2F
    lda #8
    ldx #0
    sta $30
    stx $31
    lda #2
    ldx #0
    sta $32
    stx $33
    ldax.fp __vr73
    lsl.16 .AX
    sta $34
    stx $35
    lda $30
    clc
    adc $34
    sta $36
    lda $31
    adc $34+1
    sta $37
    lda $2C
    ldx #0
    pha
    lda $2E
    ldx $2F
    sta $1F
    stx $1F+1
    lda $36
    ldx $37
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
    .loc "src/test-resources/test_array_debug.c", 26
    lda $22
    ldx $23
    sta $1F
    stx $1F+1
    ldax.fp __vr73
    mul.16 .AX, #2
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
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $3C
    stx $3D
    lda $3C
    ldx $3D
    txa
    ldx #0
    sta $3E
    stx $3F
    lda _output
    ldx _output+1
    sta $40
    stx $41
    lda #8
    ldx #0
    sta $42
    stx $43
    lda #2
    ldx #0
    sta $44
    stx $45
    lda $42
    clc
    adc $34
    sta $46
    lda $43
    adc $34+1
    sta $47
    lda $46
    clc
    adc #1
    sta $4A
    lda $47
    adc #0
    sta $4B
    lda $3E
    ldx $3F
    pha
    lda $40
    ldx $41
    sta $1F
    stx $1F+1
    lda $4A
    ldx $4B
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
@for_inc2:
    .loc "src/test-resources/test_array_debug.c", 24
    ldax.fp __vr73
    sta $4E
    stx $4F
    lda $4E
    clc
    adc #1
    sta $50
    lda $4F
    adc #0
    sta $51
    lda $50
    ldx $51
    stax.fp __vr73
    bra @for_cond0
@for_end3:
    .loc "src/test-resources/test_array_debug.c", 30
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
    .loc "src/test-resources/test_array_debug.c", 32
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
    .func_flags stack_call, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    endproc


__zp_save_buf:
    .res 248

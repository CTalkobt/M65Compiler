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
    .loc "test_array_trace.c", 5
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
    .local __vr8 = 10
    .local __vr15 = 6
    .local __vr71 = 14
    .local __vr89 = 8
    .local __vr93 = 12

@entry:
    .loc "test_array_trace.c", 7
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
    .loc "test_array_trace.c", 10
    leax.fp 0
    sta $20
    stx $21
    lda #0
    sta $22
    sta $23
    lda $20
    ldx $21
    ; base→$1F
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
    lda #255
    ldx #0
    sta $20
    stx $21
    lda $26
    ldx $27
    and.16 .AX, $20
    sta $22
    stx $23
    lda $22
    ldx $23
    sta.fp __vr8
    .loc "test_array_trace.c", 11
    leax.fp 0
    sta $20
    stx $21
    lda #0
    sta $22
    sta $23
    lda $20
    ldx $21
    ; base→$1F
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
    lda #255
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    and.16 .AX, $22
    sta $24
    stx $25
    lda $24
    ldx $25
    sta.fp __vr15
    .loc "test_array_trace.c", 12
    lda _output
    ldx _output+1
    sta $20
    stx $21
    lda #0
    sta $22
    sta $23
    lda.fp __vr8
    pha
    lda $20
    ldx $21
    sta $1F
    stx $1F+1
    lda $22
    ldx $23
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
    .loc "test_array_trace.c", 13
    lda _output
    ldx _output+1
    sta $20
    stx $21
    lda #1
    ldx #0
    sta $22
    stx $23
    lda.fp __vr15
    pha
    lda $20
    ldx $21
    sta $1F
    stx $1F+1
    lda $22
    ldx $23
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
    .loc "test_array_trace.c", 15
    leax.fp 0
    sta $20
    stx $21
    lda #1
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    ; base→$1F
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
    lda #255
    ldx #0
    sta $20
    stx $21
    lda $26
    ldx $27
    and $20
    sta $22
    stx $23
    lda $22
    ldx $23
    sta.fp __vr8
    .loc "test_array_trace.c", 16
    leax.fp 0
    sta $20
    stx $21
    lda #1
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    ; base→$1F
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
    lda #255
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    and $22
    sta $24
    stx $25
    lda $24
    ldx $25
    sta.fp __vr15
    .loc "test_array_trace.c", 17
    lda _output
    ldx _output+1
    sta $20
    stx $21
    lda #2
    ldx #0
    sta $22
    stx $23
    lda.fp __vr8
    pha
    lda $20
    ldx $21
    sta $1F
    stx $1F+1
    lda $22
    ldx $23
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
    .loc "test_array_trace.c", 18
    lda _output
    ldx _output+1
    sta $20
    stx $21
    lda #3
    ldx #0
    sta $22
    stx $23
    lda.fp __vr15
    pha
    lda $20
    ldx $21
    sta $1F
    stx $1F+1
    lda $22
    ldx $23
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
    .loc "test_array_trace.c", 20
    leax.fp 0
    sta $20
    stx $21
    lda #2
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    ; base→$1F
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
    lda #255
    ldx #0
    sta $20
    stx $21
    lda $26
    ldx $27
    and $20
    sta $22
    stx $23
    lda $22
    ldx $23
    sta.fp __vr8
    .loc "test_array_trace.c", 21
    leax.fp 0
    sta $20
    stx $21
    lda #2
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    ; base→$1F
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
    lda #255
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    and $22
    sta $24
    stx $25
    lda $24
    ldx $25
    sta.fp __vr15
    .loc "test_array_trace.c", 22
    lda _output
    ldx _output+1
    sta $20
    stx $21
    lda #4
    ldx #0
    sta $22
    stx $23
    lda.fp __vr8
    pha
    lda $20
    ldx $21
    sta $1F
    stx $1F+1
    lda $22
    ldx $23
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
    .loc "test_array_trace.c", 23
    lda _output
    ldx _output+1
    sta $20
    stx $21
    lda #5
    ldx #0
    sta $22
    stx $23
    lda.fp __vr15
    pha
    lda $20
    ldx $21
    sta $1F
    stx $1F+1
    lda $22
    ldx $23
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
    .loc "test_array_trace.c", 26
    leax.fp 0
    sta $20
    stx $21
    lda #1
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    ; base→$1F
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
    stax.fp __vr71
    .loc "test_array_trace.c", 27
    lda #255
    ldx #0
    sta $20
    stx $21
    ldax.fp __vr71
    and $20
    sta $22
    stx $23
    lda _output
    ldx _output+1
    sta $20
    stx $21
    lda #6
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
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
    .loc "test_array_trace.c", 28
    ldax.fp __vr71
    txa
    ldx #0
    sta $20
    stx $21
    lda #255
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    and $22
    sta $24
    stx $25
    lda _output
    ldx _output+1
    sta $20
    stx $21
    lda #7
    ldx #0
    sta $22
    stx $23
    lda $24
    ldx $25
    pha
    lda $20
    ldx $21
    sta $1F
    stx $1F+1
    lda $22
    ldx $23
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
    .loc "test_array_trace.c", 32
    lda #0
    taz
    staz.fp __vr89
@for_cond0_ph:
    .loc "test_array_trace.c", 33
    leax.fp 0
    sta $20
    stx $21
@for_cond0:
    .loc "test_array_trace.c", 32
    ldax.fp __vr89
    cmp.16 .AX, #3
    bcc @for_body1
    bra @for_end3
@for_body1:
    .loc "test_array_trace.c", 33
    lda $20
    ldx $21
    ; base→$1F
    sta $1F
    stx $1F+1
    ldax.fp __vr89
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
    sta $28
    stx $29
    lda $28
    ldx $29
    stax.fp __vr93
    .loc "test_array_trace.c", 34
    lda #255
    ldx #0
    sta $2A
    stx $2B
    ldax.fp __vr93
    and $2A
    sta $2C
    stx $2D
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
    ldax.fp __vr89
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
    ldx $2D
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
    .loc "test_array_trace.c", 35
    ldax.fp __vr93
    txa
    ldx #0
    sta $3A
    stx $3B
    lda #255
    ldx #0
    sta $3C
    stx $3D
    lda $3A
    ldx $3B
    and $3C
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
    .loc "test_array_trace.c", 32
    ldax.fp __vr89
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
    stax.fp __vr89
    bra @for_cond0
@for_end3:
    .loc "test_array_trace.c", 39
    lda #170
    sta $20
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #14
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
    .loc "test_array_trace.c", 41
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
    .func_flags stack_call, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    endproc


__zp_save_buf:
    .res 248

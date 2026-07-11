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

; function _main
    proc _main
    .var _fp = 0
    .loc "/tmp/test_memory_dump.c", 5
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

@entry:
    .loc "/tmp/test_memory_dump.c", 11
    lda #160
    ldx #33
    sta $20
    stx $21
    stax.fp __vr0
    .loc "/tmp/test_memory_dump.c", 13
    lda #0
    sta $20
    sta $21
    ldax.fp __vr0
    ; base→$1F
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
    lda _out
    ldx _out+1
    sta $20
    stx $21
    lda #0
    sta $22
    sta $23
    lda $24
    ldx #0
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
    .loc "/tmp/test_memory_dump.c", 14
    lda #1
    ldx #0
    sta $20
    stx $21
    ldax.fp __vr0
    ; base→$1F
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
    lda _out
    ldx _out+1
    sta $20
    stx $21
    lda #1
    ldx #0
    sta $22
    stx $23
    lda $24
    ldx #0
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
    .loc "/tmp/test_memory_dump.c", 15
    lda #2
    ldx #0
    sta $20
    stx $21
    ldax.fp __vr0
    ; base→$1F
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
    lda _out
    ldx _out+1
    sta $20
    stx $21
    lda #2
    ldx #0
    sta $22
    stx $23
    lda $24
    ldx #0
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
    .loc "/tmp/test_memory_dump.c", 16
    lda #3
    ldx #0
    sta $20
    stx $21
    ldax.fp __vr0
    ; base→$1F
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
    lda _out
    ldx _out+1
    sta $20
    stx $21
    lda #3
    ldx #0
    sta $22
    stx $23
    lda $24
    ldx #0
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
    .loc "/tmp/test_memory_dump.c", 17
    lda #4
    ldx #0
    sta $20
    stx $21
    ldax.fp __vr0
    ; base→$1F
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
    lda _out
    ldx _out+1
    sta $20
    stx $21
    lda #4
    ldx #0
    sta $22
    stx $23
    lda $24
    ldx #0
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
    .loc "/tmp/test_memory_dump.c", 18
    lda #5
    ldx #0
    sta $20
    stx $21
    ldax.fp __vr0
    ; base→$1F
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
    lda _out
    ldx _out+1
    sta $20
    stx $21
    lda #5
    ldx #0
    sta $22
    stx $23
    lda $24
    ldx #0
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
    .loc "/tmp/test_memory_dump.c", 19
    lda #6
    ldx #0
    sta $20
    stx $21
    ldax.fp __vr0
    ; base→$1F
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
    lda _out
    ldx _out+1
    sta $20
    stx $21
    lda #6
    ldx #0
    sta $22
    stx $23
    lda $24
    ldx #0
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
@__return:
    pla
    pla
    .func_flags stack_call, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers N, Z
    endproc

_out:
    .word 16384
_data:
    .byte 16
    .byte 32
    .byte 48
    .byte 64



__zp_save_buf:
    .res 248

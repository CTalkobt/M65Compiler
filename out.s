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

    .extern _bsearch

_test:
    .word 16384


; function _cmp
    proc _cmp, W#@_p_a, W#@_p_b
    .var _fp = 0
    .loc "/tmp/stdlib.h", 5
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
    .var @_p_a = 6
    .var @_p_b = 8

    ldax.fp @_p_a
    stax.fp __vr0
    ldax.fp @_p_b
    stax.fp __vr1
@entry:
    .loc "/tmp/test_bsearch_debug.c", 6
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
    ldax.fp __vr1
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $22
    stx $23
    lda $20
    sec
    sbc $22
    sta $24
    lda $21
    sbc $22+1
    sta $25
    lda $24
    ldx $25
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
    .loc "/tmp/stddef.h", 4
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
    .local __vr4 = 0
    .local __vr16 = 10
    .local __vr19 = 12
    .local __vr20 = 14
    .local __vr21 = 16
    .local __vr22 = 18

@entry:
    .loc "/tmp/test_bsearch_debug.c", 10
    lda #17
    sta $20
    lda _test
    ldx _test+1
    sta $22
    stx $23
    lda #0
    sta $24
    sta $25
    lda $20
    ldx #0
    pha
    .noopt_start
    addr_elem.16 __zp_scratch, $22, $24, #1
    ldy #0
    .noopt_end
    pla
    sta (__zp_scratch),y
    .loc "/tmp/test_bsearch_debug.c", 11
    leax.fp 0
    sta $20
    stx $21
    lda #10
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
    lda #20
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
    lda #30
    ldx #0
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
    lda #40
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    pha
    phx
    struct_elem.16 __zp_scratch, $20, #6
    plx
    pla
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    lda #50
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    pha
    phx
    struct_elem.16 __zp_scratch, $20, #8
    plx
    pla
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    .loc "/tmp/test_bsearch_debug.c", 12
    leax.fp 0
    sta $20
    stx $21
    lda #2
    ldx #0
    sta $22
    stx $23
    .noopt_start
    addr_elem.16 .AX, $20, $22, #2
    .noopt_end
    stax.fp __vr19
    leax.fp 0
    stax.fp __vr20
    lda #5
    ldx #0
    stax.fp __vr21
    lda #2
    ldx #0
    stax.fp __vr22
    ldax #_cmp
    sta $28
    stx $29
    ldax.fp __vr22
    sta $2A
    stx $2B
    ldax.fp __vr21
    sta $2C
    stx $2D
    ldax.fp __vr20
    sta $2E
    stx $2F
    ldax.fp __vr19
    sta $30
    stx $31
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
    lda $30
    ldx $31
    push .ax
    .var _fp = _fp + 2
    jsr _bsearch
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
    plz
    plz
    .var _fp = _fp - 10
    sta $20
    stx $21
    lda $20
    ldx $21
    stax.fp __vr16
    .loc "/tmp/test_bsearch_debug.c", 13
    lda #170
    sta $20
    lda _test
    ldx _test+1
    sta $22
    stx $23
    lda #1
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx #0
    pha
    .noopt_start
    addr_elem.16 __zp_scratch, $22, $24, #1
    ldy #0
    .noopt_end
    pla
    sta (__zp_scratch),y
    .loc "/tmp/test_bsearch_debug.c", 14
    ldax.fp __vr16
    bne @if_then0
    cmp #$00
    bne @if_then0
    bra @if_else1
@if_then0:
    .loc "/tmp/test_bsearch_debug.c", 15
    lda #85
    sta $20
    lda _test
    ldx _test+1
    sta $22
    stx $23
    lda #2
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx #0
    pha
    .noopt_start
    addr_elem.16 __zp_scratch, $22, $24, #1
    ldy #0
    .noopt_end
    pla
    sta (__zp_scratch),y
    bra @if_end2
@if_else1:
    .loc "/tmp/test_bsearch_debug.c", 17
    lda #153
    sta $20
    lda _test
    ldx _test+1
    sta $22
    stx $23
    lda #2
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx #0
    pha
    .noopt_start
    addr_elem.16 __zp_scratch, $22, $24, #1
    ldy #0
    .noopt_end
    pla
    sta (__zp_scratch),y
@if_end2:
    .loc "/tmp/test_bsearch_debug.c", 19
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
    .func_flags stack_call
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc


__zp_save_buf:
    .res 248
    .global _main

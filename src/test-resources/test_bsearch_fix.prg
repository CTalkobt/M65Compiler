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

_cmp_count:
    .word 0
_found_result:
    .word 65535


; function _compare_ints
    proc _compare_ints, W#@_p_a, W#@_p_b
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler.dev_v1.0.5/bin/../lib/include/stddef.h", 1
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
    .local __vr0 = 0
    .local __vr1 = 2
    .local __vr5 = 4
    .local __vr7 = 6
    .var @_p_a = 10
    .var @_p_b = 12

    ldax.fp @_p_a
    stax.fp __vr0
    ldax.fp @_p_b
    stax.fp __vr1
@entry:
    .loc "test_bsearch_fix.c", 8
    lda _cmp_count
    ldx _cmp_count+1
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
    sta _cmp_count
    stx _cmp_count+1
    .loc "test_bsearch_fix.c", 9
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
    lda $20
    ldx $21
    stax.fp __vr5
    .loc "test_bsearch_fix.c", 10
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
    sta $20
    stx $21
    lda $20
    ldx $21
    stax.fp __vr7
    .loc "test_bsearch_fix.c", 11
    ldax.fp __vr7
    sta __zp_scratch2
    stx __zp_scratch2+1
    ldax.fp __vr5
    cmp.16 .AX, __zp_scratch2
    bcc @if_then0
    bra @if_end2
@if_then0:
    lda #255
    ldx #255
    bra @__return
@if_end2:
    .loc "test_bsearch_fix.c", 12
    ldax.fp __vr7
    sta __zp_scratch2
    stx __zp_scratch2+1
    ldax.fp __vr5
    cmp.16 .AX, __zp_scratch2
    beq @if_end5
    bcs @if_then3
    bra @if_end5
@if_then3:
    lda #1
    ldx #0
    bra @__return
@if_end5:
    .loc "test_bsearch_fix.c", 13
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
    .func_flags stack_call, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    endproc

; function _main
    proc _main
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler.dev_v1.0.5/bin/../lib/include/stddef.h", 10
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
    .local __vr0 = 0
    .local __vr12 = 10
    .local __vr14 = 12
    .local __vr15 = 14
    .local __vr16 = 16
    .local __vr17 = 18
    .local __vr18 = 20

@entry:
    .loc "test_bsearch_fix.c", 17
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
    .loc "test_bsearch_fix.c", 18
    lda #30
    ldz #0
    staz.fp __vr12
    .loc "test_bsearch_fix.c", 21
    leax.fp 10
    stax.fp __vr15
    leax.fp 0
    stax.fp __vr16
    lda #5
    ldx #0
    stax.fp __vr17
    lda #2
    ldx #0
    stax.fp __vr18
    ldax #_compare_ints
    sta $28
    stx $29
    ldax.fp __vr18
    sta $2A
    stx $2B
    ldax.fp __vr17
    sta $2C
    stx $2D
    ldax.fp __vr16
    sta $2E
    stx $2F
    ldax.fp __vr15
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
    stax.fp __vr14
    .loc "test_bsearch_fix.c", 24
    ldax.fp __vr14
    stx __zp_scratch
    ora __zp_scratch
    bne @and_rhs10
    bra @if_end8
@and_rhs10:
    ldax.fp __vr14
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
    lda $20
    ldx $21
    cmp.16 .AX, #30
    beq @and_rhs9
    bra @if_end8
@and_rhs9:
    lda _cmp_count
    ldx _cmp_count+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.s16 .AX, #0
    beq @if_end8
    bcs @if_then6
    bra @if_end8
@if_then6:
    .loc "test_bsearch_fix.c", 25
    lda #0
    ldx #0
    bra @__return
@if_end8:
    .loc "test_bsearch_fix.c", 27
    lda #1
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
    .func_flags stack_call
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc


__zp_save_buf:
    .res 248

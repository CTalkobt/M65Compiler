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

    .extern _qsort

    .global _test_result
    .global _compare_calls
    .global _compare_ints
    .global _main

    .segment "data"
    .byte 0
_test_result:
    .word 16384
_compare_calls:
    .word 0

    .segment "code"

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
    .loc "test_qsort_fix.c", 8
    lda _compare_calls
    ldx _compare_calls+1
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
    sta _compare_calls
    stx _compare_calls+1
    .loc "test_qsort_fix.c", 9
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
    .loc "test_qsort_fix.c", 10
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
    .loc "test_qsort_fix.c", 11
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
    .loc "test_qsort_fix.c", 12
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
    .loc "test_qsort_fix.c", 13
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
; frame: 18 bytes (frame-allocated vRegs only)
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
    .local __vr12 = 12
    .local __vr13 = 14
    .local __vr14 = 16
    .local __vr15 = 10

@entry:
    .loc "test_qsort_fix.c", 17
    leax.fp 0
    sta $20
    stx $21
    lda #50
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
    lda #10
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
    lda #40
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
    lda #20
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
    lda #30
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
    .loc "test_qsort_fix.c", 20
    leax.fp 0
    stax.fp __vr12
    lda #5
    ldx #0
    stax.fp __vr13
    lda #2
    ldx #0
    stax.fp __vr14
    ldax #_compare_ints
    sta $28
    stx $29
    ldax.fp __vr14
    sta $2A
    stx $2B
    ldax.fp __vr13
    sta $2C
    stx $2D
    ldax.fp __vr12
    sta $2E
    stx $2F
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
    jsr _qsort
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
    .var _fp = _fp - 8
    .loc "test_qsort_fix.c", 23
    lda #1
    ldz #0
    staz.fp __vr15
    .loc "test_qsort_fix.c", 24
    leax.fp 0
    sta $20
    stx $21
    lda #0
    sta $22
    sta $23
    .noopt_start
    addr_elem.16 __zp_scratch, $20, $22, #2
    ldy #0
    .noopt_end
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
    cmp.16 .AX, #10
    bne @if_then6
    bra @if_end8
@if_then6:
    lda #0
    taz
    staz.fp __vr15
@if_end8:
    .loc "test_qsort_fix.c", 25
    leax.fp 0
    sta $20
    stx $21
    lda #1
    ldx #0
    sta $22
    stx $23
    .noopt_start
    addr_elem.16 __zp_scratch, $20, $22, #2
    ldy #0
    .noopt_end
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
    cmp.16 .AX, #20
    bne @if_then9
    bra @if_end11
@if_then9:
    lda #0
    taz
    staz.fp __vr15
@if_end11:
    .loc "test_qsort_fix.c", 26
    leax.fp 0
    sta $20
    stx $21
    lda #2
    ldx #0
    sta $22
    stx $23
    .noopt_start
    addr_elem.16 __zp_scratch, $20, $22, #2
    ldy #0
    .noopt_end
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
    cmp.16 .AX, #30
    bne @if_then12
    bra @if_end14
@if_then12:
    lda #0
    taz
    staz.fp __vr15
@if_end14:
    .loc "test_qsort_fix.c", 27
    leax.fp 0
    sta $20
    stx $21
    lda #3
    ldx #0
    sta $22
    stx $23
    .noopt_start
    addr_elem.16 __zp_scratch, $20, $22, #2
    ldy #0
    .noopt_end
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
    cmp.16 .AX, #40
    bne @if_then15
    bra @if_end17
@if_then15:
    lda #0
    taz
    staz.fp __vr15
@if_end17:
    .loc "test_qsort_fix.c", 28
    leax.fp 0
    sta $20
    stx $21
    lda #4
    ldx #0
    sta $22
    stx $23
    .noopt_start
    addr_elem.16 __zp_scratch, $20, $22, #2
    ldy #0
    .noopt_end
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
    cmp.16 .AX, #50
    bne @if_then18
    bra @if_end20
@if_then18:
    lda #0
    taz
    staz.fp __vr15
@if_end20:
    .loc "test_qsort_fix.c", 31
    ldax.fp __vr15
    bne @tern_then21
    cmp #$00
    bne @tern_then21
    bra @tern_else22
@tern_then21:
    lda #170
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @tern_end23
@tern_else22:
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@tern_end23:
    lda $22
    ldx $23
    sta $20
    lda _test_result
    ldx _test_result+1
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
    .loc "test_qsort_fix.c", 32
    lda _compare_calls
    ldx _compare_calls+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.s16 .AX, #0
    beq @tern_else25
    bcs @tern_then24
    bra @tern_else25
@tern_then24:
    lda #187
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @tern_end26
@tern_else25:
    lda #2
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@tern_end26:
    lda $22
    ldx $23
    sta $20
    lda _test_result
    ldx _test_result+1
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
    .loc "test_qsort_fix.c", 33
    lda #255
    sta $20
    lda _test_result
    ldx _test_result+1
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
    .func_flags stack_call
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc


__zp_save_buf:
    .res 248

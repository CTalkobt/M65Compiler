    .o45
    .org $2000
    .weak __sp_base
    __sp_base = $0101
    .weak __static_chain
    .weak __zp_scratch
    .weak __zp_scratch2
    .weak __zp_scratch3
    .weak __zp_scratch4
    .weak cc45.zeroPageStart
    __static_chain = $06
    __zp_scratch = $08
    __zp_scratch2 = $0A
    __zp_scratch3 = $0C
    __zp_scratch4 = $0E
    cc45.zeroPageStart = $08

    .extern _qsort

    .global _test_result
    .global _compare_calls
    .global _compare_ints
    .global _main

    .segment "data"
    .byte 0
_test_result:
; .debug_var: @global _test_result offset=0 size=2 type=ptr scope=global
    .word 16384
_compare_calls:
; .debug_var: @global _compare_calls offset=0 size=2 type=int16 scope=global
    .word 0

    .segment "code"

; function _compare_ints
; SAC inline storage: 8 bytes
    .global _compare_ints__param_a
    _compare_ints__param_a: .word 0
    .global _compare_ints__param_b
    _compare_ints__param_b: .word 0
    _compare_ints__local_0: .word 0
    _compare_ints__local_1: .word 0
    _compare_ints__local_5: .word 0
    _compare_ints__local_7: .word 0
    proc _compare_ints, W#@_p_a, W#@_p_b
    .sac
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stddef.h", 1
    .local @_l_va = 4
    .local @_l_vb = 6
; .debug_var: __compare_ints @_l_va offset=4 size=2 type=int16 scope=local
; .debug_var: __compare_ints @_l_vb offset=6 size=2 type=int16 scope=local
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __compare_ints @_p_a offset=2 size=2 type=ptr scope=parameter
; .debug_var: __compare_ints @_p_b offset=4 size=2 type=ptr scope=parameter

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
    lda _compare_ints__param_a
    ldx _compare_ints__param_a+1
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
    sta _compare_ints__local_5
    stx _compare_ints__local_5+1
    .loc "test_qsort_fix.c", 10
    lda _compare_ints__param_b
    ldx _compare_ints__param_b+1
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
    sta _compare_ints__local_7
    stx _compare_ints__local_7+1
    .loc "test_qsort_fix.c", 11
    lda _compare_ints__local_7
    ldx _compare_ints__local_7+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _compare_ints__local_5
    ldx _compare_ints__local_5+1
    cmp.16 .AX, __zp_scratch2
    bcc @if_then0
    bra @if_end2
@if_then0:
    lda #255
    ldx #255
    bra @__return
@if_end2:
    .loc "test_qsort_fix.c", 12
    lda _compare_ints__local_7
    ldx _compare_ints__local_7+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _compare_ints__local_5
    ldx _compare_ints__local_5+1
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
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 8
    endproc

; function _main
; SAC inline storage: 12 bytes
    _main__local_0: .word 0
    _main__local_12: .word 0
    _main__local_13: .word 0
    _main__local_14: .word 0
    _main__local_15: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stddef.h", 10
    .local @_l_arr = 2
    .local @_l_sorted = 0
; .debug_var: __main @_l_arr offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_sorted offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_qsort_fix.c", 17
    leax.local 2
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
    ldy #0
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
    ldy #0
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
    ldy #0
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
    ldy #0
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
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    .loc "test_qsort_fix.c", 20
    leax.local 2
    sta _main__local_12
    stx _main__local_12+1
    lda #5
    ldx #0
    sta _main__local_13
    stx _main__local_13+1
    lda #2
    ldx #0
    sta _main__local_14
    stx _main__local_14+1
    ldax #_compare_ints
    sta $28
    stx $29
    lda _main__local_14
    ldx _main__local_14+1
    sta $2A
    stx $2B
    lda _main__local_13
    ldx _main__local_13+1
    sta $2C
    stx $2D
    lda _main__local_12
    ldx _main__local_12+1
    sta $2E
    stx $2F
    lda $28
    ldx $29
    push .ax
    lda $2A
    ldx $2B
    push .ax
    lda $2C
    ldx $2D
    push .ax
    lda $2E
    ldx $2F
    push .ax
    jsr _qsort
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    .loc "test_qsort_fix.c", 23
    lda #1
    sta _main__local_15
    lda #0
    sta _main__local_15+1
    .loc "test_qsort_fix.c", 24
    leax.local 2
    sta $20
    stx $21
    lda #0
    sta $22
    sta $23
    lda $22
    ldx $23
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
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
    cmp.16 .AX, #10
    bne @if_then6
    bra @if_end8
@if_then6:
    lda #0
    sta _main__local_15
    sta _main__local_15+1
@if_end8:
    .loc "test_qsort_fix.c", 25
    leax.local 2
    sta $20
    stx $21
    lda #1
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
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
    cmp.16 .AX, #20
    bne @if_then9
    bra @if_end11
@if_then9:
    lda #0
    sta _main__local_15
    sta _main__local_15+1
@if_end11:
    .loc "test_qsort_fix.c", 26
    leax.local 2
    sta $20
    stx $21
    lda #2
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
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
    cmp.16 .AX, #30
    bne @if_then12
    bra @if_end14
@if_then12:
    lda #0
    sta _main__local_15
    sta _main__local_15+1
@if_end14:
    .loc "test_qsort_fix.c", 27
    leax.local 2
    sta $20
    stx $21
    lda #3
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
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
    cmp.16 .AX, #40
    bne @if_then15
    bra @if_end17
@if_then15:
    lda #0
    sta _main__local_15
    sta _main__local_15+1
@if_end17:
    .loc "test_qsort_fix.c", 28
    leax.local 2
    sta $20
    stx $21
    lda #4
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
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
    cmp.16 .AX, #50
    bne @if_then18
    bra @if_end20
@if_then18:
    lda #0
    sta _main__local_15
    sta _main__local_15+1
@if_end20:
    .loc "test_qsort_fix.c", 31
    lda _main__local_15
    ldx _main__local_15+1
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
    lda $24
    ldx $25
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $22
    ldx $22+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
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
    lda $24
    ldx $25
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $22
    ldx $22+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
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
    lda $24
    ldx $25
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $22
    ldx $22+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 12
    endproc


__zp_save_buf:

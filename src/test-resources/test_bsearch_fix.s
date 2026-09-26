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

    .extern _bsearch

    .global _cmp_count
    .global _found_result
    .global _compare_ints
    .global _main

    .segment "data"
    .byte 0
_cmp_count:
; .debug_var: @global _cmp_count offset=0 size=2 type=int16 scope=global
    .word 0
_found_result:
; .debug_var: @global _found_result offset=0 size=2 type=int16 scope=global
    .word 65535

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
    .loc "test_bsearch_fix.c", 10
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
    .loc "test_bsearch_fix.c", 11
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
    .loc "test_bsearch_fix.c", 12
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
    .loc "test_bsearch_fix.c", 13
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
; SAC inline storage: 14 bytes
    _main__local_0: .word 0
    _main__local_12: .word 0
    _main__local_14: .word 0
    _main__local_15: .word 0
    _main__local_16: .word 0
    _main__local_17: .word 0
    _main__local_18: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stddef.h", 10
    .local @_l_arr = 4
    .local @_l_key = 0
    .local @_l_result = 2
; .debug_var: __main @_l_arr offset=4 size=2 type=int16 scope=local
; .debug_var: __main @_l_key offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_result offset=2 size=2 type=ptr scope=local

@entry:
    .loc "test_bsearch_fix.c", 17
    leax.local 4
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
    struct_elem.16 __zp_scratch, $20, #2
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
    struct_elem.16 __zp_scratch, $20, #4
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
    struct_elem.16 __zp_scratch, $20, #6
    plx
    pla
    ldy #0
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
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    .loc "test_bsearch_fix.c", 18
    lda #30
    sta _main__local_12
    lda #0
    sta _main__local_12+1
    .loc "test_bsearch_fix.c", 21
    leax.local 0
    sta _main__local_15
    stx _main__local_15+1
    leax.local 4
    sta _main__local_16
    stx _main__local_16+1
    lda #5
    ldx #0
    sta _main__local_17
    stx _main__local_17+1
    lda #2
    ldx #0
    sta _main__local_18
    stx _main__local_18+1
    ldax #_compare_ints
    sta $28
    stx $29
    lda _main__local_18
    ldx _main__local_18+1
    sta $2A
    stx $2B
    lda _main__local_17
    ldx _main__local_17+1
    sta $2C
    stx $2D
    lda _main__local_16
    ldx _main__local_16+1
    sta $2E
    stx $2F
    lda _main__local_15
    ldx _main__local_15+1
    sta $30
    stx $31
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
    lda $30
    ldx $31
    push .ax
    jsr _bsearch
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    plz
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_14
    stx _main__local_14+1
    .loc "test_bsearch_fix.c", 24
    lda _main__local_14
    ldx _main__local_14+1
    stx __zp_scratch
    ora __zp_scratch
    bne @and_rhs10
    bra @if_end8
@and_rhs10:
    lda _main__local_14
    ldx _main__local_14+1
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
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 14
    endproc


__zp_save_buf:

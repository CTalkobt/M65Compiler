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

    .global _g_x
    .global _g_c
    .global _g_y
    .global _main

    .segment "data"
    .byte 0
_g_x:
; .debug_var: @global _g_x offset=0 size=2 type=int16 scope=global
    .word 42
_g_c:
; .debug_var: @global _g_c offset=0 size=2 type=int8 scope=global
    .byte 10

    .segment "bss"
_g_y:
; .debug_var: @global _g_y offset=0 size=2 type=int16 scope=global
    .res 2

    .segment "code"

; function _main
; SAC zero-alloc leaf: no storage overhead
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_global_vars.c", 5

@entry:
    .loc "test_global_vars.c", 6
    lda _g_x
    ldx _g_x+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #42
    bne @if_then0
    bra @if_end2
@if_then0:
    lda #1
    ldx #0
    bra @__return
@if_end2:
    .loc "test_global_vars.c", 7
    lda _g_c
    ldx #0
    sta $20
    lda #10
    sta $22
    lda $20
    ldx #0
    sxt.8
    sta $24
    stx $25
    lda $22
    ldx #0
    ldx #0
    sta $20
    stx $21
    lda $24
    ldx $25
    cmp.16 .AX, $20
    bne @if_then3
    bra @if_end5
@if_then3:
    lda #2
    ldx #0
    bra @__return
@if_end5:
    .loc "test_global_vars.c", 9
    lda _g_x
    ldx _g_x+1
    sta $20
    stx $21
    lda _g_c
    ldx #0
    sta $22
    lda $22
    ldx #0
    sxt.8
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
    sta _g_y
    stx _g_y+1
    .loc "test_global_vars.c", 10
    lda _g_y
    ldx _g_y+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #52
    bne @if_then6
    bra @if_end8
@if_then6:
    lda #3
    ldx #0
    bra @__return
@if_end8:
    .loc "test_global_vars.c", 12
    lda #100
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _g_x
    stx _g_x+1
    .loc "test_global_vars.c", 13
    lda _g_x
    ldx _g_x+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #100
    bne @if_then9
    bra @if_end11
@if_then9:
    lda #4
    ldx #0
    bra @__return
@if_end11:
    .loc "test_global_vars.c", 15
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, zeroalloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 0
    endproc


__zp_save_buf:

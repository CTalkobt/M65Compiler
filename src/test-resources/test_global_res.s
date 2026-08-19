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

    .global _global_arr
    .global _global_val
    .global _main

    .segment "bss"
_global_arr:
; .debug_var: @global _global_arr offset=0 size=2 type=int16 scope=global
    .res 20
_global_val:
; .debug_var: @global _global_val offset=0 size=2 type=int16 scope=global
    .res 2

    .segment "code"

; function _main
; SAC zero-alloc leaf: no storage overhead
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_global_res.c", 5

@entry:
    .loc "test_global_res.c", 6
    lda #42
    ldx #0
    sta $20
    stx $21
    lda #0
    sta $22
    sta $23
    lda $20
    ldx $21
    pha
    phx
    lda $22
    ldx $23
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_global_arr
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    .loc "test_global_res.c", 7
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_global_arr
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
    sta $24
    stx $25
    lda $24
    ldx $25
    sta _global_val
    stx _global_val+1
    .loc "test_global_res.c", 8
    lda _global_val
    ldx _global_val+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #42
    beq @if_then0
    bra @if_end2
@if_then0:
    lda #0
    ldx #0
    bra @__return
@if_end2:
    .loc "test_global_res.c", 9
    lda #1
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, zeroalloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 0
    endproc


__zp_save_buf:

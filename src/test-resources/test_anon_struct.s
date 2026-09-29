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

    .global _global_s
    .global _main

    .segment "bss"
_global_s:
; .debug_var: @global _global_s offset=0 size=2 type=int16 scope=global
    .res 6

    .segment "code"

; function _main
; SAC zero-alloc leaf: no storage overhead
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_anon_struct.c", 11

@entry:
    .loc "test_anon_struct.c", 12
    lda #10
    ldx #0
    sta $20
    stx $21
    ldax #_global_s
    sta $22
    stx $23
    lda $20
    ldx $21
    sta _global_s
    stx _global_s+1
    .loc "test_anon_struct.c", 13
    lda #20
    ldx #0
    sta $20
    stx $21
    ldax #_global_s
    add.16 .AX, #2
    sta $22
    stx $23
    lda $20
    ldx $21
    ldy #0
    sta ($22),y
    txa
    iny
    sta ($22),y
    .loc "test_anon_struct.c", 14
    lda #30
    ldx #0
    sta $20
    stx $21
    ldax #_global_s
    add.16 .AX, #4
    sta $22
    stx $23
    lda $20
    ldx $21
    ldy #0
    sta ($22),y
    txa
    iny
    sta ($22),y
    .loc "test_anon_struct.c", 16
    ldax #_global_s
    sta $20
    stx $21
    lda _global_s
    ldx _global_s+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #10
    bne @if_then0
    bra @if_end2
@if_then0:
    lda #1
    ldx #0
    bra @__return
@if_end2:
    .loc "test_anon_struct.c", 17
    ldax #_global_s
    add.16 .AX, #2
    sta $20
    stx $21
    ldy #0
    lda ($20),y
    pha
    iny
    lda ($20),y
    tax
    pla
    sta $22
    stx $23
    lda $22
    ldx $23
    cmp.16 .AX, #20
    bne @if_then3
    bra @if_end5
@if_then3:
    lda #2
    ldx #0
    bra @__return
@if_end5:
    .loc "test_anon_struct.c", 18
    ldax #_global_s
    add.16 .AX, #4
    sta $20
    stx $21
    ldy #0
    lda ($20),y
    pha
    iny
    lda ($20),y
    tax
    pla
    sta $22
    stx $23
    lda $22
    ldx $23
    cmp.16 .AX, #30
    bne @if_then6
    bra @if_end8
@if_then6:
    lda #3
    ldx #0
    bra @__return
@if_end8:
    .loc "test_anon_struct.c", 20
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, zeroalloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 0
    endproc


__zp_save_buf:

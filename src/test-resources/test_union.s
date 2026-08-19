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

    .global _global_u
    .global _global_s
    .global _main

    .segment "bss"
_global_u:
; .debug_var: @global _global_u offset=0 size=2 type=int16 scope=global
    .res 2
_global_s:
; .debug_var: @global _global_s offset=0 size=4 type=int32 scope=global
    .res 4

    .segment "code"

; function _main
; SAC inline storage: 0 bytes
    _main__local_1: .word 0
    _main__local_2: .word 0
    _main__local_9: .word 0
    _main__local_12: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_union.c", 17

@entry:
    .loc "test_union.c", 18
    lda #52
    ldx #18
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _global_u
    stx _global_u+1
    .loc "test_union.c", 19
    lda _global_u
    ldx #0
    sta $20
    lda #52
    sta $22
    lda $20
    ldx #0
    ldx #0
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
    bne @if_then0
    bra @if_end2
@if_then0:
    lda #1
    ldx #0
    bra @__return
@if_end2:
    .loc "test_union.c", 21
    lda #100
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _global_s
    stx _global_s+1
    .loc "test_union.c", 22
    lda #120
    ldx #86
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
    .loc "test_union.c", 23
    ldy #0
    lda ($22),y
    ldx #0
    sta $20
    lda #120
    sta $22
    lda $20
    ldx #0
    ldx #0
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
    .loc "test_union.c", 25
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 0
    endproc


__zp_save_buf:

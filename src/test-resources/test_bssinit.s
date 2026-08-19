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

    .global _out
    .global _x
    .global _main

    .segment "data"
    .byte 0
_out:
; .debug_var: @global _out offset=0 size=2 type=ptr scope=global
    .word 16384

    .segment "bss"
_x:
; .debug_var: @global _x offset=0 size=2 type=int16 scope=global
    .res 2

    .segment "code"

; function _main
; SAC zero-alloc leaf: no storage overhead
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_bssinit.c", 7

@entry:
    .loc "test_bssinit.c", 8
    lda _x
    ldx _x+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda _out
    ldx _out+1
    sta $20
    stx $21
    lda $22
    ldy #0
    sta ($20),y
    .loc "test_bssinit.c", 9
    lda #42
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _x
    stx _x+1
    .loc "test_bssinit.c", 10
    lda #1
    ldx #64
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _out
    stx _out+1
    .loc "test_bssinit.c", 11
    lda _x
    ldx _x+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda _out
    ldx _out+1
    sta $20
    stx $21
    lda $22
    ldy #0
    sta ($20),y
    .loc "test_bssinit.c", 12
    brk
@__return:
    rts
    .func_flags stack_call, static_alloc, zeroalloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 0
    endproc


__zp_save_buf:

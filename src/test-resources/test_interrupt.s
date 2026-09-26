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

    .global _tick_count
    .global _timer_isr
    .global _main

    .segment "data"
    .byte 0
_tick_count:
; .debug_var: @global _tick_count offset=0 size=2 type=int16 scope=global
    .word 0

    .segment "code"

; function _timer_isr
    proc _timer_isr
    pha
    phx
    phy
    phz
    .var _fp = 0
    .loc "test_interrupt.c", 5

@entry:
    .loc "test_interrupt.c", 6
    lda _tick_count
    ldx _tick_count+1
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
    sta _tick_count
    stx _tick_count+1
@__return:
    plz
    ply
    plx
    pla
    rti
    rts
    .func_flags stack_call, isr, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 0
    endproc

; function _main
; SAC zero-alloc leaf: no storage overhead
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_interrupt.c", 9

@entry:
    .loc "test_interrupt.c", 10
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, zeroalloc, leaf
    .reg_clobbers A, X
    .flag_clobbers N, Z
    .frame_size 0
    endproc


__zp_save_buf:

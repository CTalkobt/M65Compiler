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

    .extern _free
    .extern _malloc

    .global _main

    .segment "code"

; function _main
; SAC inline storage: 2 bytes
    _main__local_0: .word 0
    _main__local_1: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdlib.h", 3
    .local @_l_p = 0
; .debug_var: __main @_l_p offset=0 size=2 type=ptr scope=local

@entry:
    .loc "test_pragma_heap.c", 7
    lda #10
    ldx #0
    push .ax
    jsr _malloc
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_0
    stx _main__local_0+1
    .loc "test_pragma_heap.c", 8
    lda _main__local_0
    ldx _main__local_0+1
    stx __zp_scratch
    ora __zp_scratch
    bne @if_then0
    bra @if_end2
@if_then0:
    .loc "test_pragma_heap.c", 9
    lda #42
    ldx #0
    sta $20
    stx $21
    lda _main__local_0
    ldx _main__local_0+1
    sta $22
    stx $23
    lda $20
    ldx $21
    ldy #0
    sta ($22),y
    txa
    iny
    sta ($22),y
    .loc "test_pragma_heap.c", 10
    lda _main__local_0
    ldx _main__local_0+1
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    jsr _free
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
@if_end2:
    .loc "test_pragma_heap.c", 12
    brk
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc


__zp_save_buf:

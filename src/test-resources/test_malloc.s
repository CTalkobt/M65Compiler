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

    .extern _calloc
    .extern _free
    .extern _malloc
    .extern _realloc

    .global _main

    .segment "code"

; function _main
; SAC inline storage: 6 bytes
    _main__local_0: .word 0
    _main__local_1: .word 0
    _main__local_5: .word 0
    _main__local_6: .word 0
    _main__local_10: .word 0
    _main__local_11: .word 0
    _main__local_12: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdlib.h", 4
    .local @_l_arr = 4
    .local @_l_p = 0
    .local @_l_p2 = 2
; .debug_var: __main @_l_arr offset=4 size=2 type=ptr scope=local
; .debug_var: __main @_l_p offset=0 size=2 type=ptr scope=local
; .debug_var: __main @_l_p2 offset=2 size=2 type=ptr scope=local

@entry:
    .loc "test_malloc.c", 8
    lda #16
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
    .loc "test_malloc.c", 9
    lda _main__local_0
    ldx _main__local_0+1
    stx __zp_scratch
    ora __zp_scratch
    bne @if_then0
    bra @if_end2
@if_then0:
    .loc "test_malloc.c", 11
    lda #32
    ldx #0
    sta _main__local_6
    stx _main__local_6+1
    lda _main__local_6
    ldx _main__local_6+1
    sta $28
    stx $29
    lda _main__local_0
    ldx _main__local_0+1
    sta $2A
    stx $2B
    lda $28
    ldx $29
    push .ax
    lda $2A
    ldx $2B
    push .ax
    jsr _realloc
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_5
    stx _main__local_5+1
    .loc "test_malloc.c", 12
    lda _main__local_5
    ldx _main__local_5+1
    stx __zp_scratch
    ora __zp_scratch
    bne @if_then3
    bra @if_else4
@if_then3:
    .loc "test_malloc.c", 13
    lda _main__local_5
    ldx _main__local_5+1
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
    bra @if_end2
@if_else4:
    .loc "test_malloc.c", 15
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
    .loc "test_malloc.c", 20
    lda #2
    ldx #0
    push .ax
    lda #4
    ldx #0
    push .ax
    jsr _calloc
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_10
    stx _main__local_10+1
    .loc "test_malloc.c", 21
    lda _main__local_10
    ldx _main__local_10+1
    stx __zp_scratch
    ora __zp_scratch
    bne @if_then6
    bra @if_end8
@if_then6:
    .loc "test_malloc.c", 22
    lda _main__local_10
    ldx _main__local_10+1
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
@if_end8:
    .loc "test_malloc.c", 25
    brk
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc


__zp_save_buf:

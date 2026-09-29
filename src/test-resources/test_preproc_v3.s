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

    .extern _printf

    .global _once_func
    .global _main

    .segment "code"

; function _once_func
; SAC zero-alloc leaf: no storage overhead
    proc _once_func
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_preproc_v3.c", 2

@entry:
    .loc "test_preproc_v3.c", 3
@__return:
    rts
    .func_flags stack_call, static_alloc, zeroalloc, leaf
    .frame_size 0
    endproc

; function _main
; SAC inline storage: 2 bytes
    _main__local_0: .word 0
    _main__local_4: .word 0
    _main__local_5: .word 0
    _main__local_9: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_preproc_v3.c", 5
    .local @_l_success = 0
; .debug_var: __main @_l_success offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_preproc_v3.c", 10
    lda #0
    sta _main__local_0
    sta _main__local_0+1
@inline_end0:
    .loc "test_preproc_v3.c", 14
    lda _main__local_0
    ldx _main__local_0+1
    sta $20
    stx $21
    lda $20
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta _main__local_0
    stx _main__local_0+1
    .loc "test_preproc_v3.c", 17
    ldax #__str_2
    sta _main__local_4
    stx _main__local_4+1
    lda #10
    ldx #0
    sta _main__local_5
    stx _main__local_5+1
    lda _main__local_5
    ldx _main__local_5+1
    sta $28
    stx $29
    lda _main__local_4
    ldx _main__local_4+1
    sta $2A
    stx $2B
    lda $28
    ldx $29
    push .ax
    lda $2A
    ldx $2B
    push .ax
    jsr _printf
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    .loc "test_preproc_v3.c", 18
    lda _main__local_0
    ldx _main__local_0+1
    sta $20
    stx $21
    lda $20
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta _main__local_0
    stx _main__local_0+1
    .loc "test_preproc_v3.c", 21
    ldax #__str_3
    sta _main__local_9
    stx _main__local_9+1
    lda _main__local_9
    ldx _main__local_9+1
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    jsr _printf
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    .loc "test_preproc_v3.c", 22
    lda _main__local_0
    ldx _main__local_0+1
    sta $20
    stx $21
    lda $20
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta _main__local_0
    stx _main__local_0+1
    .loc "test_preproc_v3.c", 24
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, #3
    beq @if_then4
    bra @if_end6
@if_then4:
    lda #0
    ldx #0
    bra @__return
@if_end6:
    .loc "test_preproc_v3.c", 25
    lda #1
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc


    .segment "data"
__str_2:
    .text "test %d"
    .byte 0
__str_3:
    .text "test"
    .byte 0

__zp_save_buf:

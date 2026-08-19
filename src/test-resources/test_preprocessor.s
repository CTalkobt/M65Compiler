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

    .global _preproc_success
    .global _assembler_symbol_active
    .global _assembler_symbol_inactive
    .global _included_func
    .global _main

    .segment "code"

; function _preproc_success
; SAC zero-alloc leaf: no storage overhead
    proc _preproc_success
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_preprocessor.c", 3

@entry:
    .loc "test_preprocessor.c", 4
    lda #1
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, zeroalloc, leaf
    .reg_clobbers A, X
    .flag_clobbers N, Z
    .frame_size 0
    endproc

; function _assembler_symbol_active
; SAC zero-alloc leaf: no storage overhead
    proc _assembler_symbol_active
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_preprocessor.c", 13

@entry:
    .loc "test_preprocessor.c", 15
@__return:
    rts
    .func_flags stack_call, static_alloc, zeroalloc, leaf
    .frame_size 0
    endproc

; function _assembler_symbol_inactive
; SAC zero-alloc leaf: no storage overhead
    proc _assembler_symbol_inactive
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_preprocessor.c", 18

@entry:
    .loc "test_preprocessor.c", 20
@__return:
    rts
    .func_flags stack_call, static_alloc, zeroalloc, leaf
    .frame_size 0
    endproc

; function _included_func
; SAC zero-alloc leaf: no storage overhead
    proc _included_func
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_preprocessor.c", 4

@entry:
    .loc "test_preprocessor.c", 5
    lda #10
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, zeroalloc, leaf
    .reg_clobbers A, X
    .flag_clobbers N, Z
    .frame_size 0
    endproc

; function _main
; SAC zero-alloc leaf: no storage overhead
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_inc.h", 3

@entry:
    .loc "test_preprocessor.c", 4
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end0:
    .loc "test_preprocessor.c", 5
    lda #10
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $24
    stx $25
@inline_end2:
    .loc "test_preprocessor.c", 27
    lda $22
    clc
    adc $24
    sta $20
    lda $23
    adc $24+1
    sta $21
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc, zeroalloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 0
    endproc


__zp_save_buf:

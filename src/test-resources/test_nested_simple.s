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

    .global _parent
    .global _parent_nested
    .global _main
    .global _parent_nested

    .segment "code"

; function _parent
; SAC inline storage: 2 bytes
    _parent__local_0: .word 0
    _parent__local_2: .word 0
    proc _parent
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_nested_simple.c", 1
    .local @_l_x = 0
; .debug_var: __parent @_l_x offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_nested_simple.c", 2
    lda #42
    sta _parent__local_0
    lda #0
    sta _parent__local_0+1
    .loc "test_nested_simple.c", 6
    lda $FD
    ldx $FE
    sta $20
    stx $21
    lda $20
    ldx $21
    sta __static_chain
    stx __static_chain+1
    jsr _parent_nested
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _parent_nested
; SAC zero-alloc leaf: no storage overhead
    proc _parent_nested
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_nested_simple.c", 3

@entry:
    lda __static_chain
    ldx __static_chain+1
    sta _parent_nested__local_2
    stx _parent_nested__local_2+1
    .loc "test_nested_simple.c", 4
    lda #43
    ldx #0
    sta $20
    stx $21
    sta $22
    stx $23
    lda $20
    ldx $21
    ldy #0
    sta ($22),y
    txa
    iny
    sta ($22),y
@__return:
    rts
    .func_flags stack_call, static_alloc, zeroalloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers N, Z
    .frame_size 0
    endproc

; function _main
; SAC inline storage: 2 bytes
    _main__local_0: .word 0
    _main__local_2: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_nested_simple.c", 9
    .local @_l_x = 0
; .debug_var: __main @_l_x offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_nested_simple.c", 2
    lda #42
    sta _main__local_0
    lda #0
    sta _main__local_0+1
    .loc "test_nested_simple.c", 6
    lda _main__local_0
    ldx _main__local_0+1
    sta __static_chain
    stx __static_chain+1
    jsr _parent_nested
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _parent_nested
; SAC zero-alloc leaf: no storage overhead
    proc _parent_nested
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_nested_simple.c", 3

@entry:
    lda __static_chain
    ldx __static_chain+1
    sta _parent_nested__local_2
    stx _parent_nested__local_2+1
    .loc "test_nested_simple.c", 4
    lda #43
    ldx #0
    sta $20
    stx $21
    sta $22
    stx $23
    lda $20
    ldx $21
    ldy #0
    sta ($22),y
    txa
    iny
    sta ($22),y
@__return:
    rts
    .func_flags stack_call, static_alloc, zeroalloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers N, Z
    .frame_size 0
    endproc


__zp_save_buf:

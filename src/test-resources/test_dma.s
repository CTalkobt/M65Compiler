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

    .global _test_dma_copy
    .global _test_dma_fill
    .global _main

    .segment "code"

; function _test_dma_copy
; SAC inline storage: 4 bytes
    _test_dma_copy__local_0: .word 0
    _test_dma_copy__local_2: .word 0
    proc _test_dma_copy
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_dma.c", 4
    .local @_l_dst = 2
    .local @_l_src = 0
; .debug_var: __test_dma_copy @_l_dst offset=2 size=2 type=ptr scope=local
; .debug_var: __test_dma_copy @_l_src offset=0 size=2 type=ptr scope=local

@entry:
    .loc "test_dma.c", 5
    lda #0
    ldx #40
    sta $20
    stx $21
    sta _test_dma_copy__local_0
    stx _test_dma_copy__local_0+1
    .loc "test_dma.c", 6
    lda #0
    ldx #41
    sta $20
    stx $21
    sta _test_dma_copy__local_2
    stx _test_dma_copy__local_2+1
    .loc "test_dma.c", 7
    lda #0
    ldx #1
    sta $20
    stx $21
    lda _test_dma_copy__local_0
    ldx _test_dma_copy__local_0+1
    sta $02
    stx $02+1
    lda _test_dma_copy__local_2
    ldx _test_dma_copy__local_2+1
    sta $04
    stx $04+1
    lda $20
    ldx $21
    sta $06
    stx $06+1
    pha
    lda #0
    pha
    pha
    pha
    pha
    lda $05
    pha
    lda $04
    pha
    lda #0
    pha
    lda $03
    pha
    lda $02
    pha
    lda $07
    pha
    lda $06
    pha
    lda #$00
    pha
    tsx
    txa
    clc
    adc #1
    sta $D701
    lda #$01
    sta $D702
    stz $D703
    stz $D700
    tsx
    txa
    clc
    adc #13
    tax
    txs
    pla
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _test_dma_fill
; SAC inline storage: 2 bytes
    _test_dma_fill__local_0: .word 0
    proc _test_dma_fill
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_dma.c", 10
    .local @_l_dst = 0
; .debug_var: __test_dma_fill @_l_dst offset=0 size=2 type=ptr scope=local

@entry:
    .loc "test_dma.c", 11
    lda #0
    ldx #4
    sta $20
    stx $21
    sta _test_dma_fill__local_0
    stx _test_dma_fill__local_0+1
    .loc "test_dma.c", 12
    lda #232
    ldx #3
    sta $20
    stx $21
    lda #32
    ldx #0
    sta $22
    stx $23
    lda _test_dma_fill__local_0
    ldx _test_dma_fill__local_0+1
    sta $04
    stx $04+1
    lda $20
    ldx $21
    sta $06
    stx $06+1
    lda $22
    sta $02
    pha
    lda #0
    pha
    pha
    pha
    pha
    lda $05
    pha
    lda $04
    pha
    lda #0
    pha
    lda $02
    pha
    lda #0
    pha
    lda $07
    pha
    lda $06
    pha
    lda #$03
    pha
    tsx
    txa
    clc
    adc #1
    sta $D701
    lda #$01
    sta $D702
    stz $D703
    stz $D700
    tsx
    txa
    clc
    adc #13
    tax
    txs
    pla
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _main
; SAC zero-alloc leaf: no storage overhead
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_dma.c", 15

@entry:
    .loc "test_dma.c", 16
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

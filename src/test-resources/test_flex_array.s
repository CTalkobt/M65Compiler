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

    .global _main

    .segment "code"

; function _main
; SAC inline storage: 8 bytes
    _main__local_0: .word 0
    _main__local_2: .word 0
    _main__local_5: .word 0
    _main__local_11: .word 0
    _main__local_15: .word 0
    _main__local_18: .word 0
    _main__local_21: .word 0
    _main__local_27: .word 0
    _main__local_30: .word 0
    _main__local_31: .word 0
    _main__local_35: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_flex_array.c", 12
    .local @_l_b = 4
    .local @_l_p = 2
    .local @_l_result = 6
    .local @_l_sz = 0
; .debug_var: __main @_l_b offset=4 size=2 type=ptr scope=local
; .debug_var: __main @_l_p offset=2 size=2 type=ptr scope=local
; .debug_var: __main @_l_result offset=6 size=2 type=int16 scope=local
; .debug_var: __main @_l_sz offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_flex_array.c", 14
    lda #2
    sta _main__local_0
    lda #0
    sta _main__local_0+1
    .loc "test_flex_array.c", 17
    lda #0
    ldx #64
    sta $20
    stx $21
    sta _main__local_2
    stx _main__local_2+1
    .loc "test_flex_array.c", 18
    lda #3
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_2
    stx _main__local_2+1
    .loc "test_flex_array.c", 19
    lda #65
    sta $20
    lda _main__local_2
    ldx _main__local_2+1
    add.16 .AX, #2
    sta $22
    stx $23
    lda #0
    sta $24
    sta $25
    lda $20
    ldx #0
    pha
    lda $24
    ldx $25
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $22
    ldx $22+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "test_flex_array.c", 20
    lda #66
    sta $20
    lda #1
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx #0
    pha
    lda $24
    ldx $25
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $22
    ldx $22+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "test_flex_array.c", 21
    lda #67
    sta $20
    lda #2
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx #0
    pha
    lda $24
    ldx $25
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $22
    ldx $22+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "test_flex_array.c", 24
    lda #0
    ldx #65
    sta $20
    stx $21
    sta _main__local_18
    stx _main__local_18+1
    .loc "test_flex_array.c", 25
    lda #2
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_18
    stx _main__local_18+1
    .loc "test_flex_array.c", 26
    lda #100
    ldx #0
    sta $20
    stx $21
    lda _main__local_18
    ldx _main__local_18+1
    add.16 .AX, #2
    sta $24
    stx $25
    lda #0
    sta $26
    sta $27
    lda $20
    ldx $21
    pha
    phx
    lda $26
    ldx $27
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $24
    ldx $24+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    .loc "test_flex_array.c", 27
    lda #200
    ldx #0
    sta $20
    stx $21
    lda #1
    ldx #0
    sta $26
    stx $27
    lda $20
    ldx $21
    pha
    phx
    lda $26
    ldx $27
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $24
    ldx $24+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    .loc "test_flex_array.c", 29
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $22
    ldx $22+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $28
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $24
    ldx $24+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $26
    stx $27
    lda $28
    ldx #0
    ldx #0
    sta $20
    stx $21
    lda $20
    clc
    adc $26
    sta $22
    lda $21
    adc $26+1
    sta $23
    lda $22
    ldx $23
    sta _main__local_30
    stx _main__local_30+1
    .loc "test_flex_array.c", 30
    lda _main__local_30
    ldx _main__local_30+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_0
    ldx _main__local_0+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 8
    endproc


__zp_save_buf:

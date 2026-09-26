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
; SAC inline storage: 14 bytes
    _main__local_0: .long 0
    _main__local_3: .word 0
    _main__local_7: .word 0
    _main__local_10: .word 0
    _main__local_11: .word 0
    _main__local_14: .word 0
    _main__local_15: .word 0
    _main__local_21: .word 0
    _main__local_28: .word 0
    _main__local_29: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_struct.c", 11
    .local @_l_p = 2
    .local @_l_pp = 0
    .local @_l_r = 6
; .debug_var: __main @_l_p offset=2 size=4 type=int32 scope=local
; .debug_var: __main @_l_pp offset=0 size=2 type=ptr scope=local
; .debug_var: __main @_l_r offset=6 size=2 type=int16 scope=local

@entry:
    .loc "test_struct.c", 13
    lda #10
    ldx #0
    sta $20
    stx $21
    leax.local 2
    sta $22
    stx $23
    lda $20
    ldx $21
    ldy #0
    sta ($22),y
    txa
    iny
    sta ($22),y
    .loc "test_struct.c", 14
    lda #20
    ldx #0
    sta $20
    stx $21
    leax.local 2
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #2
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_struct.c", 16
    leax.local 2
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_7
    stx _main__local_7+1
    .loc "test_struct.c", 17
    lda #30
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_7
    stx _main__local_7+1
    .loc "test_struct.c", 20
    lda #100
    ldx #0
    sta $20
    stx $21
    leax.local 6
    sta $22
    stx $23
    lda $20
    ldx $21
    ldy #0
    sta ($22),y
    txa
    iny
    sta ($22),y
    .loc "test_struct.c", 21
    lda #200
    ldx #0
    sta $20
    stx $21
    leax.local 6
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #4
    sta $24
    stx $25
    lda $24
    ldx $25
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
    .loc "test_struct.c", 23
    leax.local 2
    sta $20
    stx $21
    ldy #0
    lda ($20),y
    pha
    iny
    lda ($20),y
    tax
    pla
    sta $22
    stx $23
    leax.local 2
    sta $20
    stx $21
    lda $20
    ldx $21
    add.16 .AX, #2
    sta $24
    stx $25
    ldy #0
    lda ($24),y
    pha
    iny
    lda ($24),y
    tax
    pla
    sta $20
    stx $21
    lda $22
    clc
    adc $20
    sta $24
    lda $23
    adc $20+1
    sta $25
    leax.local 6
    sta $20
    stx $21
    ldy #0
    lda ($20),y
    pha
    iny
    lda ($20),y
    tax
    pla
    sta $22
    stx $23
    lda $24
    clc
    adc $22
    sta $20
    lda $25
    adc $22+1
    sta $21
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 14
    endproc


__zp_save_buf:

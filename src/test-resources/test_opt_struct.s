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
; SAC inline storage: 7 bytes
    _main__local_0: .word 0
    _main__local_3: .word 0
    _main__local_12: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_opt_struct.c", 7
    .local @_l_d = 2
    .local @_l_p = 0
; .debug_var: __main @_l_d offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_p offset=0 size=2 type=ptr scope=local

@entry:
    .loc "test_opt_struct.c", 9
    lda #1
    sta $20
    leax.local 2
    sta $22
    stx $23
    lda $20
    ldy #0
    sta ($22),y
    .loc "test_opt_struct.c", 10
    lda #244
    ldx #1
    sta $20
    stx $21
    leax.local 2
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #1
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_opt_struct.c", 11
    leax.local 2
    sta $20
    stx $21
    lda $20
    ldx $21
    add.16 .AX, #1
    sta $22
    stx $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    leax.local 2
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #3
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_opt_struct.c", 13
    leax.local 2
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_12
    stx _main__local_12+1
    .loc "test_opt_struct.c", 14
    lda #232
    ldx #3
    sta $20
    stx $21
    lda _main__local_12
    ldx _main__local_12+1
    add.16 .AX, #1
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
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 7
    endproc


__zp_save_buf:

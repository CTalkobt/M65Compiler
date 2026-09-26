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
; SAC inline storage: 19 bytes
    _main__local_0: .long 0
    _main__local_3: .word 0
    _main__local_4: .long 0
    _main__local_6: .word 0
    _main__local_8: .word 0
    _main__local_11: .word 0
    _main__local_18: .long 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_struct_long_field.c", 16
    .local @_l_d = 0
    .local @_l_m = 12
    .local @_l_x = 4
    .local @_l_y = 8
; .debug_var: __main @_l_d offset=0 size=4 type=int32 scope=local
; .debug_var: __main @_l_m offset=12 size=2 type=int16 scope=local
; .debug_var: __main @_l_x offset=4 size=4 type=int32 scope=local
; .debug_var: __main @_l_y offset=8 size=4 type=int32 scope=local

@entry:
    .loc "test_struct_long_field.c", 18
    lda #120
    ldx #86
    ldy #52
    ldz #18
    sta $20
    stx $21
    sty $22
    stz $23
    leax.local 0
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy $22
    ldz $23
    ldy #0
    sty __zp_scratch3
    stz __zp_scratch3+1
    sta ($24),y
    txa
    iny
    sta ($24),y
    lda __zp_scratch3
    iny
    sta ($24),y
    lda __zp_scratch3+1
    iny
    sta ($24),y
    .loc "test_struct_long_field.c", 19
    leax.local 0
    sta $20
    stx $21
    ldy #0
    lda ($20),y
    pha
    iny
    lda ($20),y
    pha
    iny
    lda ($20),y
    pha
    iny
    lda ($20),y
    taz
    ply
    plx
    pla
    sta $22
    stx $23
    sty $24
    stz $25
    lda $22
    ldx $23
    ldy $24
    ldz $25
    sta _main__local_4
    stx _main__local_4+1
    sty _main__local_4+2
    stz _main__local_4+3
    .loc "test_struct_long_field.c", 22
    lda #34
    ldx #17
    sta $20
    stx $21
    leax.local 12
    sta $22
    stx $23
    lda $20
    ldx $21
    ldy #0
    sta ($22),y
    txa
    iny
    sta ($22),y
    .loc "test_struct_long_field.c", 23
    lda #221
    ldx #204
    ldy #187
    ldz #170
    sta $20
    stx $21
    sty $22
    stz $23
    leax.local 12
    sta $24
    stx $25
    lda $24
    ldx $25
    add.16 .AX, #2
    sta $26
    stx $27
    lda $20
    ldx $21
    ldy $22
    ldz $23
    ldy #0
    sty __zp_scratch3
    stz __zp_scratch3+1
    sta ($26),y
    txa
    iny
    sta ($26),y
    lda __zp_scratch3
    iny
    sta ($26),y
    lda __zp_scratch3+1
    iny
    sta ($26),y
    .loc "test_struct_long_field.c", 24
    lda #42
    sta $20
    leax.local 12
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #6
    sta $24
    stx $25
    lda $20
    ldy #0
    sta ($24),y
    .loc "test_struct_long_field.c", 26
    leax.local 12
    sta $20
    stx $21
    lda $20
    ldx $21
    add.16 .AX, #2
    sta $22
    stx $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    pha
    iny
    lda ($22),y
    pha
    iny
    lda ($22),y
    taz
    ply
    plx
    pla
    sta $24
    stx $25
    sty $26
    stz $27
    lda $24
    ldx $25
    ldy $26
    ldz $27
    sta _main__local_18
    stx _main__local_18+1
    sty _main__local_18+2
    stz _main__local_18+3
    .loc "test_struct_long_field.c", 28
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 19
    endproc


__zp_save_buf:

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

    .global _test_symbolic_fp
    .global _test_array_fp

    .segment "code"

; function _test_symbolic_fp
; SAC inline storage: 6 bytes
    _test_symbolic_fp__local_0: .word 0
    _test_symbolic_fp__local_2: .word 0
    _test_symbolic_fp__local_4: .word 0
    proc _test_symbolic_fp
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_symbolic_fp.c", 2
    .local @_l_x = 0
    .local @_l_y = 2
    .local @_l_z = 4
; .debug_var: __test_symbolic_fp @_l_x offset=0 size=2 type=int16 scope=local
; .debug_var: __test_symbolic_fp @_l_y offset=2 size=2 type=int16 scope=local
; .debug_var: __test_symbolic_fp @_l_z offset=4 size=2 type=int8 scope=local

@entry:
    .loc "test_symbolic_fp.c", 3
    lda #42
    sta _test_symbolic_fp__local_0
    lda #0
    sta _test_symbolic_fp__local_0+1
    .loc "test_symbolic_fp.c", 4
    lda #100
    sta _test_symbolic_fp__local_2
    lda #0
    sta _test_symbolic_fp__local_2+1
    .loc "test_symbolic_fp.c", 5
    lda #5
    sta _test_symbolic_fp__local_4
    lda #0
    sta _test_symbolic_fp__local_4+1
    .loc "test_symbolic_fp.c", 12
    lda _test_symbolic_fp__local_2
    ldx _test_symbolic_fp__local_2+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _test_symbolic_fp__local_0
    ldx _test_symbolic_fp__local_0+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda _test_symbolic_fp__local_4
    ldx #0
    sta $22
    stx $23
    lda $20
    clc
    adc $22
    sta $24
    lda $21
    adc $22+1
    sta $25
    lda $24
    ldx $25
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _test_array_fp
; SAC inline storage: 8 bytes
    _test_array_fp__local_0: .word 0
    proc _test_array_fp
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_symbolic_fp.c", 16
    .local @_l_arr = 0
; .debug_var: __test_array_fp @_l_arr offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_symbolic_fp.c", 18
    lda #1
    ldx #0
    sta $20
    stx $21
    leax.local 0
    sta $22
    stx $23
    lda #0
    sta $24
    sta $25
    lda $20
    ldx $21
    pha
    phx
    lda $24
    ldx $25
    mul.16 .AX, #2
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
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    .loc "test_symbolic_fp.c", 19
    lda #2
    ldx #0
    sta $20
    stx $21
    leax.local 0
    sta $22
    stx $23
    lda #1
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx $21
    pha
    phx
    lda $24
    ldx $25
    mul.16 .AX, #2
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
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    .loc "test_symbolic_fp.c", 20
    lda #3
    ldx #0
    sta $20
    stx $21
    leax.local 0
    sta $22
    stx $23
    lda #2
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx $21
    pha
    phx
    lda $24
    ldx $25
    mul.16 .AX, #2
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
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    .loc "test_symbolic_fp.c", 21
    lda #4
    ldx #0
    sta $20
    stx $21
    leax.local 0
    sta $22
    stx $23
    lda #3
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx $21
    pha
    phx
    lda $24
    ldx $25
    mul.16 .AX, #2
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
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    .loc "test_symbolic_fp.c", 22
    leax.local 0
    sta $20
    stx $21
    lda #0
    sta $22
    sta $23
    lda $22
    ldx $23
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
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
    leax.local 0
    sta $20
    stx $21
    lda #1
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
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
    sta $28
    stx $29
    lda $26
    clc
    adc $28
    sta $20
    lda $27
    adc $28+1
    sta $21
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

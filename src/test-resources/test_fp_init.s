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

    .global _test_fp_with_init
    .global _test_fp_multi_byte

    .segment "code"

; function _test_fp_with_init
; SAC inline storage: 8 bytes
    _test_fp_with_init__local_0: .word 0
    _test_fp_with_init__local_2: .word 0
    _test_fp_with_init__local_4: .word 0
    _test_fp_with_init__local_6: .word 0
    proc _test_fp_with_init
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_fp_init.c", 2
    .local @_l_a = 0
    .local @_l_b = 2
    .local @_l_c = 4
    .local @_l_d = 6
; .debug_var: __test_fp_with_init @_l_a offset=0 size=2 type=int16 scope=local
; .debug_var: __test_fp_with_init @_l_b offset=2 size=2 type=int16 scope=local
; .debug_var: __test_fp_with_init @_l_c offset=4 size=2 type=int16 scope=local
; .debug_var: __test_fp_with_init @_l_d offset=6 size=2 type=int8 scope=local

@entry:
    .loc "test_fp_init.c", 3
    lda #10
    sta _test_fp_with_init__local_0
    lda #0
    sta _test_fp_with_init__local_0+1
    .loc "test_fp_init.c", 4
    lda #20
    sta _test_fp_with_init__local_2
    lda #0
    sta _test_fp_with_init__local_2+1
    .loc "test_fp_init.c", 5
    lda #30
    sta _test_fp_with_init__local_4
    lda #0
    sta _test_fp_with_init__local_4+1
    .loc "test_fp_init.c", 6
    lda #5
    sta _test_fp_with_init__local_6
    lda #0
    sta _test_fp_with_init__local_6+1
    .loc "test_fp_init.c", 7
    lda _test_fp_with_init__local_2
    ldx _test_fp_with_init__local_2+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _test_fp_with_init__local_0
    ldx _test_fp_with_init__local_0+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda _test_fp_with_init__local_4
    ldx _test_fp_with_init__local_4+1
    add.16 .AX, $20
    sta $22
    stx $23
    lda _test_fp_with_init__local_6
    ldx #0
    sta $20
    stx $21
    lda $22
    clc
    adc $20
    sta $24
    lda $23
    adc $20+1
    sta $25
    lda $24
    ldx $25
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 8
    endproc

; function _test_fp_multi_byte
; SAC inline storage: 6 bytes
    _test_fp_multi_byte__local_0: .long 0
    _test_fp_multi_byte__local_2: .word 0
    proc _test_fp_multi_byte
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_fp_init.c", 10
    .local @_l_x = 2
    .local @_l_y = 0
; .debug_var: __test_fp_multi_byte @_l_x offset=2 size=4 type=int32 scope=local
; .debug_var: __test_fp_multi_byte @_l_y offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_fp_init.c", 11
    lda #120
    ldx #86
    ldy #52
    ldz #18
    sta $20
    stx $21
    sty $22
    stz $23
    sta _test_fp_multi_byte__local_0
    stx _test_fp_multi_byte__local_0+1
    sty _test_fp_multi_byte__local_0+2
    stz _test_fp_multi_byte__local_0+3
    .loc "test_fp_init.c", 12
    lda #0
    sta _test_fp_multi_byte__local_2
    lda #1
    sta _test_fp_multi_byte__local_2+1
    lda _test_fp_multi_byte__local_2
    ldx _test_fp_multi_byte__local_2+1
    ldy #0
    ldz #0
    sta $20
    stx $21
    sty $22
    stz $23
    .loc "test_fp_init.c", 13
    lda _test_fp_multi_byte__local_0
    ldx _test_fp_multi_byte__local_0+1
    ldy _test_fp_multi_byte__local_0+2
    ldz _test_fp_multi_byte__local_0+3
    add.32 .AXYZ, $20
    sta $24
    stx $25
    sty $26
    stz $27
    lda $24
    ldx $25
    ldy $26
    ldz $27
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc


__zp_save_buf:

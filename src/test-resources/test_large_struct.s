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

    .extern __memcpy
    .extern __memset

    .global _test

    .segment "code"

; function _test
; SAC inline storage: 20 bytes
    _test__local_0: .word 0
    _test__local_1: .word 0
    _test__local_2: .word 0
    _test__local_3: .word 0
    _test__local_4: .word 0
    _test__local_6: .word 0
    _test__local_7: .word 0
    _test__local_8: .word 0
    proc _test
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_large_struct.c", 9
    .local @_l_a = 0
    .local @_l_b = 10
; .debug_var: __test @_l_a offset=0 size=2 type=int16 scope=local
; .debug_var: __test @_l_b offset=10 size=2 type=int16 scope=local

@entry:
    .loc "test_large_struct.c", 14
    leax.local 0
    sta _test__local_2
    stx _test__local_2+1
    lda #65
    ldx #0
    sta _test__local_3
    stx _test__local_3+1
    lda #10
    ldx #0
    sta _test__local_4
    stx _test__local_4+1
    lda _test__local_4
    ldx _test__local_4+1
    sta $28
    stx $29
    lda _test__local_3
    ldx _test__local_3+1
    sta $2A
    stx $2B
    lda _test__local_2
    ldx _test__local_2+1
    sta $2C
    stx $2D
    lda $28
    ldx $29
    push .ax
    lda $2A
    ldx $2B
    push .ax
    lda $2C
    ldx $2D
    push .ax
    jsr __memset
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    .loc "test_large_struct.c", 17
    leax.local 10
    sta _test__local_6
    stx _test__local_6+1
    leax.local 0
    sta _test__local_7
    stx _test__local_7+1
    lda #10
    ldx #0
    sta _test__local_8
    stx _test__local_8+1
    lda _test__local_8
    ldx _test__local_8+1
    sta $28
    stx $29
    lda _test__local_7
    ldx _test__local_7+1
    sta $2A
    stx $2B
    lda _test__local_6
    ldx _test__local_6+1
    sta $2C
    stx $2D
    lda $28
    ldx $29
    push .ax
    lda $2A
    ldx $2B
    push .ax
    lda $2C
    ldx $2D
    push .ax
    jsr __memcpy
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    .loc "test_large_struct.c", 20
    lda _test__local_1
    ldx _test__local_1+1
    sta _test__local_0
    stx _test__local_0+1
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 20
    endproc


__zp_save_buf:

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

    .global _result
    .global _result16
    .global _main

    .segment "bss"
_result:
; .debug_var: @global _result offset=0 size=2 type=int8 scope=global
    .res 1
_result16:
; .debug_var: @global _result16 offset=0 size=2 type=int16 scope=global
    .res 2

    .segment "code"

; function _main
; SAC inline storage: 4 bytes
    _main__local_0: .word 0
    _main__local_2: .word 0
    _main__local_5: .word 0
    _main__local_8: .word 0
    _main__local_11: .word 0
    _main__local_15: .word 0
    _main__local_19: .word 0
    _main__local_23: .word 0
    _main__local_28: .word 0
    _main__local_30: .word 0
    _main__local_32: .word 0
    _main__local_35: .word 0
    _main__local_38: .word 0
    _main__local_42: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "src/test-resources/test_bitfield.c", 16
    .local @_l_f = 0
    .local @_l_wf = 2
; .debug_var: __main @_l_f offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_wf offset=2 size=2 type=int16 scope=local

@entry:
    .loc "src/test-resources/test_bitfield.c", 20
    lda #1
    lda _main__local_0
    ldx _main__local_0+1
    sta __zp_scratch3
    stx __zp_scratch3+1
    bfins __zp_scratch3, #0, #1
    sta _main__local_0
    stx _main__local_0+1
    .loc "src/test-resources/test_bitfield.c", 21
    lda #5
    lda _main__local_0
    ldx _main__local_0+1
    sta __zp_scratch3
    stx __zp_scratch3+1
    bfins __zp_scratch3, #1, #3
    sta _main__local_0
    stx _main__local_0+1
    .loc "src/test-resources/test_bitfield.c", 22
    lda #12
    lda _main__local_0
    ldx _main__local_0+1
    sta __zp_scratch3
    stx __zp_scratch3+1
    bfins __zp_scratch3, #4, #4
    sta _main__local_0
    stx _main__local_0+1
    .loc "src/test-resources/test_bitfield.c", 25
    leax.local 0
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    bfext #0, #1
    sta _result
    .loc "src/test-resources/test_bitfield.c", 26
    leax.local 0
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    bfext #1, #3
    sta _result
    .loc "src/test-resources/test_bitfield.c", 27
    leax.local 0
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    bfext #4, #4
    sta _result
    .loc "src/test-resources/test_bitfield.c", 30
    leax.local 0
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    bfext #1, #3
    inc a
    lda _main__local_0
    ldx _main__local_0+1
    sta __zp_scratch3
    stx __zp_scratch3+1
    bfins __zp_scratch3, #1, #3
    sta _main__local_0
    stx _main__local_0+1
    .loc "src/test-resources/test_bitfield.c", 34
    lda #244
    ldx #1
    lda _main__local_30
    ldx _main__local_30+1
    sta __zp_scratch3
    stx __zp_scratch3+1
    bfins16 __zp_scratch3, #0, #10
    sta _main__local_30
    stx _main__local_30+1
    .loc "src/test-resources/test_bitfield.c", 35
    lda #30
    ldx #0
    lda _main__local_30
    ldx _main__local_30+1
    sta __zp_scratch3
    stx __zp_scratch3+1
    bfins16 __zp_scratch3, #10, #6
    sta _main__local_30
    stx _main__local_30+1
    .loc "src/test-resources/test_bitfield.c", 36
    leax.local 2
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    bfext16 #0, #10
    sta _result16
    stx _result16+1
    .loc "src/test-resources/test_bitfield.c", 37
    leax.local 2
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    bfext16 #10, #6
    sta _result16
    stx _result16+1
    .loc "src/test-resources/test_bitfield.c", 39
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc


__zp_save_buf:

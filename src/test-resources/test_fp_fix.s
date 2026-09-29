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

    .global _test_result
    .global _my_func
    .global _call_fp
    .global _main

    .segment "data"
    .byte 0
_test_result:
; .debug_var: @global _test_result offset=0 size=2 type=int16 scope=global
    .word 0

    .segment "code"

; function _my_func
; SAC inline storage: 2 bytes
    .global _my_func__param_x
    _my_func__param_x: .word 0
    _my_func__local_0: .word 0
    proc _my_func, W#@_p_x
    .sac
    .var _fp = 0
    .loc "test_fp_fix.c", 4
    .var @_p_x = 2
; .debug_var: __my_func @_p_x offset=2 size=2 type=int16 scope=parameter

@entry:
    .loc "test_fp_fix.c", 5
    lda #85
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _test_result
    stx _test_result+1
    .loc "test_fp_fix.c", 6
    lda _my_func__param_x
    ldx _my_func__param_x+1
    add.16 .AX, #1
    sta $22
    stx $23
    lda $22
    ldx $23
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _call_fp
; SAC inline storage: 6 bytes
    .global _call_fp__param_fp
    _call_fp__param_fp: .word 0
    .global _call_fp__param_val
    _call_fp__param_val: .word 0
    _call_fp__local_0: .word 0
    _call_fp__local_1: .word 0
    _call_fp__local_3: .word 0
    proc _call_fp, W#@_p_fp, W#@_p_val
    .sac
    .var _fp = 0
    .loc "test_fp_fix.c", 9
    .local @_l_result = 4
; .debug_var: __call_fp @_l_result offset=4 size=2 type=int16 scope=local
    .var @_p_fp = 2
    .var @_p_val = 4
; .debug_var: __call_fp @_p_fp offset=2 size=2 type=ptr scope=parameter
; .debug_var: __call_fp @_p_val offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_fp_fix.c", 10
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta _test_result
    stx _test_result+1
    .loc "test_fp_fix.c", 11
    lda _call_fp__param_val
    ldx _call_fp__param_val+1
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    lda _call_fp__param_fp
    ldx _call_fp__param_fp+1
    sta @__call_site_0+1
    stx @__call_site_0+2
@__call_site_0:
    jsr $0000
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _call_fp__local_3
    stx _call_fp__local_3+1
    .loc "test_fp_fix.c", 12
    lda _call_fp__local_3
    ldx _call_fp__local_3+1
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _main
; SAC inline storage: 4 bytes
    _main__local_1: .word 0
    _main__local_6: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_fp_fix.c", 15
    .local @_l_result = 2
; .debug_var: __main @_l_result offset=2 size=2 type=int16 scope=local

@entry:
    .loc "test_fp_fix.c", 16
    lda #42
    ldx #0
    sta _main__local_1
    stx _main__local_1+1
    ldax #_my_func
    sta $20
    stx $21
    lda _main__local_1
    ldx _main__local_1+1
    sta $20
    stx $21
    .loc "test_fp_fix.c", 10
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta _test_result
    stx _test_result+1
    .loc "test_fp_fix.c", 11
    lda #42
    ldx #0
    push .ax
    ldax #_my_func
    sta @__call_site_1+1
    stx @__call_site_1+2
@__call_site_1:
    jsr $0000
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_6
    stx _main__local_6+1
    .loc "test_fp_fix.c", 12
    lda _main__local_6
    ldx _main__local_6+1
    sta $20
    stx $21
@inline_end0:
    .loc "test_fp_fix.c", 16
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_fp_fix.c", 19
    lda _test_result
    ldx _test_result+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #85
    beq @sc_merge2
    bra @sc_short3
@sc_merge2:
    lda $22
    ldx $23
    cmp.16 .AX, #43
    beq @__cmp_set_0
    bra @__cmp_zero_0
@__cmp_set_0:
    lda #1
    ldx #0
    bra @__cmp_done_0
@__cmp_zero_0:
    lda #0
    ldx #0
@__cmp_done_0:
    sta $24
    bra @sc_done4
@sc_short3:
@sc_done4:
    lda $24
    bne @tern_then5
    bra @tern_else6
@tern_then5:
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @tern_end7
@tern_else6:
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@tern_end7:
    lda $22
    ldx $23
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc


__zp_save_buf:

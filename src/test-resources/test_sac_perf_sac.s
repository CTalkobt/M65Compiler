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

    .global _add_nums
    .global _calculate
    .global _complex_calc
    .global _sum_via_calls
    .global _five_add
    .global _main

    .segment "code"

; function _add_nums
; SAC inline storage: 4 bytes
    .global _add_nums__param_a
    _add_nums__param_a: .word 0
    .global _add_nums__param_b
    _add_nums__param_b: .word 0
    _add_nums__local_0: .word 0
    _add_nums__local_1: .word 0
    proc _add_nums, W#@_p_a, W#@_p_b
    .sac
    .var _fp = 0
    .loc "test_sac_perf_sac.c", 9
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __add_nums @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __add_nums @_p_b offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_sac_perf_sac.c", 10
    lda _add_nums__param_b
    ldx _add_nums__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _add_nums__param_a
    ldx _add_nums__param_a+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _calculate
; SAC inline storage: 12 bytes
    .global _calculate__param_x
    _calculate__param_x: .word 0
    .global _calculate__param_y
    _calculate__param_y: .word 0
    .global _calculate__param_z
    _calculate__param_z: .word 0
    _calculate__local_0: .word 0
    _calculate__local_1: .word 0
    _calculate__local_2: .word 0
    _calculate__local_3: .word 0
    _calculate__local_5: .word 0
    _calculate__local_7: .word 0
    proc _calculate, W#@_p_x, W#@_p_y, W#@_p_z
    .sac
    .var _fp = 0
    .loc "test_sac_perf_sac.c", 14
    .local @_l_result = 10
    .local @_l_temp1 = 6
    .local @_l_temp2 = 8
; .debug_var: __calculate @_l_result offset=10 size=2 type=int16 scope=local
; .debug_var: __calculate @_l_temp1 offset=6 size=2 type=int16 scope=local
; .debug_var: __calculate @_l_temp2 offset=8 size=2 type=int16 scope=local
    .var @_p_x = 2
    .var @_p_y = 4
    .var @_p_z = 6
; .debug_var: __calculate @_p_x offset=2 size=2 type=int16 scope=parameter
; .debug_var: __calculate @_p_y offset=4 size=2 type=int16 scope=parameter
; .debug_var: __calculate @_p_z offset=6 size=2 type=int16 scope=parameter

@entry:
    .loc "test_sac_perf_sac.c", 15
    lda _calculate__param_y
    ldx _calculate__param_y+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _calculate__param_x
    ldx _calculate__param_x+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    sta _calculate__local_3
    stx _calculate__local_3+1
    .loc "test_sac_perf_sac.c", 16
    lda _calculate__param_z
    ldx _calculate__param_z+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _calculate__param_y
    ldx _calculate__param_y+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    sta _calculate__local_5
    stx _calculate__local_5+1
    .loc "test_sac_perf_sac.c", 17
    lda _calculate__local_5
    ldx _calculate__local_5+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _calculate__local_3
    ldx _calculate__local_3+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    sta _calculate__local_7
    stx _calculate__local_7+1
    .loc "test_sac_perf_sac.c", 18
    lda _calculate__local_7
    ldx _calculate__local_7+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 12
    endproc

; function _complex_calc
; SAC inline storage: 12 bytes
    .global _complex_calc__param_a
    _complex_calc__param_a: .word 0
    .global _complex_calc__param_b
    _complex_calc__param_b: .word 0
    .global _complex_calc__param_c
    _complex_calc__param_c: .word 0
    .global _complex_calc__param_d
    _complex_calc__param_d: .word 0
    _complex_calc__local_0: .word 0
    _complex_calc__local_1: .word 0
    _complex_calc__local_2: .word 0
    _complex_calc__local_3: .word 0
    _complex_calc__local_4: .word 0
    _complex_calc__local_9: .word 0
    proc _complex_calc, W#@_p_a, W#@_p_b, W#@_p_c, W#@_p_d
    .sac
    .var _fp = 0
    .loc "test_sac_perf_sac.c", 22
    .local @_l_sum1 = 8
    .local @_l_sum2 = 10
; .debug_var: __complex_calc @_l_sum1 offset=8 size=2 type=int16 scope=local
; .debug_var: __complex_calc @_l_sum2 offset=10 size=2 type=int16 scope=local
    .var @_p_a = 2
    .var @_p_b = 4
    .var @_p_c = 6
    .var @_p_d = 8
; .debug_var: __complex_calc @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __complex_calc @_p_b offset=4 size=2 type=int16 scope=parameter
; .debug_var: __complex_calc @_p_c offset=6 size=2 type=int16 scope=parameter
; .debug_var: __complex_calc @_p_d offset=8 size=2 type=int16 scope=parameter

@entry:
    .loc "test_sac_perf_sac.c", 23
    lda _complex_calc__param_a
    ldx _complex_calc__param_a+1
    sta $20
    stx $21
    lda _complex_calc__param_b
    ldx _complex_calc__param_b+1
    sta $20
    stx $21
    .loc "test_sac_perf_sac.c", 10
    lda _complex_calc__param_b
    ldx _complex_calc__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _complex_calc__param_a
    ldx _complex_calc__param_a+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end0:
    .loc "test_sac_perf_sac.c", 23
    lda $22
    ldx $23
    sta _complex_calc__local_4
    stx _complex_calc__local_4+1
    .loc "test_sac_perf_sac.c", 24
    lda _complex_calc__param_c
    ldx _complex_calc__param_c+1
    sta $20
    stx $21
    lda _complex_calc__param_d
    ldx _complex_calc__param_d+1
    sta $20
    stx $21
    .loc "test_sac_perf_sac.c", 10
    lda _complex_calc__param_d
    ldx _complex_calc__param_d+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _complex_calc__param_c
    ldx _complex_calc__param_c+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end2:
    .loc "test_sac_perf_sac.c", 24
    lda $22
    ldx $23
    sta _complex_calc__local_9
    stx _complex_calc__local_9+1
    .loc "test_sac_perf_sac.c", 25
    lda _complex_calc__local_9
    ldx _complex_calc__local_9+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _complex_calc__local_4
    ldx _complex_calc__local_4+1
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
    .frame_size 12
    endproc

; function _sum_via_calls
; SAC inline storage: 6 bytes
    .global _sum_via_calls__param_n
    _sum_via_calls__param_n: .word 0
    _sum_via_calls__local_0: .word 0
    _sum_via_calls__local_1: .word 0
    _sum_via_calls__local_3: .word 0
    proc _sum_via_calls, W#@_p_n
    .sac
    .var _fp = 0
    .loc "test_sac_perf_sac.c", 29
    .local @_l_i = 4
    .local @_l_total = 2
; .debug_var: __sum_via_calls @_l_i offset=4 size=2 type=int16 scope=local
; .debug_var: __sum_via_calls @_l_total offset=2 size=2 type=int16 scope=local
    .var @_p_n = 2
; .debug_var: __sum_via_calls @_p_n offset=2 size=2 type=int16 scope=parameter

@entry:
    .loc "test_sac_perf_sac.c", 30
    lda #0
    sta _sum_via_calls__local_1
    sta _sum_via_calls__local_1+1
    .loc "test_sac_perf_sac.c", 31
    lda #0
    sta _sum_via_calls__local_3
    sta _sum_via_calls__local_3+1
@while_cond4_ph:
    .loc "test_sac_perf_sac.c", 34
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@while_cond4:
    .loc "test_sac_perf_sac.c", 32
    lda _sum_via_calls__param_n
    ldx _sum_via_calls__param_n+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _sum_via_calls__local_3
    ldx _sum_via_calls__local_3+1
    cmp.16 .AX, __zp_scratch2
    bcc @while_body5
    bra @while_end6
@while_body5:
    .loc "test_sac_perf_sac.c", 33
    lda _sum_via_calls__local_1
    ldx _sum_via_calls__local_1+1
    sta $22
    stx $23
    lda _sum_via_calls__local_3
    ldx _sum_via_calls__local_3+1
    sta $24
    stx $25
    .loc "test_sac_perf_sac.c", 10
    lda _sum_via_calls__local_3
    ldx _sum_via_calls__local_3+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _sum_via_calls__local_1
    ldx _sum_via_calls__local_1+1
    add.16 .AX, __zp_scratch2
    sta $26
    stx $27
    lda $26
    ldx $27
    sta $28
    stx $29
@inline_end7:
    .loc "test_sac_perf_sac.c", 33
    lda $28
    ldx $29
    sta _sum_via_calls__local_1
    stx _sum_via_calls__local_1+1
    .loc "test_sac_perf_sac.c", 34
    lda _sum_via_calls__local_3
    ldx _sum_via_calls__local_3+1
    sta $2C
    stx $2D
    .loc "test_sac_perf_sac.c", 10
    lda _sum_via_calls__local_3
    ldx _sum_via_calls__local_3+1
    add.16 .AX, #1
    sta $2E
    stx $2F
    lda $2E
    ldx $2F
    sta $30
    stx $31
@inline_end9:
    .loc "test_sac_perf_sac.c", 34
    lda $30
    ldx $31
    sta _sum_via_calls__local_3
    stx _sum_via_calls__local_3+1
    bra @while_cond4
@while_end6:
    .loc "test_sac_perf_sac.c", 36
    lda _sum_via_calls__local_1
    ldx _sum_via_calls__local_1+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _five_add
; SAC inline storage: 10 bytes
    .global _five_add__param_a
    _five_add__param_a: .word 0
    .global _five_add__param_b
    _five_add__param_b: .word 0
    .global _five_add__param_c
    _five_add__param_c: .word 0
    .global _five_add__param_d
    _five_add__param_d: .word 0
    .global _five_add__param_e
    _five_add__param_e: .word 0
    _five_add__local_0: .word 0
    _five_add__local_1: .word 0
    _five_add__local_2: .word 0
    _five_add__local_3: .word 0
    _five_add__local_4: .word 0
    proc _five_add, W#@_p_a, W#@_p_b, W#@_p_c, W#@_p_d, W#@_p_e
    .sac
    .var _fp = 0
    .loc "test_sac_perf_sac.c", 40
    .var @_p_a = 2
    .var @_p_b = 4
    .var @_p_c = 6
    .var @_p_d = 8
    .var @_p_e = 10
; .debug_var: __five_add @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __five_add @_p_b offset=4 size=2 type=int16 scope=parameter
; .debug_var: __five_add @_p_c offset=6 size=2 type=int16 scope=parameter
; .debug_var: __five_add @_p_d offset=8 size=2 type=int16 scope=parameter
; .debug_var: __five_add @_p_e offset=10 size=2 type=int16 scope=parameter

@entry:
    .loc "test_sac_perf_sac.c", 41
    lda _five_add__param_b
    ldx _five_add__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _five_add__param_a
    ldx _five_add__param_a+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda _five_add__param_c
    ldx _five_add__param_c+1
    add.16 .AX, $20
    sta $22
    stx $23
    lda _five_add__param_d
    ldx _five_add__param_d+1
    add.16 .AX, $22
    sta $20
    stx $21
    lda _five_add__param_e
    ldx _five_add__param_e+1
    add.16 .AX, $20
    sta $22
    stx $23
    lda $22
    ldx $23
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 10
    endproc

; function _main
; SAC inline storage: 24 bytes
    _main__local_0: .word 0
    _main__local_8: .word 0
    _main__local_16: .word 0
    _main__local_18: .word 0
    _main__local_20: .word 0
    _main__local_23: .word 0
    _main__local_33: .word 0
    _main__local_39: .word 0
    _main__local_47: .word 0
    _main__local_51: .word 0
    _main__local_53: .word 0
    _main__local_68: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_sac_perf_sac.c", 44
    .local @_l_i = 20
    .local @_l_r1 = 0
    .local @_l_r2 = 2
    .local @_l_r3 = 10
    .local @_l_r4 = 16
    .local @_l_r5 = 22
    .local @_l_result = 8
    .local @_l_sum1 = 12
    .local @_l_sum2 = 14
    .local @_l_temp1 = 4
    .local @_l_temp2 = 6
    .local @_l_total = 18
; .debug_var: __main @_l_i offset=20 size=2 type=int16 scope=local
; .debug_var: __main @_l_r1 offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_r2 offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_r3 offset=10 size=2 type=int16 scope=local
; .debug_var: __main @_l_r4 offset=16 size=2 type=int16 scope=local
; .debug_var: __main @_l_r5 offset=22 size=2 type=int16 scope=local
; .debug_var: __main @_l_result offset=8 size=2 type=int16 scope=local
; .debug_var: __main @_l_sum1 offset=12 size=2 type=int16 scope=local
; .debug_var: __main @_l_sum2 offset=14 size=2 type=int16 scope=local
; .debug_var: __main @_l_temp1 offset=4 size=2 type=int16 scope=local
; .debug_var: __main @_l_temp2 offset=6 size=2 type=int16 scope=local
; .debug_var: __main @_l_total offset=18 size=2 type=int16 scope=local

@entry:
    .loc "test_sac_perf_sac.c", 46
    .loc "test_sac_perf_sac.c", 10
@inline_end11:
    .loc "test_sac_perf_sac.c", 46
    .loc "test_sac_perf_sac.c", 47
    .loc "test_sac_perf_sac.c", 15
    .loc "test_sac_perf_sac.c", 16
    .loc "test_sac_perf_sac.c", 17
    .loc "test_sac_perf_sac.c", 18
@inline_end13:
    .loc "test_sac_perf_sac.c", 47
    .loc "test_sac_perf_sac.c", 48
    .loc "test_sac_perf_sac.c", 23
    .loc "test_sac_perf_sac.c", 10
@inline_end16:
    .loc "test_sac_perf_sac.c", 23
    .loc "test_sac_perf_sac.c", 24
    .loc "test_sac_perf_sac.c", 10
@inline_end18:
    .loc "test_sac_perf_sac.c", 24
    .loc "test_sac_perf_sac.c", 25
@inline_end15:
    .loc "test_sac_perf_sac.c", 48
    .loc "test_sac_perf_sac.c", 49
    lda #5
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_sac_perf_sac.c", 30
    lda #0
    sta _main__local_51
    sta _main__local_51+1
    .loc "test_sac_perf_sac.c", 31
    lda #0
    sta _main__local_53
    sta _main__local_53+1
@while_cond22_ph:
    .loc "test_sac_perf_sac.c", 34
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $24
    stx $25
@while_cond22:
    .loc "test_sac_perf_sac.c", 32
    lda _main__local_53
    ldx _main__local_53+1
    cmp.16 .AX, $22
    bcc @while_body23
    bra @while_end24
@while_body23:
    .loc "test_sac_perf_sac.c", 33
    lda _main__local_51
    ldx _main__local_51+1
    sta $24
    stx $25
    lda _main__local_53
    ldx _main__local_53+1
    sta $26
    stx $27
    .loc "test_sac_perf_sac.c", 10
    lda _main__local_53
    ldx _main__local_53+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_51
    ldx _main__local_51+1
    add.16 .AX, __zp_scratch2
    sta $28
    stx $29
    lda $28
    ldx $29
    sta $2A
    stx $2B
    lda $2A
    ldx $2B
    sta $2C
    stx $2D
@inline_end25:
    .loc "test_sac_perf_sac.c", 33
    lda $2C
    ldx $2D
    sta _main__local_51
    stx _main__local_51+1
    .loc "test_sac_perf_sac.c", 34
    lda _main__local_53
    ldx _main__local_53+1
    sta $30
    stx $31
    .loc "test_sac_perf_sac.c", 10
    lda _main__local_53
    ldx _main__local_53+1
    add.16 .AX, #1
    sta $32
    stx $33
    lda $32
    ldx $33
    sta $34
    stx $35
    lda $34
    ldx $35
    sta $36
    stx $37
@inline_end27:
    .loc "test_sac_perf_sac.c", 34
    lda $36
    ldx $37
    sta _main__local_53
    stx _main__local_53+1
    bra @while_cond22
@while_end24:
    .loc "test_sac_perf_sac.c", 36
@inline_end21:
    .loc "test_sac_perf_sac.c", 49
    .loc "test_sac_perf_sac.c", 50
    .loc "test_sac_perf_sac.c", 41
@inline_end30:
    .loc "test_sac_perf_sac.c", 50
    .loc "test_sac_perf_sac.c", 53
    brk
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 24
    endproc


__zp_save_buf:

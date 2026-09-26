; [Phase 87: Peephole Optimizer Applied]
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

    .extern ___idiom_sum16

    .global _small_file_test_1
    .global _small_file_test_2
    .global _medium_complexity
    .global _medium_optimization_candidate
    .global _large_inline_candidate
    .global _large_optimizable
    .global _large_loop_candidate
    .global _decision_test_cse
    .global _decision_test_constant_propagation
    .global _decision_test_dead_code
    .global _test_inlining_calls
    .global _high_register_pressure
    .global _high_branch_density
    .global _array_access_test
    .global _pointer_arithmetic_test
    .global _mixed_optimization
    .global _main

    .segment "code"

; function _small_file_test_1
; SAC zero-alloc leaf: no storage overhead
    proc _small_file_test_1
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdio.h", 9

@entry:
    .loc "src/test-resources/test_phase108_validation.c", 13
    lda #42
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, zeroalloc, leaf
    .reg_clobbers A, X
    .flag_clobbers N, Z
    .frame_size 0
    endproc

; function _small_file_test_2
; SAC inline storage: 2 bytes
    _small_file_test_2__local_0: .word 0
    proc _small_file_test_2
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdio.h", 13
    .local @_l_x = 0
; .debug_var: __small_file_test_2 @_l_x offset=0 size=2 type=int16 scope=local

@entry:
    .loc "src/test-resources/test_phase108_validation.c", 17
    lda #10
    sta _small_file_test_2__local_0
    lda #0
    sta _small_file_test_2__local_0+1
    .loc "src/test-resources/test_phase108_validation.c", 18
    lda _small_file_test_2__local_0
    ldx _small_file_test_2__local_0+1
    add.16 .AX, #5
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _medium_complexity
; SAC inline storage: 6 bytes
    .global _medium_complexity__param_n
    _medium_complexity__param_n: .word 0
    _medium_complexity__local_0: .word 0
    _medium_complexity__local_1: .word 0
    _medium_complexity__local_3: .word 0
    proc _medium_complexity, W#@_p_n
    .sac
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/string.h", 2
    .local @_l_i = 4
    .local @_l_sum = 2
; .debug_var: __medium_complexity @_l_i offset=4 size=2 type=int16 scope=local
; .debug_var: __medium_complexity @_l_sum offset=2 size=2 type=int16 scope=local
    .var @_p_n = 2
; .debug_var: __medium_complexity @_p_n offset=2 size=2 type=int16 scope=parameter

@entry:
    .loc "src/test-resources/test_phase108_validation.c", 27
    lda #0
    sta _medium_complexity__local_1
    sta _medium_complexity__local_1+1
    .loc "src/test-resources/test_phase108_validation.c", 28
    lda #0
    sta _medium_complexity__local_3
    sta _medium_complexity__local_3+1
@for_cond0:
    lda _medium_complexity__param_n
    ldx _medium_complexity__param_n+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _medium_complexity__local_3
    ldx _medium_complexity__local_3+1
    cmp.16 .AX, __zp_scratch2
    bcc @for_body1
    bra @for_end3
@for_body1:
    .loc "src/test-resources/test_phase108_validation.c", 29
    lda #2
    ldx #0
    sta $22
    stx $23
    lda _medium_complexity__local_3
    ldx _medium_complexity__local_3+1
    lsl.16 .AX
    sta $24
    stx $25
    lda _medium_complexity__local_1
    ldx _medium_complexity__local_1+1
    add.16 .AX, $24
    sta $26
    stx $27
    sta _medium_complexity__local_1
    stx _medium_complexity__local_1+1
@for_inc2:
    .loc "src/test-resources/test_phase108_validation.c", 28
    lda _medium_complexity__local_3
    ldx _medium_complexity__local_3+1
    sta $28
    stx $29
    ; [peephole-opt]     lda $28
    ; [peephole-opt]     ldx $29
    add.16 .AX, #1
    sta _medium_complexity__local_3
    stx _medium_complexity__local_3+1
    bra @for_cond0
@for_end3:
    .loc "src/test-resources/test_phase108_validation.c", 31
    lda _medium_complexity__local_1
    ldx _medium_complexity__local_1+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .param_sizes 2
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _medium_optimization_candidate
; SAC inline storage: 10 bytes
    .global _medium_optimization_candidate__param_x
    _medium_optimization_candidate__param_x: .word 0
    .global _medium_optimization_candidate__param_y
    _medium_optimization_candidate__param_y: .word 0
    _medium_optimization_candidate__local_0: .word 0
    _medium_optimization_candidate__local_1: .word 0
    _medium_optimization_candidate__local_2: .word 0
    _medium_optimization_candidate__local_5: .word 0
    _medium_optimization_candidate__local_8: .word 0
    proc _medium_optimization_candidate, W#@_p_x, W#@_p_y
    .sac
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/string.h", 10
    .local @_l_a = 4
    .local @_l_b = 6
    .local @_l_c = 8
; .debug_var: __medium_optimization_candidate @_l_a offset=4 size=2 type=int16 scope=local
; .debug_var: __medium_optimization_candidate @_l_b offset=6 size=2 type=int16 scope=local
; .debug_var: __medium_optimization_candidate @_l_c offset=8 size=2 type=int16 scope=local
    .var @_p_x = 2
    .var @_p_y = 4
; .debug_var: __medium_optimization_candidate @_p_x offset=2 size=2 type=int16 scope=parameter
; .debug_var: __medium_optimization_candidate @_p_y offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "src/test-resources/test_phase108_validation.c", 35
    lda _medium_optimization_candidate__param_x
    ldx _medium_optimization_candidate__param_x+1
    add.16 .AX, #1
    sta _medium_optimization_candidate__local_2
    stx _medium_optimization_candidate__local_2+1
    .loc "src/test-resources/test_phase108_validation.c", 36
    lda _medium_optimization_candidate__param_x
    ldx _medium_optimization_candidate__param_x+1
    add.16 .AX, #1
    sta _medium_optimization_candidate__local_5
    stx _medium_optimization_candidate__local_5+1
    .loc "src/test-resources/test_phase108_validation.c", 37
    lda _medium_optimization_candidate__param_x
    ldx _medium_optimization_candidate__param_x+1
    add.16 .AX, #1
    sta _medium_optimization_candidate__local_8
    stx _medium_optimization_candidate__local_8+1
    .loc "src/test-resources/test_phase108_validation.c", 38
    lda _medium_optimization_candidate__local_5
    ; [peephole-opt]     ldx _medium_optimization_candidate__local_5+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _medium_optimization_candidate__local_2
    ; [peephole-opt]     ldx _medium_optimization_candidate__local_2+1
    add.16 .AX, __zp_scratch2
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _medium_optimization_candidate__local_8
    ; [peephole-opt]     ldx _medium_optimization_candidate__local_8+1
    add.16 .AX, __zp_scratch2
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .param_sizes 2, 2
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 10
    endproc

; function _large_inline_candidate
; SAC inline storage: 2 bytes
    .global _large_inline_candidate__param_x
    _large_inline_candidate__param_x: .word 0
    _large_inline_candidate__local_0: .word 0
    proc _large_inline_candidate, W#@_p_x
    .sac
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/string.h", 22
    .var @_p_x = 2
; .debug_var: __large_inline_candidate @_p_x offset=2 size=2 type=int16 scope=parameter

@entry:
    .loc "src/test-resources/test_phase108_validation.c", 47
    lda #2
    ldx #0
    lda _large_inline_candidate__param_x
    ldx _large_inline_candidate__param_x+1
    lsl.16 .AX
    add.16 .AX, #1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .param_sizes 2
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _large_optimizable
; SAC inline storage: 16 bytes
    .global _large_optimizable__param_a
    _large_optimizable__param_a: .word 0
    .global _large_optimizable__param_b
    _large_optimizable__param_b: .word 0
    .global _large_optimizable__param_c
    _large_optimizable__param_c: .word 0
    _large_optimizable__local_0: .word 0
    _large_optimizable__local_1: .word 0
    _large_optimizable__local_2: .word 0
    _large_optimizable__local_3: .word 0
    _large_optimizable__local_5: .word 0
    _large_optimizable__local_7: .word 0
    _large_optimizable__local_9: .word 0
    _large_optimizable__local_11: .word 0
    proc _large_optimizable, W#@_p_a, W#@_p_b, W#@_p_c
    .sac
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/string.h", 26
    .local @_l_r1 = 12
    .local @_l_r2 = 14
    .local @_l_t1 = 6
    .local @_l_t2 = 8
    .local @_l_t3 = 10
; .debug_var: __large_optimizable @_l_r1 offset=12 size=2 type=int16 scope=local
; .debug_var: __large_optimizable @_l_r2 offset=14 size=2 type=int16 scope=local
; .debug_var: __large_optimizable @_l_t1 offset=6 size=2 type=int16 scope=local
; .debug_var: __large_optimizable @_l_t2 offset=8 size=2 type=int16 scope=local
; .debug_var: __large_optimizable @_l_t3 offset=10 size=2 type=int16 scope=local
    .var @_p_a = 2
    .var @_p_b = 4
    .var @_p_c = 6
; .debug_var: __large_optimizable @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __large_optimizable @_p_b offset=4 size=2 type=int16 scope=parameter
; .debug_var: __large_optimizable @_p_c offset=6 size=2 type=int16 scope=parameter

@entry:
    .loc "src/test-resources/test_phase108_validation.c", 51
    lda _large_optimizable__param_b
    ldx _large_optimizable__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _large_optimizable__param_a
    ldx _large_optimizable__param_a+1
    add.16 .AX, __zp_scratch2
    sta _large_optimizable__local_3
    stx _large_optimizable__local_3+1
    .loc "src/test-resources/test_phase108_validation.c", 52
    lda _large_optimizable__param_c
    ldx _large_optimizable__param_c+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _large_optimizable__param_b
    ldx _large_optimizable__param_b+1
    add.16 .AX, __zp_scratch2
    sta _large_optimizable__local_5
    stx _large_optimizable__local_5+1
    .loc "src/test-resources/test_phase108_validation.c", 53
    lda _large_optimizable__param_a
    ldx _large_optimizable__param_a+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _large_optimizable__param_c
    ldx _large_optimizable__param_c+1
    add.16 .AX, __zp_scratch2
    sta _large_optimizable__local_7
    stx _large_optimizable__local_7+1
    .loc "src/test-resources/test_phase108_validation.c", 54
    lda _large_optimizable__local_5
    ; [peephole-opt]     ldx _large_optimizable__local_5+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _large_optimizable__local_3
    ; [peephole-opt]     ldx _large_optimizable__local_3+1
    add.16 .AX, __zp_scratch2
    sta _large_optimizable__local_9
    stx _large_optimizable__local_9+1
    .loc "src/test-resources/test_phase108_validation.c", 55
    lda _large_optimizable__local_7
    ; [peephole-opt]     ldx _large_optimizable__local_7+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _large_optimizable__local_5
    ; [peephole-opt]     ldx _large_optimizable__local_5+1
    add.16 .AX, __zp_scratch2
    sta _large_optimizable__local_11
    stx _large_optimizable__local_11+1
    .loc "src/test-resources/test_phase108_validation.c", 56
    lda _large_optimizable__local_11
    ; [peephole-opt]     ldx _large_optimizable__local_11+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _large_optimizable__local_9
    ; [peephole-opt]     ldx _large_optimizable__local_9+1
    add.16 .AX, __zp_scratch2
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .param_sizes 2, 2, 2
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 16
    endproc

; function _large_loop_candidate
; SAC inline storage: 6 bytes
    .global _large_loop_candidate__param_n
    _large_loop_candidate__param_n: .word 0
    _large_loop_candidate__local_0: .word 0
    _large_loop_candidate__local_1: .word 0
    _large_loop_candidate__local_3: .word 0
    proc _large_loop_candidate, W#@_p_n
    .sac
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/string.h", 35
    .local @_l_i = 4
    .local @_l_result = 2
; .debug_var: __large_loop_candidate @_l_i offset=4 size=2 type=int16 scope=local
; .debug_var: __large_loop_candidate @_l_result offset=2 size=2 type=int16 scope=local
    .var @_p_n = 2
; .debug_var: __large_loop_candidate @_p_n offset=2 size=2 type=int16 scope=parameter

@entry:
    .loc "src/test-resources/test_phase108_validation.c", 60
    lda #0
    sta _large_loop_candidate__local_1
    sta _large_loop_candidate__local_1+1
    .loc "src/test-resources/test_phase108_validation.c", 61
    lda #0
    sta _large_loop_candidate__local_3
    sta _large_loop_candidate__local_3+1
@for_cond4:
    lda _large_loop_candidate__param_n
    ldx _large_loop_candidate__param_n+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _large_loop_candidate__local_3
    ldx _large_loop_candidate__local_3+1
    cmp.16 .AX, __zp_scratch2
    bcc @for_body5
    bra @for_end7
@for_body5:
    .loc "src/test-resources/test_phase108_validation.c", 62
    lda _large_loop_candidate__local_3
    ldx _large_loop_candidate__local_3+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _large_loop_candidate__local_1
    ldx _large_loop_candidate__local_1+1
    add.16 .AX, __zp_scratch2
    sta $22
    stx $23
    sta _large_loop_candidate__local_1
    stx _large_loop_candidate__local_1+1
    .loc "src/test-resources/test_phase108_validation.c", 63
    lda #2
    ldx #0
    sta $24
    stx $25
    lda _large_loop_candidate__local_1
    ; [peephole-opt]     ldx _large_loop_candidate__local_1+1
    lsl.16 .AX
    sta $26
    stx $27
    ; [peephole-opt]     lda $26
    ; [peephole-opt]     ldx $27
    sta _large_loop_candidate__local_1
    stx _large_loop_candidate__local_1+1
    .loc "src/test-resources/test_phase108_validation.c", 64
    dec.16f __vr1
@for_inc6:
    .loc "src/test-resources/test_phase108_validation.c", 61
    lda _large_loop_candidate__local_3
    ldx _large_loop_candidate__local_3+1
    sta $2C
    stx $2D
    ; [peephole-opt]     lda $2C
    ; [peephole-opt]     ldx $2D
    add.16 .AX, #1
    sta _large_loop_candidate__local_3
    stx _large_loop_candidate__local_3+1
    bra @for_cond4
@for_end7:
    .loc "src/test-resources/test_phase108_validation.c", 66
    lda _large_loop_candidate__local_1
    ldx _large_loop_candidate__local_1+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .param_sizes 2
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _decision_test_cse
; SAC inline storage: 6 bytes
    _decision_test_cse__local_0: .word 0
    _decision_test_cse__local_2: .word 0
    _decision_test_cse__local_5: .word 0
    proc _decision_test_cse
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "src/test-resources/test_phase108_validation.c", 14
    .local @_l_x = 0
    .local @_l_y = 2
    .local @_l_z = 4
; .debug_var: __decision_test_cse @_l_x offset=0 size=2 type=int16 scope=local
; .debug_var: __decision_test_cse @_l_y offset=2 size=2 type=int16 scope=local
; .debug_var: __decision_test_cse @_l_z offset=4 size=2 type=int16 scope=local

@entry:
    .loc "src/test-resources/test_phase108_validation.c", 76
    lda #100
    sta _decision_test_cse__local_0
    lda #0
    sta _decision_test_cse__local_0+1
    .loc "src/test-resources/test_phase108_validation.c", 77
    lda _decision_test_cse__local_0
    ldx _decision_test_cse__local_0+1
    add.16 .AX, #50
    sta _decision_test_cse__local_2
    stx _decision_test_cse__local_2+1
    .loc "src/test-resources/test_phase108_validation.c", 78
    lda _decision_test_cse__local_0
    ldx _decision_test_cse__local_0+1
    add.16 .AX, #50
    sta _decision_test_cse__local_5
    stx _decision_test_cse__local_5+1
    .loc "src/test-resources/test_phase108_validation.c", 79
    lda _decision_test_cse__local_5
    ; [peephole-opt]     ldx _decision_test_cse__local_5+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _decision_test_cse__local_2
    ; [peephole-opt]     ldx _decision_test_cse__local_2+1
    add.16 .AX, __zp_scratch2
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _decision_test_constant_propagation
; SAC inline storage: 8 bytes
    _decision_test_constant_propagation__local_0: .word 0
    _decision_test_constant_propagation__local_2: .word 0
    _decision_test_constant_propagation__local_3: .word 0
    _decision_test_constant_propagation__local_4: .word 0
    proc _decision_test_constant_propagation
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "src/test-resources/test_phase108_validation.c", 22
    .local @_l_a = 0
    .local @_l_b = 2
    .local @_l_c = 4
    .local @_l_d = 6
; .debug_var: __decision_test_constant_propagation @_l_a offset=0 size=2 type=int16 scope=local
; .debug_var: __decision_test_constant_propagation @_l_b offset=2 size=2 type=int16 scope=local
; .debug_var: __decision_test_constant_propagation @_l_c offset=4 size=2 type=int16 scope=local
; .debug_var: __decision_test_constant_propagation @_l_d offset=6 size=2 type=int16 scope=local

@entry:
    .loc "src/test-resources/test_phase108_validation.c", 84
    lda #42
    sta _decision_test_constant_propagation__local_0
    lda #0
    sta _decision_test_constant_propagation__local_0+1
    .loc "src/test-resources/test_phase108_validation.c", 85
    lda _decision_test_constant_propagation__local_0
    ldx _decision_test_constant_propagation__local_0+1
    sta _decision_test_constant_propagation__local_2
    stx _decision_test_constant_propagation__local_2+1
    .loc "src/test-resources/test_phase108_validation.c", 86
    lda _decision_test_constant_propagation__local_2
    ; [peephole-opt]     ldx _decision_test_constant_propagation__local_2+1
    sta _decision_test_constant_propagation__local_3
    stx _decision_test_constant_propagation__local_3+1
    .loc "src/test-resources/test_phase108_validation.c", 87
    lda _decision_test_constant_propagation__local_3
    ; [peephole-opt]     ldx _decision_test_constant_propagation__local_3+1
    sta _decision_test_constant_propagation__local_4
    stx _decision_test_constant_propagation__local_4+1
    .loc "src/test-resources/test_phase108_validation.c", 88
    lda _decision_test_constant_propagation__local_4
    ; [peephole-opt]     ldx _decision_test_constant_propagation__local_4+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers N, Z
    .frame_size 8
    endproc

; function _decision_test_dead_code
; SAC inline storage: 4 bytes
    _decision_test_dead_code__local_0: .word 0
    _decision_test_dead_code__local_1: .word 0
    _decision_test_dead_code__local_2: .word 0
    proc _decision_test_dead_code
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "src/test-resources/test_phase108_validation.c", 31
    .local @_l_unused = 0
    .local @_l_used = 2
; .debug_var: __decision_test_dead_code @_l_unused offset=0 size=2 type=int16 scope=local
; .debug_var: __decision_test_dead_code @_l_used offset=2 size=2 type=int16 scope=local

@entry:
    .loc "src/test-resources/test_phase108_validation.c", 94
    lda #123
    sta _decision_test_dead_code__local_2
    lda #0
    sta _decision_test_dead_code__local_2+1
    .loc "src/test-resources/test_phase108_validation.c", 95
    lda _decision_test_dead_code__local_2
    ldx _decision_test_dead_code__local_2+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers N, Z
    .frame_size 4
    endproc

; function _test_inlining_calls
; SAC inline storage: 4 bytes
    _test_inlining_calls__local_0: .word 0
    _test_inlining_calls__local_2: .word 0
    proc _test_inlining_calls
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "src/test-resources/test_phase108_validation.c", 51
    .local @_l_i = 2
    .local @_l_total = 0
; .debug_var: __test_inlining_calls @_l_i offset=2 size=2 type=int16 scope=local
; .debug_var: __test_inlining_calls @_l_total offset=0 size=2 type=int16 scope=local

@entry:
    .loc "src/test-resources/test_phase108_validation.c", 112
    lda #0
    sta _test_inlining_calls__local_0
    sta _test_inlining_calls__local_0+1
    .loc "src/test-resources/test_phase108_validation.c", 113
    lda #0
    sta _test_inlining_calls__local_2
    sta _test_inlining_calls__local_2+1
@for_cond8:
    lda _test_inlining_calls__local_2
    ldx _test_inlining_calls__local_2+1
    cmp.16 .AX, #10
    bcc @for_body9
    bra @for_end11
@for_body9:
    .loc "src/test-resources/test_phase108_validation.c", 114
    lda _test_inlining_calls__local_2
    ldx _test_inlining_calls__local_2+1
    sta $24
    stx $25
    .loc "src/test-resources/test_phase108_validation.c", 104
    lda _test_inlining_calls__local_2
    ldx _test_inlining_calls__local_2+1
    add.16 .AX, #1
    sta $28
    stx $29
    ; [peephole-opt]     lda $28
    ; [peephole-opt]     ldx $29
    sta $2A
    stx $2B
@inline_end12:
    .loc "src/test-resources/test_phase108_validation.c", 114
    lda _test_inlining_calls__local_0
    ldx _test_inlining_calls__local_0+1
    add.16 .AX, $2A
    sta $2C
    stx $2D
    sta _test_inlining_calls__local_0
    stx _test_inlining_calls__local_0+1
    .loc "src/test-resources/test_phase108_validation.c", 115
    lda _test_inlining_calls__local_2
    ldx _test_inlining_calls__local_2+1
    add.16 .AX, #1
    sta $30
    stx $31
    lda _test_inlining_calls__local_2
    ldx _test_inlining_calls__local_2+1
    sta $32
    stx $33
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    sta $34
    stx $35
    .loc "src/test-resources/test_phase108_validation.c", 108
    lda _test_inlining_calls__local_2
    ldx _test_inlining_calls__local_2+1
    add.16 .AX, $30
    sta $36
    stx $37
    ; [peephole-opt]     lda $36
    clc
    adc #1
    sta $3A
    lda $37
    adc #0
    sta $3B
    ; [peephole-opt]     lda $3A
    ldx $3B
    sta $3C
    stx $3D
@inline_end14:
    .loc "src/test-resources/test_phase108_validation.c", 115
    lda _test_inlining_calls__local_0
    ldx _test_inlining_calls__local_0+1
    add.16 .AX, $3C
    sta $3E
    stx $3F
    sta _test_inlining_calls__local_0
    stx _test_inlining_calls__local_0+1
@for_inc10:
    .loc "src/test-resources/test_phase108_validation.c", 113
    lda _test_inlining_calls__local_2
    ldx _test_inlining_calls__local_2+1
    sta $40
    stx $41
    ; [peephole-opt]     lda $40
    ; [peephole-opt]     ldx $41
    add.16 .AX, #1
    sta _test_inlining_calls__local_2
    stx _test_inlining_calls__local_2+1
    bra @for_cond8
@for_end11:
    .loc "src/test-resources/test_phase108_validation.c", 117
    lda _test_inlining_calls__local_0
    ldx _test_inlining_calls__local_0+1
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _high_register_pressure
; SAC inline storage: 24 bytes
    .global _high_register_pressure__param_a
    _high_register_pressure__param_a: .word 0
    .global _high_register_pressure__param_b
    _high_register_pressure__param_b: .word 0
    .global _high_register_pressure__param_c
    _high_register_pressure__param_c: .word 0
    .global _high_register_pressure__param_d
    _high_register_pressure__param_d: .word 0
    .global _high_register_pressure__param_e
    _high_register_pressure__param_e: .word 0
    .global _high_register_pressure__param_f
    _high_register_pressure__param_f: .word 0
    _high_register_pressure__local_0: .word 0
    _high_register_pressure__local_1: .word 0
    _high_register_pressure__local_2: .word 0
    _high_register_pressure__local_3: .word 0
    _high_register_pressure__local_4: .word 0
    _high_register_pressure__local_5: .word 0
    _high_register_pressure__local_6: .word 0
    _high_register_pressure__local_8: .word 0
    _high_register_pressure__local_10: .word 0
    _high_register_pressure__local_12: .word 0
    _high_register_pressure__local_14: .word 0
    _high_register_pressure__local_16: .word 0
    proc _high_register_pressure, W#@_p_a, W#@_p_b, W#@_p_c, W#@_p_d, W#@_p_e, W#@_p_f
    .sac
    .var _fp = 0
    .loc "src/test-resources/test_phase108_validation.c", 65
    .local @_l_r1 = 12
    .local @_l_r2 = 14
    .local @_l_r3 = 16
    .local @_l_r4 = 18
    .local @_l_r5 = 20
    .local @_l_r6 = 22
; .debug_var: __high_register_pressure @_l_r1 offset=12 size=2 type=int16 scope=local
; .debug_var: __high_register_pressure @_l_r2 offset=14 size=2 type=int16 scope=local
; .debug_var: __high_register_pressure @_l_r3 offset=16 size=2 type=int16 scope=local
; .debug_var: __high_register_pressure @_l_r4 offset=18 size=2 type=int16 scope=local
; .debug_var: __high_register_pressure @_l_r5 offset=20 size=2 type=int16 scope=local
; .debug_var: __high_register_pressure @_l_r6 offset=22 size=2 type=int16 scope=local
    .var @_p_a = 2
    .var @_p_b = 4
    .var @_p_c = 6
    .var @_p_d = 8
    .var @_p_e = 10
    .var @_p_f = 12
; .debug_var: __high_register_pressure @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __high_register_pressure @_p_b offset=4 size=2 type=int16 scope=parameter
; .debug_var: __high_register_pressure @_p_c offset=6 size=2 type=int16 scope=parameter
; .debug_var: __high_register_pressure @_p_d offset=8 size=2 type=int16 scope=parameter
; .debug_var: __high_register_pressure @_p_e offset=10 size=2 type=int16 scope=parameter
; .debug_var: __high_register_pressure @_p_f offset=12 size=2 type=int16 scope=parameter

@entry:
    .loc "src/test-resources/test_phase108_validation.c", 126
    lda _high_register_pressure__param_b
    ldx _high_register_pressure__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _high_register_pressure__param_a
    ldx _high_register_pressure__param_a+1
    add.16 .AX, __zp_scratch2
    sta _high_register_pressure__local_6
    stx _high_register_pressure__local_6+1
    .loc "src/test-resources/test_phase108_validation.c", 127
    lda _high_register_pressure__param_d
    ldx _high_register_pressure__param_d+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _high_register_pressure__param_c
    ldx _high_register_pressure__param_c+1
    add.16 .AX, __zp_scratch2
    sta _high_register_pressure__local_8
    stx _high_register_pressure__local_8+1
    .loc "src/test-resources/test_phase108_validation.c", 128
    lda _high_register_pressure__param_f
    ldx _high_register_pressure__param_f+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _high_register_pressure__param_e
    ldx _high_register_pressure__param_e+1
    add.16 .AX, __zp_scratch2
    sta _high_register_pressure__local_10
    stx _high_register_pressure__local_10+1
    .loc "src/test-resources/test_phase108_validation.c", 129
    lda _high_register_pressure__local_8
    ; [peephole-opt]     ldx _high_register_pressure__local_8+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _high_register_pressure__local_6
    ; [peephole-opt]     ldx _high_register_pressure__local_6+1
    add.16 .AX, __zp_scratch2
    sta _high_register_pressure__local_12
    stx _high_register_pressure__local_12+1
    .loc "src/test-resources/test_phase108_validation.c", 130
    lda _high_register_pressure__local_10
    ; [peephole-opt]     ldx _high_register_pressure__local_10+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _high_register_pressure__local_8
    ; [peephole-opt]     ldx _high_register_pressure__local_8+1
    add.16 .AX, __zp_scratch2
    sta _high_register_pressure__local_14
    stx _high_register_pressure__local_14+1
    .loc "src/test-resources/test_phase108_validation.c", 131
    lda _high_register_pressure__local_6
    ldx _high_register_pressure__local_6+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _high_register_pressure__local_10
    ; [peephole-opt]     ldx _high_register_pressure__local_10+1
    add.16 .AX, __zp_scratch2
    sta _high_register_pressure__local_16
    stx _high_register_pressure__local_16+1
    .loc "src/test-resources/test_phase108_validation.c", 132
    lda _high_register_pressure__local_14
    ; [peephole-opt]     ldx _high_register_pressure__local_14+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _high_register_pressure__local_12
    ; [peephole-opt]     ldx _high_register_pressure__local_12+1
    add.16 .AX, __zp_scratch2
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _high_register_pressure__local_16
    ; [peephole-opt]     ldx _high_register_pressure__local_16+1
    add.16 .AX, __zp_scratch2
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .param_sizes 2, 2, 2, 2, 2, 2
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 24
    endproc

; function _high_branch_density
; SAC inline storage: 6 bytes
    .global _high_branch_density__param_x
    _high_branch_density__param_x: .word 0
    .global _high_branch_density__param_y
    _high_branch_density__param_y: .word 0
    .global _high_branch_density__param_z
    _high_branch_density__param_z: .word 0
    _high_branch_density__local_0: .word 0
    _high_branch_density__local_1: .word 0
    _high_branch_density__local_2: .word 0
    proc _high_branch_density, W#@_p_x, W#@_p_y, W#@_p_z
    .sac
    .var _fp = 0
    .loc "src/test-resources/test_phase108_validation.c", 80
    .var @_p_x = 2
    .var @_p_y = 4
    .var @_p_z = 6
; .debug_var: __high_branch_density @_p_x offset=2 size=2 type=int16 scope=parameter
; .debug_var: __high_branch_density @_p_y offset=4 size=2 type=int16 scope=parameter
; .debug_var: __high_branch_density @_p_z offset=6 size=2 type=int16 scope=parameter

@entry:
    .loc "src/test-resources/test_phase108_validation.c", 141
    lda _high_branch_density__param_x
    ldx _high_branch_density__param_x+1
    cmp.16 .AX, #0
    beq @if_else17
    bcs @if_then16
    bra @if_else17
@if_then16:
    .loc "src/test-resources/test_phase108_validation.c", 142
    lda _high_branch_density__param_y
    ldx _high_branch_density__param_y+1
    cmp.16 .AX, #0
    beq @if_else20
    bcs @if_then19
    bra @if_else20
@if_then19:
    .loc "src/test-resources/test_phase108_validation.c", 143
    lda _high_branch_density__param_z
    ldx _high_branch_density__param_z+1
    cmp.16 .AX, #0
    beq @if_else23
    bcs @if_then22
    bra @if_else23
@if_then22:
    .loc "src/test-resources/test_phase108_validation.c", 144
    lda _high_branch_density__param_y
    ldx _high_branch_density__param_y+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _high_branch_density__param_x
    ldx _high_branch_density__param_x+1
    add.16 .AX, __zp_scratch2
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _high_branch_density__param_z
    ldx _high_branch_density__param_z+1
    add.16 .AX, __zp_scratch2
    bra @__return
    bra @if_end18
@if_else23:
    .loc "src/test-resources/test_phase108_validation.c", 146
    lda _high_branch_density__param_y
    ldx _high_branch_density__param_y+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _high_branch_density__param_x
    ldx _high_branch_density__param_x+1
    add.16 .AX, __zp_scratch2
    bra @__return
    bra @if_end18
@if_else20:
    .loc "src/test-resources/test_phase108_validation.c", 149
    lda _high_branch_density__param_x
    ldx _high_branch_density__param_x+1
    bra @__return
    bra @if_end18
@if_else17:
    .loc "src/test-resources/test_phase108_validation.c", 152
    lda #0
    ldx #0
    bra @__return
@if_end18:
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .param_sizes 2, 2, 2
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _array_access_test
; SAC inline storage: 6 bytes
    .global _array_access_test__param_arr
    _array_access_test__param_arr: .word 0
    .global _array_access_test__param_n
    _array_access_test__param_n: .word 0
    _array_access_test__local_0: .word 0
    _array_access_test__local_1: .word 0
    _array_access_test__local_2: .word 0
    proc _array_access_test, W#@_p_arr, W#@_p_n
    .sac
    .var _fp = 0
    .loc "src/test-resources/test_phase108_validation.c", 101
    .local @_l_sum = 4
; .debug_var: __array_access_test @_l_sum offset=4 size=2 type=int16 scope=local
    .var @_p_arr = 2
    .var @_p_n = 4
; .debug_var: __array_access_test @_p_arr offset=2 size=2 type=ptr scope=parameter
; .debug_var: __array_access_test @_p_n offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "src/test-resources/test_phase108_validation.c", 162
    lda #0
    sta _array_access_test__local_2
    sta _array_access_test__local_2+1
    lda _array_access_test__param_n
    ldx _array_access_test__param_n+1
    sta $28
    stx $29
    lda _array_access_test__param_arr
    ldx _array_access_test__param_arr+1
    sta $2A
    stx $2B
    ; [peephole-opt]     lda $28
    ; [peephole-opt]     ldx $29
    push .ax
    ; [peephole-opt]     lda $2A
    ; [peephole-opt]     ldx $2B
    push .ax
    jsr ___idiom_sum16
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ; [peephole-opt]     ldx __zp_scratch4+1
    sta _array_access_test__local_2
    stx _array_access_test__local_2+1
    .loc "src/test-resources/test_phase108_validation.c", 166
    lda _array_access_test__local_2
    ; [peephole-opt]     ldx _array_access_test__local_2+1
@__return:
    rts
    .func_flags stack_call, static_alloc
    .param_sizes 2, 2
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _pointer_arithmetic_test
; SAC inline storage: 8 bytes
    .global _pointer_arithmetic_test__param_ptr
    _pointer_arithmetic_test__param_ptr: .word 0
    .global _pointer_arithmetic_test__param_len
    _pointer_arithmetic_test__param_len: .word 0
    _pointer_arithmetic_test__local_0: .word 0
    _pointer_arithmetic_test__local_1: .word 0
    _pointer_arithmetic_test__local_2: .word 0
    _pointer_arithmetic_test__local_4: .word 0
    proc _pointer_arithmetic_test, W#@_p_ptr, W#@_p_len
    .sac
    .var _fp = 0
    .loc "src/test-resources/test_phase108_validation.c", 114
    .local @_l_i = 6
    .local @_l_result = 4
; .debug_var: __pointer_arithmetic_test @_l_i offset=6 size=2 type=int16 scope=local
; .debug_var: __pointer_arithmetic_test @_l_result offset=4 size=2 type=int16 scope=local
    .var @_p_ptr = 2
    .var @_p_len = 4
; .debug_var: __pointer_arithmetic_test @_p_ptr offset=2 size=2 type=ptr scope=parameter
; .debug_var: __pointer_arithmetic_test @_p_len offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "src/test-resources/test_phase108_validation.c", 175
    lda #0
    sta _pointer_arithmetic_test__local_2
    sta _pointer_arithmetic_test__local_2+1
    .loc "src/test-resources/test_phase108_validation.c", 176
    lda #0
    sta _pointer_arithmetic_test__local_4
    sta _pointer_arithmetic_test__local_4+1
@for_cond25:
    lda _pointer_arithmetic_test__param_len
    ldx _pointer_arithmetic_test__param_len+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _pointer_arithmetic_test__local_4
    ldx _pointer_arithmetic_test__local_4+1
    cmp.16 .AX, __zp_scratch2
    bcc @for_body26
    bra @for_end28
@for_body26:
    .loc "src/test-resources/test_phase108_validation.c", 177
    lda _pointer_arithmetic_test__param_ptr
    ldx _pointer_arithmetic_test__param_ptr+1
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $22
    stx $23
    lda _pointer_arithmetic_test__local_2
    ldx _pointer_arithmetic_test__local_2+1
    add.16 .AX, $22
    sta $24
    stx $25
    sta _pointer_arithmetic_test__local_2
    stx _pointer_arithmetic_test__local_2+1
    .loc "src/test-resources/test_phase108_validation.c", 178
    lda _pointer_arithmetic_test__param_ptr
    ldx _pointer_arithmetic_test__param_ptr+1
    sta $26
    stx $27
    ; [peephole-opt]     lda $26
    ; [peephole-opt]     ldx $27
    add.16 .AX, #2
    sta $28
    stx $29
    sta _pointer_arithmetic_test__param_ptr
    stx _pointer_arithmetic_test__param_ptr+1
@for_inc27:
    .loc "src/test-resources/test_phase108_validation.c", 176
    lda _pointer_arithmetic_test__local_4
    ldx _pointer_arithmetic_test__local_4+1
    sta $2A
    stx $2B
    ; [peephole-opt]     lda $2A
    ; [peephole-opt]     ldx $2B
    add.16 .AX, #1
    sta _pointer_arithmetic_test__local_4
    stx _pointer_arithmetic_test__local_4+1
    bra @for_cond25
@for_end28:
    .loc "src/test-resources/test_phase108_validation.c", 180
    lda _pointer_arithmetic_test__local_2
    ldx _pointer_arithmetic_test__local_2+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .param_sizes 2, 2
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 8
    endproc

; function _mixed_optimization
; SAC inline storage: 14 bytes
    .global _mixed_optimization__param_points
    _mixed_optimization__param_points: .word 0
    .global _mixed_optimization__param_count
    _mixed_optimization__param_count: .word 0
    _mixed_optimization__local_0: .word 0
    _mixed_optimization__local_1: .word 0
    _mixed_optimization__local_2: .word 0
    _mixed_optimization__local_4: .word 0
    _mixed_optimization__local_7: .word 0
    _mixed_optimization__local_11: .word 0
    _mixed_optimization__local_15: .word 0
    proc _mixed_optimization, W#@_p_points, W#@_p_count
    .sac
    .var _fp = 0
    .loc "src/test-resources/test_phase108_validation.c", 133
    .local @_l_dist_sq = 12
    .local @_l_i = 6
    .local @_l_px = 8
    .local @_l_py = 10
    .local @_l_total = 4
; .debug_var: __mixed_optimization @_l_dist_sq offset=12 size=2 type=int16 scope=local
; .debug_var: __mixed_optimization @_l_i offset=6 size=2 type=int16 scope=local
; .debug_var: __mixed_optimization @_l_px offset=8 size=2 type=int16 scope=local
; .debug_var: __mixed_optimization @_l_py offset=10 size=2 type=int16 scope=local
; .debug_var: __mixed_optimization @_l_total offset=4 size=2 type=int16 scope=local
    .var @_p_points = 2
    .var @_p_count = 4
; .debug_var: __mixed_optimization @_p_points offset=2 size=2 type=ptr scope=parameter
; .debug_var: __mixed_optimization @_p_count offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "src/test-resources/test_phase108_validation.c", 194
    lda #0
    sta _mixed_optimization__local_2
    sta _mixed_optimization__local_2+1
    .loc "src/test-resources/test_phase108_validation.c", 195
    lda #0
    sta _mixed_optimization__local_4
    sta _mixed_optimization__local_4+1
@for_cond29:
    lda _mixed_optimization__param_count
    ldx _mixed_optimization__param_count+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _mixed_optimization__local_4
    ldx _mixed_optimization__local_4+1
    cmp.16 .AX, __zp_scratch2
    bcc @for_body30
    bra @for_end32
@for_body30:
    .loc "src/test-resources/test_phase108_validation.c", 196
    lda _mixed_optimization__param_points
    ldx _mixed_optimization__param_points+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _mixed_optimization__local_4
    ldx _mixed_optimization__local_4+1
    mul.16 .AX, #4
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda __zp_scratch2
    ; [peephole-opt]     ldx __zp_scratch2+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
    lda $22
    ldx $23
    sta $24
    stx $25
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $26
    stx $27
    ; [peephole-opt]     lda $26
    ; [peephole-opt]     ldx $27
    sta _mixed_optimization__local_7
    stx _mixed_optimization__local_7+1
    .loc "src/test-resources/test_phase108_validation.c", 197
    lda $22
    ldx $23
    sta $28
    stx $29
    lda $22
    ldx $23
    add.16 .AX, #2
    sta $2A
    stx $2B
    ldy #0
    lda ($2A),y
    pha
    iny
    lda ($2A),y
    tax
    pla
    sta $2C
    stx $2D
    ; [peephole-opt]     lda $2C
    ; [peephole-opt]     ldx $2D
    sta _mixed_optimization__local_11
    stx _mixed_optimization__local_11+1
    .loc "src/test-resources/test_phase108_validation.c", 198
    lda _mixed_optimization__local_7
    ; [peephole-opt]     ldx _mixed_optimization__local_7+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _mixed_optimization__local_7
    ldx _mixed_optimization__local_7+1
    mul.16 .AX, __zp_scratch2
    sta $2E
    stx $2F
    lda _mixed_optimization__local_11
    ; [peephole-opt]     ldx _mixed_optimization__local_11+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _mixed_optimization__local_11
    ; [peephole-opt]     ldx _mixed_optimization__local_11+1
    mul.16 .AX, __zp_scratch2
    sta $30
    stx $31
    ; [peephole-opt]     lda $2E
    clc
    adc $30
    sta $32
    lda $2F
    adc $30+1
    sta $33
    ; [peephole-opt]     lda $32
    ldx $33
    sta _mixed_optimization__local_15
    stx _mixed_optimization__local_15+1
    .loc "src/test-resources/test_phase108_validation.c", 199
    lda _mixed_optimization__local_15
    ; [peephole-opt]     ldx _mixed_optimization__local_15+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _mixed_optimization__local_2
    ldx _mixed_optimization__local_2+1
    add.16 .AX, __zp_scratch2
    sta $34
    stx $35
    sta _mixed_optimization__local_2
    stx _mixed_optimization__local_2+1
@for_inc31:
    .loc "src/test-resources/test_phase108_validation.c", 195
    lda _mixed_optimization__local_4
    ldx _mixed_optimization__local_4+1
    sta $36
    stx $37
    ; [peephole-opt]     lda $36
    ; [peephole-opt]     ldx $37
    add.16 .AX, #1
    sta _mixed_optimization__local_4
    stx _mixed_optimization__local_4+1
    bra @for_cond29
@for_end32:
    .loc "src/test-resources/test_phase108_validation.c", 201
    lda _mixed_optimization__local_2
    ldx _mixed_optimization__local_2+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .param_sizes 2, 2
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 14
    endproc

; function _main
; SAC inline storage: 96 bytes
    _main__local_0: .word 0
    _main__local_55: .word 0
    _main__local_57: .word 0
    _main__local_59: .word 0
    _main__local_84: .word 0
    _main__local_86: .word 0
    _main__local_89: .word 0
    _main__local_95: .word 0
    _main__local_97: .word 0
    _main__local_98: .word 0
    _main__local_99: .word 0
    _main__local_102: .word 0
    _main__local_103: .word 0
    _main__local_104: .word 0
    _main__local_144: .word 0
    _main__local_146: .word 0
    _main__local_148: .word 0
    _main__local_150: .word 0
    _main__local_152: .word 0
    _main__local_154: .word 0
    _main__local_177: .word 0
    _main__local_189: .word 0
    _main__local_190: .word 0
    _main__local_194: .word 0
    _main__local_203: .word 0
    _main__local_215: .long 0
    _main__local_217: .word 0
    _main__local_220: .word 0
    _main__local_228: .word 0
    _main__local_230: .word 0
    _main__local_233: .word 0
    _main__local_237: .word 0
    _main__local_241: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "src/test-resources/test_phase108_validation.c", 148
    .local @_l_a = 34
    .local @_l_b = 36
    .local @_l_c = 38
    .local @_l_d = 40
    .local @_l_dist_sq = 76
    .local @_l_i = 70
    .local @_l_pts = 78
    .local @_l_px = 72
    .local @_l_py = 74
    .local @_l_r1 = 50
    .local @_l_r2 = 52
    .local @_l_r3 = 54
    .local @_l_r4 = 56
    .local @_l_r5 = 58
    .local @_l_r6 = 60
    .local @_l_result = 64
    .local @_l_sum = 62
    .local @_l_t1 = 14
    .local @_l_t2 = 16
    .local @_l_t3 = 18
    .local @_l_test_arr = 86
    .local @_l_total = 68
    .local @_l_unused = 42
    .local @_l_used = 44
    .local @_l_x = 28
    .local @_l_y = 30
    .local @_l_z = 32
; .debug_var: __main @_l_a offset=34 size=2 type=int16 scope=local
; .debug_var: __main @_l_b offset=36 size=2 type=int16 scope=local
; .debug_var: __main @_l_c offset=38 size=2 type=int16 scope=local
; .debug_var: __main @_l_d offset=40 size=2 type=int16 scope=local
; .debug_var: __main @_l_dist_sq offset=76 size=2 type=int16 scope=local
; .debug_var: __main @_l_i offset=70 size=2 type=int16 scope=local
; .debug_var: __main @_l_pts offset=78 size=4 type=int32 scope=local
; .debug_var: __main @_l_px offset=72 size=2 type=int16 scope=local
; .debug_var: __main @_l_py offset=74 size=2 type=int16 scope=local
; .debug_var: __main @_l_r1 offset=50 size=2 type=int16 scope=local
; .debug_var: __main @_l_r2 offset=52 size=2 type=int16 scope=local
; .debug_var: __main @_l_r3 offset=54 size=2 type=int16 scope=local
; .debug_var: __main @_l_r4 offset=56 size=2 type=int16 scope=local
; .debug_var: __main @_l_r5 offset=58 size=2 type=int16 scope=local
; .debug_var: __main @_l_r6 offset=60 size=2 type=int16 scope=local
; .debug_var: __main @_l_result offset=64 size=2 type=int16 scope=local
; .debug_var: __main @_l_sum offset=62 size=2 type=int16 scope=local
; .debug_var: __main @_l_t1 offset=14 size=2 type=int16 scope=local
; .debug_var: __main @_l_t2 offset=16 size=2 type=int16 scope=local
; .debug_var: __main @_l_t3 offset=18 size=2 type=int16 scope=local
; .debug_var: __main @_l_test_arr offset=86 size=2 type=int16 scope=local
; .debug_var: __main @_l_total offset=68 size=2 type=int16 scope=local
; .debug_var: __main @_l_unused offset=42 size=2 type=int16 scope=local
; .debug_var: __main @_l_used offset=44 size=2 type=int16 scope=local
; .debug_var: __main @_l_x offset=28 size=2 type=int16 scope=local
; .debug_var: __main @_l_y offset=30 size=2 type=int16 scope=local
; .debug_var: __main @_l_z offset=32 size=2 type=int16 scope=local

@entry:
    .loc "src/test-resources/test_phase108_validation.c", 209
    lda #0
    sta _main__local_0
    sta _main__local_0+1
    .loc "src/test-resources/test_phase108_validation.c", 13
    lda #42
    ldx #0
@inline_end33:
    .loc "src/test-resources/test_phase108_validation.c", 212
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_0
    ldx _main__local_0+1
    add.16 .AX, __zp_scratch2
    sta _main__local_0
    stx _main__local_0+1
    .loc "src/test-resources/test_phase108_validation.c", 17
    lda #10
    ldx #0
    .loc "src/test-resources/test_phase108_validation.c", 18
    add.16 .AX, #5
@inline_end35:
    .loc "src/test-resources/test_phase108_validation.c", 213
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_0
    ; [peephole-opt]     ldx _main__local_0+1
    add.16 .AX, __zp_scratch2
    sta _main__local_0
    stx _main__local_0+1
    .loc "src/test-resources/test_phase108_validation.c", 216
    lda #10
    ldx #0
    sta $20
    stx $21
    .loc "src/test-resources/test_phase108_validation.c", 27
    lda #0
    sta $22
    sta $23
    .loc "src/test-resources/test_phase108_validation.c", 28
    lda #0
    sta $24
    sta $25
@for_cond38:
    ; [peephole-opt]     lda $24
    ldx $25
    cmp.16 .AX, $20
    bcc @for_body39
    bra @for_end41
@for_body39:
    .loc "src/test-resources/test_phase108_validation.c", 29
    lda #2
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    lsl.16 .AX
    sta $2A
    stx $2B
    lda $22
    clc
    adc $2A
    sta $22
    lda $23
    adc $2A+1
    sta $23
@for_inc40:
    .loc "src/test-resources/test_phase108_validation.c", 28
    lda $24
    ldx $25
    sta $2E
    stx $2F
    ; [peephole-opt]     lda $2E
    clc
    adc #1
    sta $24
    lda $2F
    adc #0
    sta $25
    bra @for_cond38
@for_end41:
    .loc "src/test-resources/test_phase108_validation.c", 31
    lda $22
    ldx $23
@inline_end37:
    .loc "src/test-resources/test_phase108_validation.c", 216
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_0
    ldx _main__local_0+1
    add.16 .AX, __zp_scratch2
    sta _main__local_0
    stx _main__local_0+1
    .loc "src/test-resources/test_phase108_validation.c", 217
    lda #5
    ldx #0
    sta $20
    stx $21
    ; [peephole-opt]     lda $20
    ; [peephole-opt]     ldx $21
    .loc "src/test-resources/test_phase108_validation.c", 35
    ; [peephole-opt]     lda $20
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    .loc "src/test-resources/test_phase108_validation.c", 36
    lda $20
    clc
    adc #1
    sta $24
    lda $21
    adc #0
    sta $25
    .loc "src/test-resources/test_phase108_validation.c", 37
    lda $20
    ; [peephole-opt]     ldx $21
    add.16 .AX, #1
    .loc "src/test-resources/test_phase108_validation.c", 38
    ; [peephole-opt]     lda $22
    ldx $23
    add.16 .AX, $24
    sta __zp_scratch2
    stx __zp_scratch2+1
    add.16 .AX, __zp_scratch2
@inline_end43:
    .loc "src/test-resources/test_phase108_validation.c", 217
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_0
    ldx _main__local_0+1
    add.16 .AX, __zp_scratch2
    sta _main__local_0
    stx _main__local_0+1
    .loc "src/test-resources/test_phase108_validation.c", 220
    lda #10
    ldx #0
    sta $20
    stx $21
    ; [peephole-opt]     lda $20
    ; [peephole-opt]     ldx $21
    .loc "src/test-resources/test_phase108_validation.c", 47
    lda #2
    ldx #0
    ; [peephole-opt]     lda $20
    ; [peephole-opt]     ldx $21
    lsl.16 .AX
    add.16 .AX, #1
@inline_end45:
    .loc "src/test-resources/test_phase108_validation.c", 220
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_0
    ; [peephole-opt]     ldx _main__local_0+1
    add.16 .AX, __zp_scratch2
    sta _main__local_0
    stx _main__local_0+1
    .loc "src/test-resources/test_phase108_validation.c", 221
    lda #1
    ldx #0
    sta $20
    stx $21
    lda #2
    ldx #0
    sta $22
    stx $23
    lda #3
    ldx #0
    sta $24
    stx $25
    ; [peephole-opt]     lda $20
    ; [peephole-opt]     ldx $21
    ; [peephole-opt]     lda $22
    ; [peephole-opt]     ldx $23
    ; [peephole-opt]     lda $24
    ; [peephole-opt]     ldx $25
    .loc "src/test-resources/test_phase108_validation.c", 51
    ; [peephole-opt]     lda $20
    ; [peephole-opt]     ldx $21
    add.16 .AX, $22
    sta _main__local_55
    stx _main__local_55+1
    .loc "src/test-resources/test_phase108_validation.c", 52
    ; [peephole-opt]     lda $22
    ; [peephole-opt]     ldx $23
    add.16 .AX, $24
    sta _main__local_57
    stx _main__local_57+1
    .loc "src/test-resources/test_phase108_validation.c", 53
    ; [peephole-opt]     lda $24
    ; [peephole-opt]     ldx $25
    add.16 .AX, $20
    sta _main__local_59
    stx _main__local_59+1
    .loc "src/test-resources/test_phase108_validation.c", 54
    lda _main__local_57
    ; [peephole-opt]     ldx _main__local_57+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_55
    ; [peephole-opt]     ldx _main__local_55+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    .loc "src/test-resources/test_phase108_validation.c", 55
    lda _main__local_59
    ; [peephole-opt]     ldx _main__local_59+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_57
    ; [peephole-opt]     ldx _main__local_57+1
    add.16 .AX, __zp_scratch2
    .loc "src/test-resources/test_phase108_validation.c", 56
    sta __zp_scratch2
    stx __zp_scratch2+1
    ; [peephole-opt]     lda $20
    ; [peephole-opt]     ldx $21
    add.16 .AX, __zp_scratch2
@inline_end47:
    .loc "src/test-resources/test_phase108_validation.c", 221
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_0
    ldx _main__local_0+1
    add.16 .AX, __zp_scratch2
    sta _main__local_0
    stx _main__local_0+1
    .loc "src/test-resources/test_phase108_validation.c", 222
    lda #5
    ldx #0
    sta $20
    stx $21
    .loc "src/test-resources/test_phase108_validation.c", 60
    lda #0
    sta $22
    sta $23
    .loc "src/test-resources/test_phase108_validation.c", 61
    lda #0
    sta $24
    sta $25
@for_cond50:
    ; [peephole-opt]     lda $24
    ldx $25
    cmp.16 .AX, $20
    bcc @for_body51
    bra @for_end53
@for_body51:
    .loc "src/test-resources/test_phase108_validation.c", 62
    lda $22
    clc
    adc $24
    sta $22
    lda $23
    adc $24+1
    sta $23
    .loc "src/test-resources/test_phase108_validation.c", 63
    lda #2
    ldx #0
    sta $2A
    stx $2B
    ; [peephole-opt]     lda $22
    ldx $23
    lsl.16 .AX
    sta $2C
    stx $2D
    ; [peephole-opt]     lda $2C
    ; [peephole-opt]     ldx $2D
    sta $22
    stx $23
    .loc "src/test-resources/test_phase108_validation.c", 64
    ; [peephole-opt]     lda $22
    bne *+4
    dec $23
    dec $22
@for_inc52:
    .loc "src/test-resources/test_phase108_validation.c", 61
    lda $24
    ldx $25
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    clc
    adc #1
    sta $24
    lda $33
    adc #0
    sta $25
    bra @for_cond50
@for_end53:
    .loc "src/test-resources/test_phase108_validation.c", 66
    lda $22
    ldx $23
@inline_end49:
    .loc "src/test-resources/test_phase108_validation.c", 222
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_0
    ldx _main__local_0+1
    add.16 .AX, __zp_scratch2
    sta _main__local_0
    stx _main__local_0+1
    .loc "src/test-resources/test_phase108_validation.c", 76
    lda #100
    sta _main__local_84
    lda #0
    sta _main__local_84+1
    .loc "src/test-resources/test_phase108_validation.c", 77
    lda _main__local_84
    ldx _main__local_84+1
    add.16 .AX, #50
    sta _main__local_86
    stx _main__local_86+1
    .loc "src/test-resources/test_phase108_validation.c", 78
    lda _main__local_84
    ldx _main__local_84+1
    add.16 .AX, #50
    sta _main__local_89
    stx _main__local_89+1
    .loc "src/test-resources/test_phase108_validation.c", 79
    lda _main__local_89
    ; [peephole-opt]     ldx _main__local_89+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_86
    ; [peephole-opt]     ldx _main__local_86+1
    add.16 .AX, __zp_scratch2
@inline_end55:
    .loc "src/test-resources/test_phase108_validation.c", 225
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_0
    ldx _main__local_0+1
    add.16 .AX, __zp_scratch2
    sta _main__local_0
    stx _main__local_0+1
    .loc "src/test-resources/test_phase108_validation.c", 84
    lda #42
    sta _main__local_95
    lda #0
    sta _main__local_95+1
    .loc "src/test-resources/test_phase108_validation.c", 85
    lda _main__local_95
    ldx _main__local_95+1
    sta _main__local_97
    stx _main__local_97+1
    .loc "src/test-resources/test_phase108_validation.c", 86
    lda _main__local_97
    ; [peephole-opt]     ldx _main__local_97+1
    sta _main__local_98
    stx _main__local_98+1
    .loc "src/test-resources/test_phase108_validation.c", 87
    lda _main__local_98
    ; [peephole-opt]     ldx _main__local_98+1
    sta _main__local_99
    stx _main__local_99+1
    .loc "src/test-resources/test_phase108_validation.c", 88
    lda _main__local_99
    ; [peephole-opt]     ldx _main__local_99+1
@inline_end57:
    .loc "src/test-resources/test_phase108_validation.c", 226
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_0
    ; [peephole-opt]     ldx _main__local_0+1
    add.16 .AX, __zp_scratch2
    sta _main__local_0
    stx _main__local_0+1
    .loc "src/test-resources/test_phase108_validation.c", 94
    lda #123
    sta _main__local_104
    lda #0
    sta _main__local_104+1
    .loc "src/test-resources/test_phase108_validation.c", 95
    lda _main__local_104
    ldx _main__local_104+1
@inline_end59:
    .loc "src/test-resources/test_phase108_validation.c", 227
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_0
    ; [peephole-opt]     ldx _main__local_0+1
    add.16 .AX, __zp_scratch2
    sta _main__local_0
    stx _main__local_0+1
    .loc "src/test-resources/test_phase108_validation.c", 112
    lda #0
    sta $20
    sta $21
    .loc "src/test-resources/test_phase108_validation.c", 113
    lda #0
    sta $22
    sta $23
@for_cond62:
    ; [peephole-opt]     lda $22
    ldx $23
    cmp.16 .AX, #10
    bcc @for_body63
    bra @for_end65
@for_body63:
    .loc "src/test-resources/test_phase108_validation.c", 114
    lda $22
    ldx $23
    sta $28
    stx $29
    .loc "src/test-resources/test_phase108_validation.c", 104
    lda $22
    clc
    adc #1
    sta $2C
    lda $23
    adc #0
    sta $2D
    ; [peephole-opt]     lda $2C
    ldx $2D
    sta $2E
    stx $2F
@inline_end66:
    .loc "src/test-resources/test_phase108_validation.c", 114
    lda $20
    clc
    adc $2E
    sta $20
    lda $21
    adc $2E+1
    sta $21
    .loc "src/test-resources/test_phase108_validation.c", 115
    lda $22
    clc
    adc #1
    sta $34
    lda $23
    adc #0
    sta $35
    lda $22
    ldx $23
    sta $36
    stx $37
    ; [peephole-opt]     lda $34
    ldx $35
    sta $38
    stx $39
    .loc "src/test-resources/test_phase108_validation.c", 108
    lda $22
    clc
    adc $34
    sta $3A
    lda $23
    adc $34+1
    sta $3B
    ; [peephole-opt]     lda $3A
    clc
    adc #1
    sta $3E
    ; [peephole-opt]     lda $3B
    adc #0
    sta $3F
    ; [peephole-opt]     lda $3E
    ldx $3F
    sta $40
    stx $41
@inline_end68:
    .loc "src/test-resources/test_phase108_validation.c", 115
    lda $20
    clc
    adc $40
    sta $20
    lda $21
    adc $40+1
    sta $21
@for_inc64:
    .loc "src/test-resources/test_phase108_validation.c", 113
    lda $22
    ldx $23
    sta $44
    stx $45
    ; [peephole-opt]     lda $44
    clc
    adc #1
    sta $22
    lda $45
    adc #0
    sta $23
    bra @for_cond62
@for_end65:
    .loc "src/test-resources/test_phase108_validation.c", 117
    lda $20
    ldx $21
@inline_end61:
    .loc "src/test-resources/test_phase108_validation.c", 230
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_0
    ldx _main__local_0+1
    add.16 .AX, __zp_scratch2
    sta _main__local_0
    stx _main__local_0+1
    .loc "src/test-resources/test_phase108_validation.c", 233
    lda #1
    ldx #0
    sta $20
    stx $21
    lda #2
    ldx #0
    sta $22
    stx $23
    lda #3
    ldx #0
    sta $24
    stx $25
    lda #4
    ldx #0
    sta $26
    stx $27
    lda #5
    ldx #0
    sta $28
    stx $29
    lda #6
    ldx #0
    sta $2A
    stx $2B
    ; [peephole-opt]     lda $20
    ; [peephole-opt]     ldx $21
    ; [peephole-opt]     lda $22
    ; [peephole-opt]     ldx $23
    ; [peephole-opt]     lda $24
    ; [peephole-opt]     ldx $25
    ; [peephole-opt]     lda $26
    ; [peephole-opt]     ldx $27
    ; [peephole-opt]     lda $28
    ; [peephole-opt]     ldx $29
    ; [peephole-opt]     lda $2A
    ; [peephole-opt]     ldx $2B
    .loc "src/test-resources/test_phase108_validation.c", 126
    lda $20
    ldx $21
    add.16 .AX, $22
    sta _main__local_144
    stx _main__local_144+1
    .loc "src/test-resources/test_phase108_validation.c", 127
    lda $24
    ldx $25
    add.16 .AX, $26
    sta _main__local_146
    stx _main__local_146+1
    .loc "src/test-resources/test_phase108_validation.c", 128
    lda $28
    ldx $29
    add.16 .AX, $2A
    sta _main__local_148
    stx _main__local_148+1
    .loc "src/test-resources/test_phase108_validation.c", 129
    lda _main__local_146
    ; [peephole-opt]     ldx _main__local_146+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_144
    ; [peephole-opt]     ldx _main__local_144+1
    add.16 .AX, __zp_scratch2
    sta _main__local_150
    stx _main__local_150+1
    .loc "src/test-resources/test_phase108_validation.c", 130
    lda _main__local_148
    ; [peephole-opt]     ldx _main__local_148+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_146
    ; [peephole-opt]     ldx _main__local_146+1
    add.16 .AX, __zp_scratch2
    sta _main__local_152
    stx _main__local_152+1
    .loc "src/test-resources/test_phase108_validation.c", 131
    lda _main__local_144
    ldx _main__local_144+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_148
    ; [peephole-opt]     ldx _main__local_148+1
    add.16 .AX, __zp_scratch2
    sta _main__local_154
    stx _main__local_154+1
    .loc "src/test-resources/test_phase108_validation.c", 132
    lda _main__local_152
    ; [peephole-opt]     ldx _main__local_152+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_150
    ; [peephole-opt]     ldx _main__local_150+1
    add.16 .AX, __zp_scratch2
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_154
    ; [peephole-opt]     ldx _main__local_154+1
    add.16 .AX, __zp_scratch2
@inline_end71:
    .loc "src/test-resources/test_phase108_validation.c", 233
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_0
    ldx _main__local_0+1
    add.16 .AX, __zp_scratch2
    sta _main__local_0
    stx _main__local_0+1
    .loc "src/test-resources/test_phase108_validation.c", 236
    lda #1
    ldx #0
    sta $20
    stx $21
    lda #2
    ldx #0
    sta $22
    stx $23
    lda #3
    ldx #0
    sta $24
    stx $25
    ; [peephole-opt]     lda $20
    ; [peephole-opt]     ldx $21
    sta $26
    stx $27
    ; [peephole-opt]     lda $22
    ; [peephole-opt]     ldx $23
    sta $28
    stx $29
    ; [peephole-opt]     lda $24
    ; [peephole-opt]     ldx $25
    sta $22
    stx $23
    .loc "src/test-resources/test_phase108_validation.c", 141
    ; [peephole-opt]     lda $20
    ; [peephole-opt]     ldx $21
    cmp.16 .AX, #0
    beq @if_else75
    bcs @if_then74
    bra @if_else75
@if_then74:
    .loc "src/test-resources/test_phase108_validation.c", 142
    lda $28
    ldx $29
    cmp.16 .AX, #0
    beq @if_else78
    bcs @if_then77
    bra @if_else78
@if_then77:
    .loc "src/test-resources/test_phase108_validation.c", 143
    lda $22
    ldx $23
    cmp.16 .AX, #0
    beq @if_else81
    bcs @if_then80
    bra @if_else81
@if_then80:
    .loc "src/test-resources/test_phase108_validation.c", 144
    lda $26
    ldx $27
    add.16 .AX, $28
    add.16 .AX, $22
    sta $20
    stx $21
    bra @inline_end73
@if_else81:
    .loc "src/test-resources/test_phase108_validation.c", 146
    lda $26
    ldx $27
    add.16 .AX, $28
    sta $20
    stx $21
    bra @inline_end73
@if_else78:
    .loc "src/test-resources/test_phase108_validation.c", 149
    lda $26
    ldx $27
    sta $20
    stx $21
    bra @inline_end73
@if_else75:
    .loc "src/test-resources/test_phase108_validation.c", 152
    lda #0
    tax
    sta $20
    stx $21
@inline_end73:
    .loc "src/test-resources/test_phase108_validation.c", 236
    lda _main__local_0
    ldx _main__local_0+1
    add.16 .AX, $20
    sta _main__local_0
    stx _main__local_0+1
    .loc "src/test-resources/test_phase108_validation.c", 239
    leax.local 86
    sta $20
    stx $21
    lda #1
    ldx #0
    pha
    phx
    struct_elem.16 __zp_scratch, $20, #0
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    lda #2
    ldx #0
    pha
    phx
    struct_elem.16 __zp_scratch, $20, #2
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    lda #3
    ldx #0
    pha
    phx
    struct_elem.16 __zp_scratch, $20, #4
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    lda #4
    ldx #0
    pha
    phx
    struct_elem.16 __zp_scratch, $20, #6
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    lda #5
    ldx #0
    pha
    phx
    struct_elem.16 __zp_scratch, $20, #8
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    .loc "src/test-resources/test_phase108_validation.c", 240
    leax.local 86
    sta _main__local_189
    stx _main__local_189+1
    lda #5
    ldx #0
    sta _main__local_190
    stx _main__local_190+1
    lda _main__local_189
    ; [peephole-opt]     ldx _main__local_189+1
    lda _main__local_190
    ; [peephole-opt]     ldx _main__local_190+1
    .loc "src/test-resources/test_phase108_validation.c", 162
    lda #0
    sta _main__local_194
    sta _main__local_194+1
    lda _main__local_190
    ; [peephole-opt]     ldx _main__local_190+1
    sta $28
    stx $29
    lda _main__local_189
    ; [peephole-opt]     ldx _main__local_189+1
    sta $2A
    stx $2B
    ; [peephole-opt]     lda $28
    ; [peephole-opt]     ldx $29
    push .ax
    ; [peephole-opt]     lda $2A
    ; [peephole-opt]     ldx $2B
    push .ax
    jsr ___idiom_sum16
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ; [peephole-opt]     ldx __zp_scratch4+1
    sta _main__local_194
    stx _main__local_194+1
    .loc "src/test-resources/test_phase108_validation.c", 166
    lda _main__local_194
    ; [peephole-opt]     ldx _main__local_194+1
@inline_end87:
    .loc "src/test-resources/test_phase108_validation.c", 240
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_0
    ldx _main__local_0+1
    add.16 .AX, __zp_scratch2
    sta _main__local_0
    stx _main__local_0+1
    .loc "src/test-resources/test_phase108_validation.c", 243
    leax.local 86
    lda #5
    ldx #0
    sta $20
    stx $21
    sta $22
    stx $23
    .loc "src/test-resources/test_phase108_validation.c", 175
    lda #0
    sta _main__local_203
    sta _main__local_203+1
    .loc "src/test-resources/test_phase108_validation.c", 176
    lda #0
    sta $24
    sta $25
@for_cond90:
    ; [peephole-opt]     lda $24
    ldx $25
    cmp.16 .AX, $22
    bcc @for_body91
    bra @for_end93
@for_body91:
    .loc "src/test-resources/test_phase108_validation.c", 177
    ldy #0
    lda ($20),y
    pha
    iny
    lda ($20),y
    tax
    pla
    sta $28
    stx $29
    lda _main__local_203
    ldx _main__local_203+1
    add.16 .AX, $28
    sta $2A
    stx $2B
    sta _main__local_203
    stx _main__local_203+1
    .loc "src/test-resources/test_phase108_validation.c", 178
    lda $20
    ldx $21
    sta $2C
    stx $2D
    ; [peephole-opt]     lda $2C
    ; [peephole-opt]     ldx $2D
    add.16 .AX, #2
    sta $2E
    stx $2F
    ; [peephole-opt]     lda $2E
    ; [peephole-opt]     ldx $2F
    ldy #0
    sta ($20),y
    txa
    iny
    sta ($20),y
@for_inc92:
    .loc "src/test-resources/test_phase108_validation.c", 176
    lda $24
    ldx $25
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    clc
    adc #1
    sta $24
    lda $31
    adc #0
    sta $25
    bra @for_cond90
@for_end93:
    .loc "src/test-resources/test_phase108_validation.c", 180
    lda _main__local_203
    ldx _main__local_203+1
@inline_end89:
    .loc "src/test-resources/test_phase108_validation.c", 243
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_0
    ldx _main__local_0+1
    add.16 .AX, __zp_scratch2
    sta _main__local_0
    stx _main__local_0+1
    .loc "src/test-resources/test_phase108_validation.c", 246
    leax.local 78
    sta $20
    stx $21
    lda #4
    ldx #0
    pha
    phx
    struct_elem.16 __zp_scratch, $20, #0
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    tya
    iny
    sta (__zp_scratch),y
    lda #12
    ldx #0
    pha
    phx
    struct_elem.16 __zp_scratch, $20, #4
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    tya
    iny
    sta (__zp_scratch),y
    .loc "src/test-resources/test_phase108_validation.c", 247
    leax.local 78
    lda #2
    ldx #0
    sta $20
    stx $21
    sta $22
    stx $23
    .loc "src/test-resources/test_phase108_validation.c", 194
    lda #0
    sta _main__local_228
    sta _main__local_228+1
    .loc "src/test-resources/test_phase108_validation.c", 195
    lda #0
    sta _main__local_230
    sta _main__local_230+1
@for_cond96:
    lda _main__local_230
    ldx _main__local_230+1
    cmp.16 .AX, $22
    bcc @for_body97
    bra @for_end99
@for_body97:
    .loc "src/test-resources/test_phase108_validation.c", 196
    lda _main__local_230
    ldx _main__local_230+1
    mul.16 .AX, #4
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
    sta $26
    stx $26+1
    lda $26
    ldx $27
    sta $28
    stx $29
    ldy #0
    lda ($26),y
    pha
    iny
    lda ($26),y
    tax
    pla
    sta $2A
    stx $2B
    ; [peephole-opt]     lda $2A
    ; [peephole-opt]     ldx $2B
    sta _main__local_233
    stx _main__local_233+1
    .loc "src/test-resources/test_phase108_validation.c", 197
    lda $26
    ldx $27
    sta $2C
    stx $2D
    lda $26
    ldx $27
    add.16 .AX, #2
    sta $2E
    stx $2F
    ldy #0
    lda ($2E),y
    pha
    iny
    lda ($2E),y
    tax
    pla
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    sta _main__local_237
    stx _main__local_237+1
    .loc "src/test-resources/test_phase108_validation.c", 198
    lda _main__local_233
    ; [peephole-opt]     ldx _main__local_233+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_233
    ldx _main__local_233+1
    mul.16 .AX, __zp_scratch2
    sta $32
    stx $33
    lda _main__local_237
    ; [peephole-opt]     ldx _main__local_237+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_237
    ; [peephole-opt]     ldx _main__local_237+1
    mul.16 .AX, __zp_scratch2
    sta $34
    stx $35
    ; [peephole-opt]     lda $32
    clc
    adc $34
    sta $36
    lda $33
    adc $34+1
    sta $37
    ; [peephole-opt]     lda $36
    ldx $37
    sta _main__local_241
    stx _main__local_241+1
    .loc "src/test-resources/test_phase108_validation.c", 199
    lda _main__local_241
    ; [peephole-opt]     ldx _main__local_241+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_228
    ldx _main__local_228+1
    add.16 .AX, __zp_scratch2
    sta $38
    stx $39
    sta _main__local_228
    stx _main__local_228+1
@for_inc98:
    .loc "src/test-resources/test_phase108_validation.c", 195
    lda _main__local_230
    ldx _main__local_230+1
    sta $3A
    stx $3B
    ; [peephole-opt]     lda $3A
    ; [peephole-opt]     ldx $3B
    add.16 .AX, #1
    sta _main__local_230
    stx _main__local_230+1
    bra @for_cond96
@for_end99:
    .loc "src/test-resources/test_phase108_validation.c", 201
    lda _main__local_228
    ldx _main__local_228+1
@inline_end95:
    .loc "src/test-resources/test_phase108_validation.c", 247
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_0
    ldx _main__local_0+1
    add.16 .AX, __zp_scratch2
    sta _main__local_0
    stx _main__local_0+1
    .loc "src/test-resources/test_phase108_validation.c", 249
    lda #0
    ldx #1
    lda _main__local_0
    ; [peephole-opt]     ldx _main__local_0+1
    and.16 .AX, #255
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 96
    endproc


__zp_save_buf:
; [DEBUG] Phase 87 code reached, optimize=true

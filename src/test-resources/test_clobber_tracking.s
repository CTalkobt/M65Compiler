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

    .global _g_val
    .global _g_flag
    .global _set_value
    .global _set_flag
    .global _noop
    .global _caller
    .global _get_value
    .global _add
    .global _main

    .segment "bss"
_g_val:
; .debug_var: @global _g_val offset=0 size=2 type=int16 scope=global
    .res 2
_g_flag:
; .debug_var: @global _g_flag offset=0 size=2 type=int8 scope=global
    .res 1

    .segment "code"

; function _set_value
; SAC inline storage: 2 bytes
    .global _set_value__param_v
    _set_value__param_v: .word 0
    _set_value__local_0: .word 0
    proc _set_value, W#@_p_v
    .sac
    .var _fp = 0
    .loc "src/test-resources/test_clobber_tracking.c", 10
    .var @_p_v = 2
; .debug_var: __set_value @_p_v offset=2 size=2 type=int16 scope=parameter

@entry:
    .loc "src/test-resources/test_clobber_tracking.c", 11
    lda _set_value__param_v
    ldx _set_value__param_v+1
    sta _g_val
    stx _g_val+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .param_sizes 2
    .reg_clobbers A, X
    .flag_clobbers N, Z
    .frame_size 2
    endproc

; function _set_flag
; SAC inline storage: 2 bytes
    .global _set_flag__param_f
    _set_flag__param_f: .word 0
    _set_flag__local_0: .word 0
    proc _set_flag, B#@_p_f
    .sac
    .var _fp = 0
    .loc "src/test-resources/test_clobber_tracking.c", 15
    .var @_p_f = 2
; .debug_var: __set_flag @_p_f offset=2 size=2 type=int8 scope=parameter

@entry:
    .loc "src/test-resources/test_clobber_tracking.c", 16
    lda.local 0
    sta _g_flag
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .param_sizes 2
    .reg_clobbers A, X
    .flag_clobbers N, Z
    .frame_size 2
    endproc

; function _noop
; SAC zero-alloc leaf: no storage overhead
    proc _noop
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "src/test-resources/test_clobber_tracking.c", 20

@entry:
@__return:
    rts
    .func_flags stack_call, static_alloc, zeroalloc, leaf
    .frame_size 0
    endproc

; function _caller
; SAC inline storage: 0 bytes
    _caller__local_2: .word 0
    _caller__local_3: .word 0
    _caller__local_4: .word 0
    proc _caller
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "src/test-resources/test_clobber_tracking.c", 24

@entry:
    .loc "src/test-resources/test_clobber_tracking.c", 25
    lda #42
    ldx #0
    .loc "src/test-resources/test_clobber_tracking.c", 11
    sta _g_val
    stx _g_val+1
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X
    .flag_clobbers N, Z
    .frame_size 0
    endproc

; function _get_value
; SAC zero-alloc leaf: no storage overhead
    proc _get_value
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "src/test-resources/test_clobber_tracking.c", 31

@entry:
    .loc "src/test-resources/test_clobber_tracking.c", 32
    lda _g_val
    ldx _g_val+1
@__return:
    rts
    .func_flags stack_call, static_alloc, zeroalloc, leaf
    .reg_clobbers A, X
    .flag_clobbers N, Z
    .frame_size 0
    endproc

; function _add
; SAC inline storage: 4 bytes
    .global _add__param_a
    _add__param_a: .word 0
    .global _add__param_b
    _add__param_b: .word 0
    _add__local_0: .word 0
    _add__local_1: .word 0
    proc _add, W#@_p_a, W#@_p_b
    .sac
    .var _fp = 0
    .loc "src/test-resources/test_clobber_tracking.c", 36
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __add @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __add @_p_b offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "src/test-resources/test_clobber_tracking.c", 37
    lda _add__param_b
    ldx _add__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _add__param_a
    ldx _add__param_a+1
    add.16 .AX, __zp_scratch2
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .param_sizes 2, 2
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _main
; SAC inline storage: 4 bytes
    _main__local_10: .word 0
    _main__local_14: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "src/test-resources/test_clobber_tracking.c", 40
    .local @_l_r = 0
    .local @_l_s = 2
; .debug_var: __main @_l_r offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_s offset=2 size=2 type=int16 scope=local

@entry:
    .loc "src/test-resources/test_clobber_tracking.c", 41
    lda #100
    ldx #0
    .loc "src/test-resources/test_clobber_tracking.c", 11
    sta _g_val
    stx _g_val+1
@inline_end3:
    .loc "src/test-resources/test_clobber_tracking.c", 42
    lda #1
    ldx #0
    .loc "src/test-resources/test_clobber_tracking.c", 16
    sta _g_flag
@inline_end4:
@inline_end5:
    .loc "src/test-resources/test_clobber_tracking.c", 25
    lda #42
    ldx #0
    .loc "src/test-resources/test_clobber_tracking.c", 11
    sta _g_val
    stx _g_val+1
@inline_end7:
    .loc "src/test-resources/test_clobber_tracking.c", 26
    lda #1
    ldx #0
    .loc "src/test-resources/test_clobber_tracking.c", 16
    sta _g_flag
@inline_end8:
@inline_end9:
@inline_end6:
    .loc "src/test-resources/test_clobber_tracking.c", 32
    lda _g_val
    ; [peephole-opt]     ldx _g_val+1
@inline_end10:
    .loc "src/test-resources/test_clobber_tracking.c", 45
    sta _main__local_10
    stx _main__local_10+1
    .loc "src/test-resources/test_clobber_tracking.c", 46
    lda #3
    ldx #0
    sta $20
    stx $21
    lda #4
    ldx #0
    sta $22
    stx $23
    ; [peephole-opt]     lda $20
    ; [peephole-opt]     ldx $21
    ; [peephole-opt]     lda $22
    ; [peephole-opt]     ldx $23
    .loc "src/test-resources/test_clobber_tracking.c", 37
    ; [peephole-opt]     lda $20
    ; [peephole-opt]     ldx $21
    add.16 .AX, $22
@inline_end12:
    .loc "src/test-resources/test_clobber_tracking.c", 46
    sta _main__local_14
    stx _main__local_14+1
    .loc "src/test-resources/test_clobber_tracking.c", 47
    lda _main__local_14
    ; [peephole-opt]     ldx _main__local_14+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_10
    ; [peephole-opt]     ldx _main__local_10+1
    add.16 .AX, __zp_scratch2
    sta _g_val
    stx _g_val+1
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc


__zp_save_buf:
; [DEBUG] Phase 87 code reached, optimize=true

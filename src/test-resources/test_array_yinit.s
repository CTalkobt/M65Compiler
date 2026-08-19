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

    .global _output
    .global _set_y_nonzero
    .global _main

    .segment "data"
    .byte 0
_output:
; .debug_var: @global _output offset=0 size=2 type=ptr scope=global
    .word 16384

    .segment "code"

; function _set_y_nonzero
; SAC zero-alloc leaf: no storage overhead
    proc _set_y_nonzero
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_array_yinit.c", 6

@entry:
    .loc "test_array_yinit.c", 7
    ldy #5
@__return:
    rts
    .func_flags stack_call, static_alloc, zeroalloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 0
    endproc

; function _main
; SAC inline storage: 8 bytes
    _main__local_0: .word 0
    _main__local_1: .word 0
    _main__local_2: .word 0
    _main__local_3: .word 0
    _main__local_4: .word 0
    _main__local_5: .word 0
    _main__local_6: .word 0
    _main__local_7: .word 0
    _main__local_8: .word 0
    _main__local_9: .word 0
    _main__local_10: .word 0
    _main__local_11: .word 0
    _main__local_12: .word 0
    _main__local_13: .word 0
    _main__local_14: .word 0
    _main__local_15: .word 0
    _main__local_16: .word 0
    _main__local_17: .word 0
    _main__local_18: .word 0
    _main__local_19: .word 0
    _main__local_20: .word 0
    _main__local_21: .word 0
    _main__local_22: .word 0
    _main__local_23: .word 0
    _main__local_24: .word 0
    _main__local_25: .word 0
    _main__local_26: .word 0
    _main__local_27: .word 0
    _main__local_28: .word 0
    _main__local_29: .word 0
    _main__local_30: .word 0
    _main__local_31: .word 0
    _main__local_32: .word 0
    _main__local_33: .word 0
    _main__local_34: .word 0
    _main__local_35: .word 0
    _main__local_36: .word 0
    _main__local_37: .word 0
    _main__local_38: .word 0
    _main__local_39: .word 0
    _main__local_40: .word 0
    _main__local_41: .word 0
    _main__local_42: .word 0
    _main__local_43: .word 0
    _main__local_44: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_array_yinit.c", 10
    .local @_l_arr = 2
    .local @_l_val = 0
; .debug_var: __main @_l_arr offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_val offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_array_yinit.c", 7
    ldy #5
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 8
    endproc


__zp_save_buf:

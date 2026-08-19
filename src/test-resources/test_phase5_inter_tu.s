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

    .global _leaf_compute
    .global _leaf_combine
    .global _chain_compute
    .global _chain_combine
    .global _main

    .segment "code"

; function _leaf_compute
; SAC inline storage: 2 bytes
    .global _leaf_compute__param_x
    _leaf_compute__param_x: .word 0
    _leaf_compute__local_0: .word 0
    proc _leaf_compute, W#@_p_x
    .sac
    .var _fp = 0
    .loc "test_phase5_inter_tu.c", 5
    .var @_p_x = 2
; .debug_var: __leaf_compute @_p_x offset=2 size=2 type=int16 scope=parameter

@entry:
    .loc "test_phase5_inter_tu.c", 6
    lda #2
    ldx #0
    sta $20
    stx $21
    lda _leaf_compute__param_x
    ldx _leaf_compute__param_x+1
    lsl.16 .AX
    sta $20
    stx $21
    lda $20
    clc
    adc #5
    sta $24
    lda $21
    adc #0
    sta $25
    lda $24
    ldx $25
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _leaf_combine
; SAC inline storage: 6 bytes
    .global _leaf_combine__param_a
    _leaf_combine__param_a: .word 0
    .global _leaf_combine__param_b
    _leaf_combine__param_b: .word 0
    .global _leaf_combine__param_c
    _leaf_combine__param_c: .word 0
    _leaf_combine__local_0: .word 0
    _leaf_combine__local_1: .word 0
    _leaf_combine__local_2: .word 0
    proc _leaf_combine, W#@_p_a, W#@_p_b, W#@_p_c
    .sac
    .var _fp = 0
    .loc "test_phase5_inter_tu.c", 10
    .var @_p_a = 2
    .var @_p_b = 4
    .var @_p_c = 6
; .debug_var: __leaf_combine @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __leaf_combine @_p_b offset=4 size=2 type=int16 scope=parameter
; .debug_var: __leaf_combine @_p_c offset=6 size=2 type=int16 scope=parameter

@entry:
    .loc "test_phase5_inter_tu.c", 11
    lda _leaf_combine__param_b
    ldx _leaf_combine__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _leaf_combine__param_a
    ldx _leaf_combine__param_a+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda _leaf_combine__param_c
    ldx _leaf_combine__param_c+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx $21
    mul.16 .AX, __zp_scratch2
    sta $22
    stx $23
    lda $22
    sec
    sbc #10
    sta $24
    lda $23
    sbc #0
    sta $25
    lda $24
    ldx $25
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _chain_compute
; SAC inline storage: 4 bytes
    .global _chain_compute__param_x
    _chain_compute__param_x: .word 0
    .global _chain_compute__param_y
    _chain_compute__param_y: .word 0
    _chain_compute__local_0: .word 0
    _chain_compute__local_1: .word 0
    proc _chain_compute, W#@_p_x, W#@_p_y
    .sac
    .var _fp = 0
    .loc "test_phase5_inter_tu.c", 15
    .var @_p_x = 2
    .var @_p_y = 4
; .debug_var: __chain_compute @_p_x offset=2 size=2 type=int16 scope=parameter
; .debug_var: __chain_compute @_p_y offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_phase5_inter_tu.c", 16
    lda _chain_compute__param_x
    ldx _chain_compute__param_x+1
    sta $20
    stx $21
    .loc "test_phase5_inter_tu.c", 6
    lda #2
    ldx #0
    sta $20
    stx $21
    lda _chain_compute__param_x
    ldx _chain_compute__param_x+1
    lsl.16 .AX
    sta $20
    stx $21
    lda $20
    clc
    adc #5
    sta $24
    lda $21
    adc #0
    sta $25
    lda $24
    ldx $25
    sta $20
    stx $21
@inline_end0:
    .loc "test_phase5_inter_tu.c", 16
    lda _chain_compute__param_y
    ldx _chain_compute__param_y+1
    sta $22
    stx $23
    .loc "test_phase5_inter_tu.c", 6
    lda #2
    ldx #0
    sta $22
    stx $23
    lda _chain_compute__param_y
    ldx _chain_compute__param_y+1
    lsl.16 .AX
    sta $22
    stx $23
    lda $22
    clc
    adc #5
    sta $26
    lda $23
    adc #0
    sta $27
    lda $26
    ldx $27
    sta $22
    stx $23
@inline_end2:
    .loc "test_phase5_inter_tu.c", 16
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
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _chain_combine
; SAC inline storage: 8 bytes
    .global _chain_combine__param_a
    _chain_combine__param_a: .word 0
    .global _chain_combine__param_b
    _chain_combine__param_b: .word 0
    .global _chain_combine__param_c
    _chain_combine__param_c: .word 0
    _chain_combine__local_0: .word 0
    _chain_combine__local_1: .word 0
    _chain_combine__local_2: .word 0
    _chain_combine__local_3: .word 0
    proc _chain_combine, W#@_p_a, W#@_p_b, W#@_p_c
    .sac
    .var _fp = 0
    .loc "test_phase5_inter_tu.c", 20
    .local @_l_temp = 6
; .debug_var: __chain_combine @_l_temp offset=6 size=2 type=int16 scope=local
    .var @_p_a = 2
    .var @_p_b = 4
    .var @_p_c = 6
; .debug_var: __chain_combine @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __chain_combine @_p_b offset=4 size=2 type=int16 scope=parameter
; .debug_var: __chain_combine @_p_c offset=6 size=2 type=int16 scope=parameter

@entry:
    .loc "test_phase5_inter_tu.c", 21
    lda _chain_combine__param_a
    ldx _chain_combine__param_a+1
    sta $20
    stx $21
    lda _chain_combine__param_b
    ldx _chain_combine__param_b+1
    sta $20
    stx $21
    lda _chain_combine__param_c
    ldx _chain_combine__param_c+1
    sta $20
    stx $21
    .loc "test_phase5_inter_tu.c", 11
    lda _chain_combine__param_b
    ldx _chain_combine__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _chain_combine__param_a
    ldx _chain_combine__param_a+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda _chain_combine__param_c
    ldx _chain_combine__param_c+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx $21
    mul.16 .AX, __zp_scratch2
    sta $22
    stx $23
    lda $22
    sec
    sbc #10
    sta $24
    lda $23
    sbc #0
    sta $25
    lda $24
    ldx $25
    sta $20
    stx $21
@inline_end4:
    .loc "test_phase5_inter_tu.c", 21
    lda $20
    ldx $21
    sta _chain_combine__local_3
    stx _chain_combine__local_3+1
    .loc "test_phase5_inter_tu.c", 22
    lda _chain_combine__local_3
    ldx _chain_combine__local_3+1
    sta $20
    stx $21
    .loc "test_phase5_inter_tu.c", 6
    lda #2
    ldx #0
    sta $20
    stx $21
    lda _chain_combine__local_3
    ldx _chain_combine__local_3+1
    lsl.16 .AX
    sta $20
    stx $21
    lda $20
    clc
    adc #5
    sta $24
    lda $21
    adc #0
    sta $25
    lda $24
    ldx $25
    sta $20
    stx $21
@inline_end6:
    .loc "test_phase5_inter_tu.c", 22
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 8
    endproc

; function _main
; SAC inline storage: 10 bytes
    _main__local_0: .word 0
    _main__local_9: .word 0
    _main__local_22: .word 0
    _main__local_44: .word 0
    _main__local_52: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_phase5_inter_tu.c", 25
    .local @_l_r1 = 0
    .local @_l_r2 = 2
    .local @_l_r3 = 4
    .local @_l_r4 = 6
    .local @_l_temp = 8
; .debug_var: __main @_l_r1 offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_r2 offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_r3 offset=4 size=2 type=int16 scope=local
; .debug_var: __main @_l_r4 offset=6 size=2 type=int16 scope=local
; .debug_var: __main @_l_temp offset=8 size=2 type=int16 scope=local

@entry:
    .loc "test_phase5_inter_tu.c", 27
    .loc "test_phase5_inter_tu.c", 6
@inline_end8:
    .loc "test_phase5_inter_tu.c", 27
    .loc "test_phase5_inter_tu.c", 30
    .loc "test_phase5_inter_tu.c", 11
@inline_end10:
    .loc "test_phase5_inter_tu.c", 30
    .loc "test_phase5_inter_tu.c", 33
    .loc "test_phase5_inter_tu.c", 16
    .loc "test_phase5_inter_tu.c", 6
@inline_end13:
    .loc "test_phase5_inter_tu.c", 16
    .loc "test_phase5_inter_tu.c", 6
@inline_end15:
    .loc "test_phase5_inter_tu.c", 16
@inline_end12:
    .loc "test_phase5_inter_tu.c", 33
    .loc "test_phase5_inter_tu.c", 36
    .loc "test_phase5_inter_tu.c", 21
    .loc "test_phase5_inter_tu.c", 11
@inline_end19:
    .loc "test_phase5_inter_tu.c", 21
    .loc "test_phase5_inter_tu.c", 22
    .loc "test_phase5_inter_tu.c", 6
@inline_end21:
    .loc "test_phase5_inter_tu.c", 22
@inline_end18:
    .loc "test_phase5_inter_tu.c", 36
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .frame_size 10
    endproc


__zp_save_buf:

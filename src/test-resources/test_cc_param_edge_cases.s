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

    .global _results
    .global _zp_four_params
    .global _stack_five_params
    .global _main
    .global _main_zp_mixed

    .segment "data"
    .byte 0
_results:
; .debug_var: @global _results offset=0 size=2 type=ptr scope=global
    .word 16384

    .segment "code"

; function _zp_four_params
; SAC inline storage: 8 bytes
    .global _zp_four_params__param_a
    _zp_four_params__param_a: .word 0
    .global _zp_four_params__param_b
    _zp_four_params__param_b: .word 0
    .global _zp_four_params__param_c
    _zp_four_params__param_c: .word 0
    .global _zp_four_params__param_d
    _zp_four_params__param_d: .word 0
    _zp_four_params__local_0: .word 0
    _zp_four_params__local_1: .word 0
    _zp_four_params__local_2: .word 0
    _zp_four_params__local_3: .word 0
    proc _zp_four_params, W#@_p_a, W#@_p_b, W#@_p_c, W#@_p_d
    .sac
    .var _fp = 0
    .loc "test_cc_param_edge_cases.c", 6
    .var @_p_a = 2
    .var @_p_b = 4
    .var @_p_c = 6
    .var @_p_d = 8
; .debug_var: __zp_four_params @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __zp_four_params @_p_b offset=4 size=2 type=int16 scope=parameter
; .debug_var: __zp_four_params @_p_c offset=6 size=2 type=int16 scope=parameter
; .debug_var: __zp_four_params @_p_d offset=8 size=2 type=int16 scope=parameter

@entry:
    .loc "test_cc_param_edge_cases.c", 7
    lda _zp_four_params__param_b
    ldx _zp_four_params__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _zp_four_params__param_a
    ldx _zp_four_params__param_a+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda _zp_four_params__param_c
    ldx _zp_four_params__param_c+1
    add.16 .AX, $20
    sta $22
    stx $23
    lda _zp_four_params__param_d
    ldx _zp_four_params__param_d+1
    add.16 .AX, $22
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 8
    endproc

; function _stack_five_params
; SAC inline storage: 10 bytes
    .global _stack_five_params__param_a
    _stack_five_params__param_a: .word 0
    .global _stack_five_params__param_b
    _stack_five_params__param_b: .word 0
    .global _stack_five_params__param_c
    _stack_five_params__param_c: .word 0
    .global _stack_five_params__param_d
    _stack_five_params__param_d: .word 0
    .global _stack_five_params__param_e
    _stack_five_params__param_e: .word 0
    _stack_five_params__local_0: .word 0
    _stack_five_params__local_1: .word 0
    _stack_five_params__local_2: .word 0
    _stack_five_params__local_3: .word 0
    _stack_five_params__local_4: .word 0
    proc _stack_five_params, W#@_p_a, W#@_p_b, W#@_p_c, W#@_p_d, W#@_p_e
    .sac
    .var _fp = 0
    .loc "test_cc_param_edge_cases.c", 11
    .var @_p_a = 2
    .var @_p_b = 4
    .var @_p_c = 6
    .var @_p_d = 8
    .var @_p_e = 10
; .debug_var: __stack_five_params @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __stack_five_params @_p_b offset=4 size=2 type=int16 scope=parameter
; .debug_var: __stack_five_params @_p_c offset=6 size=2 type=int16 scope=parameter
; .debug_var: __stack_five_params @_p_d offset=8 size=2 type=int16 scope=parameter
; .debug_var: __stack_five_params @_p_e offset=10 size=2 type=int16 scope=parameter

@entry:
    .loc "test_cc_param_edge_cases.c", 12
    lda _stack_five_params__param_b
    ldx _stack_five_params__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _stack_five_params__param_a
    ldx _stack_five_params__param_a+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda _stack_five_params__param_c
    ldx _stack_five_params__param_c+1
    add.16 .AX, $20
    sta $22
    stx $23
    lda _stack_five_params__param_d
    ldx _stack_five_params__param_d+1
    add.16 .AX, $22
    sta $20
    stx $21
    lda _stack_five_params__param_e
    ldx _stack_five_params__param_e+1
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
; SAC inline storage: 8 bytes
    _main__local_0: .word 0
    _main__local_18: .word 0
    _main__local_41: .word 0
    _main__local_42: .word 0
    _main__local_44: .word 0
    _main__local_45: .word 0
    _main__local_54: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_cc_param_edge_cases.c", 16
    .local @_l_r1 = 0
    .local @_l_r2 = 2
    .local @_l_r3 = 4
    .local @_l_r4 = 6
; .debug_var: __main @_l_r1 offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_r2 offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_r3 offset=4 size=2 type=int16 scope=local
; .debug_var: __main @_l_r4 offset=6 size=2 type=int16 scope=local

@entry:
    .loc "test_cc_param_edge_cases.c", 18
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
    lda $20
    ldx $21
    sta $28
    stx $29
    lda $22
    ldx $23
    sta $28
    stx $29
    lda $24
    ldx $25
    sta $28
    stx $29
    lda $26
    ldx $27
    sta $28
    stx $29
    .loc "test_cc_param_edge_cases.c", 7
    lda $20
    clc
    adc #2
    sta $28
    lda $21
    adc #0
    sta $29
    lda $28
    clc
    adc #3
    sta $20
    lda $29
    adc #0
    sta $21
    lda $20
    clc
    adc #4
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end0:
    .loc "test_cc_param_edge_cases.c", 18
    lda $22
    ldx $23
    sta _main__local_0
    stx _main__local_0+1
    .loc "test_cc_param_edge_cases.c", 19
    lda _main__local_0
    ldx _main__local_0+1
    sta $20
    lda _results
    ldx _results+1
    sta $22
    stx $23
    lda #0
    sta $24
    sta $25
    lda $20
    ldx #0
    pha
    lda $24
    ldx $25
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
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "test_cc_param_edge_cases.c", 22
    lda #10
    ldx #0
    sta $20
    stx $21
    lda #20
    ldx #0
    sta $22
    stx $23
    lda #30
    ldx #0
    sta $24
    stx $25
    lda #40
    ldx #0
    sta $26
    stx $27
    lda #50
    ldx #0
    sta $28
    stx $29
    lda $20
    ldx $21
    sta $2A
    stx $2B
    lda $22
    ldx $23
    sta $2A
    stx $2B
    lda $24
    ldx $25
    sta $2A
    stx $2B
    lda $26
    ldx $27
    sta $2A
    stx $2B
    lda $28
    ldx $29
    sta $2A
    stx $2B
    .loc "test_cc_param_edge_cases.c", 12
    lda $20
    clc
    adc #20
    sta $2A
    lda $21
    adc #0
    sta $2B
    lda $2A
    clc
    adc #30
    sta $20
    lda $2B
    adc #0
    sta $21
    lda $20
    clc
    adc #40
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    clc
    adc #50
    sta $20
    lda $23
    adc #0
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end2:
    .loc "test_cc_param_edge_cases.c", 22
    lda $20
    ldx $21
    sta _main__local_18
    stx _main__local_18+1
    .loc "test_cc_param_edge_cases.c", 23
    lda #255
    ldx #0
    sta $20
    stx $21
    lda _main__local_18
    ldx _main__local_18+1
    and $20
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    lda _results
    ldx _results+1
    sta $20
    stx $21
    lda #1
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    pha
    lda $24
    ldx $25
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
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "test_cc_param_edge_cases.c", 30
    lda #5
    ldx #0
    sta $20
    stx $21
    lda #100
    ldx #0
    sta _main__local_44
    stx _main__local_44+1
    lda $20
    ldx $21
    sta _main__local_45
    lda $FD
    ldx $FE
    sta $20
    stx $21
    lda $20
    ldx $21
    sta __static_chain
    stx __static_chain+1
    lda _main__local_45
    sta _main_zp_mixed__param_0
    stx _main_zp_mixed__param_0+1
    jsr _main_zp_mixed
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_42
    stx _main__local_42+1
    .loc "test_cc_param_edge_cases.c", 31
    lda #255
    ldx #0
    sta $20
    stx $21
    lda _main__local_42
    ldx _main__local_42+1
    and $20
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    lda _results
    ldx _results+1
    sta $20
    stx $21
    lda #2
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    pha
    lda $24
    ldx $25
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
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "test_cc_param_edge_cases.c", 34
    lda #11
    ldx #0
    sta $20
    stx $21
    lda #22
    ldx #0
    sta $22
    stx $23
    lda #33
    ldx #0
    sta $24
    stx $25
    lda #44
    ldx #0
    sta $26
    stx $27
    lda $20
    ldx $21
    sta $28
    stx $29
    lda $22
    ldx $23
    sta $28
    stx $29
    lda $24
    ldx $25
    sta $28
    stx $29
    lda $26
    ldx $27
    sta $28
    stx $29
    .loc "test_cc_param_edge_cases.c", 7
    lda $20
    clc
    adc #22
    sta $28
    lda $21
    adc #0
    sta $29
    lda $28
    clc
    adc #33
    sta $20
    lda $29
    adc #0
    sta $21
    lda $20
    clc
    adc #44
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end4:
    .loc "test_cc_param_edge_cases.c", 34
    lda $22
    ldx $23
    sta _main__local_54
    stx _main__local_54+1
    .loc "test_cc_param_edge_cases.c", 35
    lda #255
    ldx #0
    sta $20
    stx $21
    lda _main__local_54
    ldx _main__local_54+1
    and $20
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    lda _results
    ldx _results+1
    sta $20
    stx $21
    lda #3
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    pha
    lda $24
    ldx $25
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
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "test_cc_param_edge_cases.c", 38
    lda #255
    sta $20
    lda _results
    ldx _results+1
    sta $22
    stx $23
    lda #4
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx #0
    pha
    lda $24
    ldx $25
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
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "test_cc_param_edge_cases.c", 40
    brk
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 8
    endproc

; function _main_zp_mixed
; SAC inline storage: 4 bytes
    .global _main_zp_mixed__param_a
    _main_zp_mixed__param_a: .word 0
    .global _main_zp_mixed__param_b
    _main_zp_mixed__param_b: .word 100
    _main_zp_mixed__local_0: .word 0
    _main_zp_mixed__local_1: .word 0
    proc _main_zp_mixed, B#@_p_a, W#@_p_b
    .sac
    .var _fp = 0
    .loc "test_cc_param_edge_cases.c", 27
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __main_zp_mixed @_p_a offset=2 size=2 type=int8 scope=parameter
; .debug_var: __main_zp_mixed @_p_b offset=4 size=2 type=int16 scope=parameter

@entry:
    lda _main_zp_mixed__param_a
    ldx #0
    sta $20
    stx $21
    .loc "test_cc_param_edge_cases.c", 28
    lda _main_zp_mixed__param_b
    ldx _main_zp_mixed__param_b+1
    add.16 .AX, $20
    sta $22
    stx $23
    lda $22
    ldx $23
@__return:
    rts
    .param_const _main_zp_mixed 1 100
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc


__zp_save_buf:

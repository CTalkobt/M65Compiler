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
    .global _stack_func
    .global _zp_func
    .global _call_via_ptr
    .global _main

    .segment "data"
    .byte 0
_results:
; .debug_var: @global _results offset=0 size=2 type=ptr scope=global
    .word 16384

    .segment "code"

; function _stack_func
; SAC inline storage: 4 bytes
    .global _stack_func__param_a
    _stack_func__param_a: .word 0
    .global _stack_func__param_b
    _stack_func__param_b: .word 0
    _stack_func__local_0: .word 0
    _stack_func__local_1: .word 0
    proc _stack_func, W#@_p_a, W#@_p_b
    .sac
    .var _fp = 0
    .loc "test_cc_indirect.c", 7
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __stack_func @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __stack_func @_p_b offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_cc_indirect.c", 8
    lda _stack_func__param_b
    ldx _stack_func__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _stack_func__param_a
    ldx _stack_func__param_a+1
    mul.16 .AX, __zp_scratch2
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

; function _zp_func
; SAC inline storage: 4 bytes
    .global _zp_func__param_a
    _zp_func__param_a: .word 0
    .global _zp_func__param_b
    _zp_func__param_b: .word 0
    _zp_func__local_0: .word 0
    _zp_func__local_1: .word 0
    proc _zp_func, W#@_p_a, W#@_p_b
    .sac
    .var _fp = 0
    .loc "test_cc_indirect.c", 12
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __zp_func @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __zp_func @_p_b offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_cc_indirect.c", 13
    lda _zp_func__param_b
    ldx _zp_func__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _zp_func__param_a
    ldx _zp_func__param_a+1
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

; function _call_via_ptr
; SAC inline storage: 6 bytes
    .global _call_via_ptr__param_f
    _call_via_ptr__param_f: .word 0
    .global _call_via_ptr__param_x
    _call_via_ptr__param_x: .word 0
    .global _call_via_ptr__param_y
    _call_via_ptr__param_y: .word 0
    _call_via_ptr__local_0: .word 0
    _call_via_ptr__local_1: .word 0
    _call_via_ptr__local_2: .word 0
    proc _call_via_ptr, W#@_p_f, W#@_p_x, W#@_p_y
    .sac
    .var _fp = 0
    .loc "test_cc_indirect.c", 17
    .var @_p_f = 2
    .var @_p_x = 4
    .var @_p_y = 6
; .debug_var: __call_via_ptr @_p_f offset=2 size=2 type=ptr scope=parameter
; .debug_var: __call_via_ptr @_p_x offset=4 size=2 type=int16 scope=parameter
; .debug_var: __call_via_ptr @_p_y offset=6 size=2 type=int16 scope=parameter

@entry:
    .loc "test_cc_indirect.c", 18
    lda _call_via_ptr__param_y
    ldx _call_via_ptr__param_y+1
    sta $28
    stx $29
    lda _call_via_ptr__param_x
    ldx _call_via_ptr__param_x+1
    sta $2A
    stx $2B
    lda $28
    ldx $29
    push .ax
    lda $2A
    ldx $2B
    push .ax
    lda _call_via_ptr__param_f
    ldx _call_via_ptr__param_f+1
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
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _main
; SAC inline storage: 6 bytes
    _main__local_0: .word 0
    _main__local_1: .word 0
    _main__local_2: .word 0
    _main__local_22: .word 0
    _main__local_23: .word 0
    _main__local_24: .word 0
    _main__local_25: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_cc_indirect.c", 21
    .local @_l_fptr = 2
    .local @_l_r1 = 0
    .local @_l_r2 = 4
; .debug_var: __main @_l_fptr offset=2 size=2 type=ptr scope=local
; .debug_var: __main @_l_r1 offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_r2 offset=4 size=2 type=int16 scope=local

@entry:
    .loc "test_cc_indirect.c", 23
    lda #6
    ldx #0
    sta _main__local_1
    stx _main__local_1+1
    lda #7
    ldx #0
    sta _main__local_2
    stx _main__local_2+1
    ldax #_stack_func
    sta $20
    stx $21
    lda _main__local_1
    ldx _main__local_1+1
    sta $20
    stx $21
    lda _main__local_2
    ldx _main__local_2+1
    sta $20
    stx $21
    .loc "test_cc_indirect.c", 18
    lda #7
    ldx #0
    push .ax
    lda #6
    ldx #0
    push .ax
    ldax #_stack_func
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
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end0:
    .loc "test_cc_indirect.c", 23
    lda $20
    ldx $21
    sta _main__local_0
    stx _main__local_0+1
    .loc "test_cc_indirect.c", 24
    lda #255
    ldx #0
    sta $20
    stx $21
    lda _main__local_0
    ldx _main__local_0+1
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
    lda #0
    sta $24
    sta $25
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
    .loc "test_cc_indirect.c", 25
    lda _main__local_0
    ldx _main__local_0+1
    txa
    ldx #0
    sta $20
    stx $21
    lda #255
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    and $22
    sta $24
    stx $25
    lda $24
    ldx $25
    sta $20
    lda _results
    ldx _results+1
    sta $20
    stx $21
    lda #1
    ldx #0
    sta $22
    stx $23
    lda $24
    ldx $25
    pha
    lda $22
    ldx $23
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
    .loc "test_cc_indirect.c", 33
    ldax #_zp_func
    sta _main__local_22
    stx _main__local_22+1
    .loc "test_cc_indirect.c", 34
    lda #5
    ldx #0
    sta _main__local_24
    stx _main__local_24+1
    lda #3
    ldx #0
    sta _main__local_25
    stx _main__local_25+1
    lda _main__local_22
    ldx _main__local_22+1
    sta $20
    stx $21
    lda _main__local_24
    ldx _main__local_24+1
    sta $20
    stx $21
    lda _main__local_25
    ldx _main__local_25+1
    sta $20
    stx $21
    .loc "test_cc_indirect.c", 18
    lda #3
    ldx #0
    push .ax
    lda #5
    ldx #0
    push .ax
    lda _main__local_22
    ldx _main__local_22+1
    sta @__call_site_2+1
    stx @__call_site_2+2
@__call_site_2:
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
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end2:
    .loc "test_cc_indirect.c", 34
    lda $20
    ldx $21
    sta _main__local_23
    stx _main__local_23+1
    .loc "test_cc_indirect.c", 35
    lda #255
    ldx #0
    sta $20
    stx $21
    lda _main__local_23
    ldx _main__local_23+1
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
    .loc "test_cc_indirect.c", 36
    lda _main__local_23
    ldx _main__local_23+1
    txa
    ldx #0
    sta $20
    stx $21
    lda #255
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    and $22
    sta $24
    stx $25
    lda $24
    ldx $25
    sta $20
    lda _results
    ldx _results+1
    sta $20
    stx $21
    lda #3
    ldx #0
    sta $22
    stx $23
    lda $24
    ldx $25
    pha
    lda $22
    ldx $23
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
    .loc "test_cc_indirect.c", 39
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
    .loc "test_cc_indirect.c", 41
    brk
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc


__zp_save_buf:

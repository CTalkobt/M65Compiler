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

    .global _add
    .global _sub
    .global _apply
    .global _apply2
    .global _main

    .segment "code"

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
    .loc "test_funcptr.c", 3
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __add @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __add @_p_b offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_funcptr.c", 4
    lda _add__param_b
    ldx _add__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _add__param_a
    ldx _add__param_a+1
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

; function _sub
; SAC inline storage: 4 bytes
    .global _sub__param_a
    _sub__param_a: .word 0
    .global _sub__param_b
    _sub__param_b: .word 0
    _sub__local_0: .word 0
    _sub__local_1: .word 0
    proc _sub, W#@_p_a, W#@_p_b
    .sac
    .var _fp = 0
    .loc "test_funcptr.c", 7
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __sub @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __sub @_p_b offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_funcptr.c", 8
    lda _sub__param_b
    ldx _sub__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _sub__param_a
    ldx _sub__param_a+1
    sub.16 .AX, __zp_scratch2
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

; function _apply
; SAC inline storage: 6 bytes
    .global _apply__param_op
    _apply__param_op: .word 0
    .global _apply__param_x
    _apply__param_x: .word 0
    .global _apply__param_y
    _apply__param_y: .word 0
    _apply__local_0: .word 0
    _apply__local_1: .word 0
    _apply__local_2: .word 0
    proc _apply, W#@_p_op, W#@_p_x, W#@_p_y
    .sac
    .var _fp = 0
    .loc "test_funcptr.c", 11
    .var @_p_op = 2
    .var @_p_x = 4
    .var @_p_y = 6
; .debug_var: __apply @_p_op offset=2 size=2 type=ptr scope=parameter
; .debug_var: __apply @_p_x offset=4 size=2 type=int16 scope=parameter
; .debug_var: __apply @_p_y offset=6 size=2 type=int16 scope=parameter

@entry:
    .loc "test_funcptr.c", 12
    lda _apply__param_y
    ldx _apply__param_y+1
    sta $28
    stx $29
    lda _apply__param_x
    ldx _apply__param_x+1
    sta $2A
    stx $2B
    lda $28
    ldx $29
    push .ax
    lda $2A
    ldx $2B
    push .ax
    lda _apply__param_op
    ldx _apply__param_op+1
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

; function _apply2
; SAC inline storage: 6 bytes
    .global _apply2__param_op
    _apply2__param_op: .word 0
    .global _apply2__param_x
    _apply2__param_x: .word 0
    .global _apply2__param_y
    _apply2__param_y: .word 0
    _apply2__local_0: .word 0
    _apply2__local_1: .word 0
    _apply2__local_2: .word 0
    proc _apply2, W#@_p_op, W#@_p_x, W#@_p_y
    .sac
    .var _fp = 0
    .loc "test_funcptr.c", 17
    .var @_p_op = 2
    .var @_p_x = 4
    .var @_p_y = 6
; .debug_var: __apply2 @_p_op offset=2 size=2 type=ptr scope=parameter
; .debug_var: __apply2 @_p_x offset=4 size=2 type=int16 scope=parameter
; .debug_var: __apply2 @_p_y offset=6 size=2 type=int16 scope=parameter

@entry:
    .loc "test_funcptr.c", 18
    lda _apply2__param_y
    ldx _apply2__param_y+1
    sta $28
    stx $29
    lda _apply2__param_x
    ldx _apply2__param_x+1
    sta $2A
    stx $2B
    lda $28
    ldx $29
    push .ax
    lda $2A
    ldx $2B
    push .ax
    lda _apply2__param_op
    ldx _apply2__param_op+1
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
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _main
; SAC inline storage: 22 bytes
    _main__local_0: .word 0
    _main__local_1: .word 0
    _main__local_2: .word 0
    _main__local_3: .word 0
    _main__local_5: .word 0
    _main__local_6: .word 0
    _main__local_7: .word 0
    _main__local_8: .word 0
    _main__local_10: .word 0
    _main__local_11: .word 0
    _main__local_12: .word 0
    _main__local_14: .word 0
    _main__local_15: .word 0
    _main__local_16: .word 0
    _main__local_22: .word 0
    _main__local_23: .word 0
    _main__local_24: .word 0
    _main__local_30: .word 0
    _main__local_31: .word 0
    _main__local_32: .word 0
    _main__local_38: .word 0
    _main__local_40: .word 0
    _main__local_41: .word 0
    _main__local_42: .word 0
    _main__local_44: .word 0
    _main__local_45: .word 0
    _main__local_46: .word 0
    _main__local_47: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_funcptr.c", 21
    .local @_l_fp = 0
    .local @_l_fp2 = 4
    .local @_l_fp3 = 16
    .local @_l_r1 = 2
    .local @_l_r2 = 6
    .local @_l_r3 = 8
    .local @_l_r4 = 10
    .local @_l_r5 = 12
    .local @_l_r6 = 14
    .local @_l_r7 = 18
    .local @_l_r8 = 20
; .debug_var: __main @_l_fp offset=0 size=2 type=ptr scope=local
; .debug_var: __main @_l_fp2 offset=4 size=2 type=ptr scope=local
; .debug_var: __main @_l_fp3 offset=16 size=2 type=ptr scope=local
; .debug_var: __main @_l_r1 offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_r2 offset=6 size=2 type=int16 scope=local
; .debug_var: __main @_l_r3 offset=8 size=2 type=int16 scope=local
; .debug_var: __main @_l_r4 offset=10 size=2 type=int16 scope=local
; .debug_var: __main @_l_r5 offset=12 size=2 type=int16 scope=local
; .debug_var: __main @_l_r6 offset=14 size=2 type=int16 scope=local
; .debug_var: __main @_l_r7 offset=18 size=2 type=int16 scope=local
; .debug_var: __main @_l_r8 offset=20 size=2 type=int16 scope=local

@entry:
    .loc "test_funcptr.c", 23
    ldax #_add
    sta _main__local_0
    stx _main__local_0+1
    .loc "test_funcptr.c", 24
    lda #3
    ldx #0
    sta _main__local_2
    stx _main__local_2+1
    lda #4
    ldx #0
    sta _main__local_3
    stx _main__local_3+1
    lda #4
    ldx #0
    push .ax
    lda #3
    ldx #0
    push .ax
    lda _main__local_0
    ldx _main__local_0+1
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
    sta _main__local_1
    stx _main__local_1+1
    .loc "test_funcptr.c", 27
    ldax #_sub
    sta _main__local_5
    stx _main__local_5+1
    .loc "test_funcptr.c", 28
    lda #10
    ldx #0
    sta _main__local_7
    stx _main__local_7+1
    lda #3
    ldx #0
    sta _main__local_8
    stx _main__local_8+1
    lda #3
    ldx #0
    push .ax
    lda #10
    ldx #0
    push .ax
    lda _main__local_5
    ldx _main__local_5+1
    sta @__call_site_3+1
    stx @__call_site_3+2
@__call_site_3:
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
    .loc "test_funcptr.c", 31
    ldax #_add
    sta _main__local_5
    stx _main__local_5+1
    .loc "test_funcptr.c", 32
    lda #10
    ldx #0
    sta _main__local_11
    stx _main__local_11+1
    lda #3
    ldx #0
    sta _main__local_12
    stx _main__local_12+1
    lda #3
    ldx #0
    push .ax
    lda #10
    ldx #0
    push .ax
    lda _main__local_5
    ldx _main__local_5+1
    sta @__call_site_4+1
    stx @__call_site_4+2
@__call_site_4:
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
    sta _main__local_10
    stx _main__local_10+1
    .loc "test_funcptr.c", 35
    lda #5
    ldx #0
    sta _main__local_15
    stx _main__local_15+1
    lda #6
    ldx #0
    sta _main__local_16
    stx _main__local_16+1
    ldax #_add
    sta $20
    stx $21
    lda _main__local_15
    ldx _main__local_15+1
    sta $20
    stx $21
    lda _main__local_16
    ldx _main__local_16+1
    sta $20
    stx $21
    .loc "test_funcptr.c", 12
    lda #6
    ldx #0
    push .ax
    lda #5
    ldx #0
    push .ax
    ldax #_add
    sta @__call_site_5+1
    stx @__call_site_5+2
@__call_site_5:
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
@inline_end0:
    .loc "test_funcptr.c", 35
    lda $22
    ldx $23
    sta _main__local_14
    stx _main__local_14+1
    .loc "test_funcptr.c", 36
    lda #10
    ldx #0
    sta _main__local_23
    stx _main__local_23+1
    lda #3
    ldx #0
    sta _main__local_24
    stx _main__local_24+1
    ldax #_sub
    sta $20
    stx $21
    lda _main__local_23
    ldx _main__local_23+1
    sta $20
    stx $21
    lda _main__local_24
    ldx _main__local_24+1
    sta $20
    stx $21
    .loc "test_funcptr.c", 12
    lda #3
    ldx #0
    push .ax
    lda #10
    ldx #0
    push .ax
    ldax #_sub
    sta @__call_site_6+1
    stx @__call_site_6+2
@__call_site_6:
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
@inline_end2:
    .loc "test_funcptr.c", 36
    lda $22
    ldx $23
    sta _main__local_22
    stx _main__local_22+1
    .loc "test_funcptr.c", 39
    lda #8
    ldx #0
    sta _main__local_31
    stx _main__local_31+1
    lda #2
    ldx #0
    sta _main__local_32
    stx _main__local_32+1
    ldax #_add
    sta $20
    stx $21
    lda _main__local_31
    ldx _main__local_31+1
    sta $20
    stx $21
    lda _main__local_32
    ldx _main__local_32+1
    sta $20
    stx $21
    .loc "test_funcptr.c", 18
    lda #2
    ldx #0
    push .ax
    lda #8
    ldx #0
    push .ax
    ldax #_add
    sta @__call_site_7+1
    stx @__call_site_7+2
@__call_site_7:
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
@inline_end4:
    .loc "test_funcptr.c", 39
    lda $22
    ldx $23
    sta _main__local_30
    stx _main__local_30+1
    .loc "test_funcptr.c", 42
    ldax #_sub
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_38
    stx _main__local_38+1
    .loc "test_funcptr.c", 43
    lda #20
    ldx #0
    sta _main__local_41
    stx _main__local_41+1
    lda #5
    ldx #0
    sta _main__local_42
    stx _main__local_42+1
    lda #5
    ldx #0
    push .ax
    lda #20
    ldx #0
    push .ax
    lda _main__local_38
    ldx _main__local_38+1
    sta @__call_site_8+1
    stx @__call_site_8+2
@__call_site_8:
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
    sta _main__local_40
    stx _main__local_40+1
    .loc "test_funcptr.c", 46
    lda #2
    ldx #0
    sta _main__local_45
    stx _main__local_45+1
    lda #3
    ldx #0
    sta _main__local_46
    stx _main__local_46+1
    lda _main__local_0
    ldx _main__local_0+1
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta _main__local_47
    stx _main__local_47+1
    lda #3
    ldx #0
    push .ax
    lda #2
    ldx #0
    push .ax
    lda _main__local_47
    ldx _main__local_47+1
    sta @__call_site_9+1
    stx @__call_site_9+2
@__call_site_9:
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
    sta _main__local_44
    stx _main__local_44+1
    .loc "test_funcptr.c", 48
    lda _main__local_6
    ldx _main__local_6+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_1
    ldx _main__local_1+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda _main__local_10
    ldx _main__local_10+1
    add.16 .AX, $20
    sta $22
    stx $23
    lda _main__local_14
    ldx _main__local_14+1
    add.16 .AX, $22
    sta $20
    stx $21
    lda _main__local_22
    ldx _main__local_22+1
    add.16 .AX, $20
    sta $22
    stx $23
    lda _main__local_30
    ldx _main__local_30+1
    add.16 .AX, $22
    sta $20
    stx $21
    lda _main__local_40
    ldx _main__local_40+1
    add.16 .AX, $20
    sta $22
    stx $23
    lda _main__local_44
    ldx _main__local_44+1
    add.16 .AX, $22
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 22
    endproc


__zp_save_buf:

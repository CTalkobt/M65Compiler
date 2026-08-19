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
    .global _sum
    .global _double_it
    .global _call_variadic_with_live_params
    .global _mixed_calls
    .global _max_of
    .global _main

    .segment "data"
    .byte 0
_results:
; .debug_var: @global _results offset=0 size=2 type=ptr scope=global
    .word 16384

    .segment "code"

; function _sum
    proc _sum, W#@_p_count
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdarg.h", 6
; frame: 8 bytes (frame-allocated vRegs only)
    phw #0
    phw #0
    phw #0
    phw #0
    tsy
    tsx
    inx
    bne @__fp_no_carry_0
    iny
@__fp_no_carry_0:
    stx $FD
    sty $FE
    .frameptr_zp $FD
    leax.local 0
    sta $10
    stx $10+1
    .local __vr0 = 0
    .local __vr1 = 2
    .local __vr3 = 4
    .local __vr5 = 6
    .local @_l_ap = 2
    .local @_l_i = 6
    .local @_l_total = 4
; .debug_var: __sum @_l_ap offset=2 size=2 type=int16 scope=local
; .debug_var: __sum @_l_i offset=6 size=2 type=int16 scope=local
; .debug_var: __sum @_l_total offset=4 size=2 type=int16 scope=local
    .var @_p_count = 10
; .debug_var: __sum @_p_count offset=10 size=2 type=int16 scope=parameter

    ldax.param @_p_count
    stax.local __vr0
@entry:
    .loc "test_zpcall_mixed.c", 12
    leax.param @_p_count
    add.16 .AX, #2
    sta $20
    stx $21
    lda $20
    ldx $21
    stax.local __vr1
    .loc "test_zpcall_mixed.c", 13
    lda #0
    taz
    staz.local __vr3
    .loc "test_zpcall_mixed.c", 15
    lda #0
    taz
    staz.local __vr5
@for_cond0:
    ldax.local __vr0
    sta __zp_scratch2
    stx __zp_scratch2+1
    ldax.local __vr5
    cmp.16 .AX, __zp_scratch2
    bcc @for_body1
    bra @for_end3
@for_body1:
    .loc "test_zpcall_mixed.c", 16
    ldax.local __vr1
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
    ldax.local __vr1
    add.16 .AX, #2
    sta $24
    stx $25
    stax.local __vr1
    ldax.local __vr3
    add.16 .AX, $22
    sta $26
    stx $27
    stax.local __vr3
@for_inc2:
    .loc "test_zpcall_mixed.c", 15
    ldax.local __vr5
    sta $28
    stx $29
    lda $28
    clc
    adc #1
    sta $2A
    lda $29
    adc #0
    sta $2B
    lda $2A
    ldx $2B
    stax.local __vr5
    bra @for_cond0
@for_end3:
    .loc "test_zpcall_mixed.c", 18
    .loc "test_zpcall_mixed.c", 19
    ldax.local __vr3
@__return:
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    rts
    .func_flags stack_call, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 8
    endproc

; function _double_it
; SAC inline storage: 2 bytes
    .global _double_it__param_x
    _double_it__param_x: .word 0
    _double_it__local_0: .word 0
    proc _double_it, W#@_p_x
    .sac
    .var _fp = 0
    .loc "test_zpcall_mixed.c", 12
    .var @_p_x = 2
; .debug_var: __double_it @_p_x offset=2 size=2 type=int16 scope=parameter

@entry:
    .loc "test_zpcall_mixed.c", 24
    lda _double_it__param_x
    ldx _double_it__param_x+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _double_it__param_x
    ldx _double_it__param_x+1
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
    .frame_size 2
    endproc

; function _call_variadic_with_live_params
; SAC inline storage: 6 bytes
    .global _call_variadic_with_live_params__param_a
    _call_variadic_with_live_params__param_a: .word 0
    .global _call_variadic_with_live_params__param_b
    _call_variadic_with_live_params__param_b: .word 0
    _call_variadic_with_live_params__local_0: .word 0
    _call_variadic_with_live_params__local_1: .word 0
    _call_variadic_with_live_params__local_2: .word 0
    _call_variadic_with_live_params__local_3: .word 0
    _call_variadic_with_live_params__local_4: .word 0
    _call_variadic_with_live_params__local_5: .word 0
    _call_variadic_with_live_params__local_6: .word 0
    proc _call_variadic_with_live_params, W#@_p_a, W#@_p_b
    .sac
    .var _fp = 0
    .loc "test_zpcall_mixed.c", 17
    .local @_l_s = 4
; .debug_var: __call_variadic_with_live_params @_l_s offset=4 size=2 type=int16 scope=local
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __call_variadic_with_live_params @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __call_variadic_with_live_params @_p_b offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_zpcall_mixed.c", 29
    lda #30
    ldx #0
    push .ax
    lda #20
    ldx #0
    push .ax
    lda #10
    ldx #0
    push .ax
    lda #3
    ldx #0
    push .ax
    jsr _sum
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _call_variadic_with_live_params__local_2
    stx _call_variadic_with_live_params__local_2+1
    .loc "test_zpcall_mixed.c", 30
    lda _call_variadic_with_live_params__param_b
    ldx _call_variadic_with_live_params__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _call_variadic_with_live_params__param_a
    ldx _call_variadic_with_live_params__param_a+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda _call_variadic_with_live_params__local_2
    ldx _call_variadic_with_live_params__local_2+1
    add.16 .AX, $20
    sta $22
    stx $23
    lda $22
    ldx $23
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _mixed_calls
; SAC inline storage: 6 bytes
    .global _mixed_calls__param_x
    _mixed_calls__param_x: .word 0
    _mixed_calls__local_0: .word 0
    _mixed_calls__local_1: .word 0
    _mixed_calls__local_5: .word 0
    _mixed_calls__local_6: .word 0
    _mixed_calls__local_7: .word 0
    proc _mixed_calls, W#@_p_x
    .sac
    .var _fp = 0
    .loc "test_zpcall_mixed.c", 23
    .local @_l_d = 2
    .local @_l_s = 4
; .debug_var: __mixed_calls @_l_d offset=2 size=2 type=int16 scope=local
; .debug_var: __mixed_calls @_l_s offset=4 size=2 type=int16 scope=local
    .var @_p_x = 2
; .debug_var: __mixed_calls @_p_x offset=2 size=2 type=int16 scope=parameter

@entry:
    .loc "test_zpcall_mixed.c", 35
    lda _mixed_calls__param_x
    ldx _mixed_calls__param_x+1
    sta $20
    stx $21
    .loc "test_zpcall_mixed.c", 24
    lda _mixed_calls__param_x
    ldx _mixed_calls__param_x+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _mixed_calls__param_x
    ldx _mixed_calls__param_x+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end4:
    .loc "test_zpcall_mixed.c", 35
    lda $22
    ldx $23
    sta _mixed_calls__local_1
    stx _mixed_calls__local_1+1
    .loc "test_zpcall_mixed.c", 36
    lda #2
    ldx #0
    sta _mixed_calls__local_6
    stx _mixed_calls__local_6+1
    lda #100
    ldx #0
    sta _mixed_calls__local_7
    stx _mixed_calls__local_7+1
    lda _mixed_calls__local_7
    ldx _mixed_calls__local_7+1
    sta $28
    stx $29
    lda _mixed_calls__local_1
    ldx _mixed_calls__local_1+1
    sta $2A
    stx $2B
    lda _mixed_calls__local_6
    ldx _mixed_calls__local_6+1
    sta $2C
    stx $2D
    lda $28
    ldx $29
    push .ax
    lda $2A
    ldx $2B
    push .ax
    lda $2C
    ldx $2D
    push .ax
    jsr _sum
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _mixed_calls__local_5
    stx _mixed_calls__local_5+1
    .loc "test_zpcall_mixed.c", 37
    lda _mixed_calls__local_5
    ldx _mixed_calls__local_5+1
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _max_of
    proc _max_of, W#@_p_count
    .var _fp = 0
    .loc "test_zpcall_mixed.c", 30
; frame: 10 bytes (frame-allocated vRegs only)
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    tsy
    tsx
    inx
    bne @__fp_no_carry_1
    iny
@__fp_no_carry_1:
    stx $FD
    sty $FE
    .frameptr_zp $FD
    leax.local 0
    sta $12
    stx $12+1
    .local __vr0 = 0
    .local __vr1 = 2
    .local __vr3 = 4
    .local __vr6 = 6
    .local __vr9 = 8
    .local @_l_ap = 2
    .local @_l_best = 4
    .local @_l_i = 6
    .local @_l_v = 8
; .debug_var: __max_of @_l_ap offset=2 size=2 type=int16 scope=local
; .debug_var: __max_of @_l_best offset=4 size=2 type=int16 scope=local
; .debug_var: __max_of @_l_i offset=6 size=2 type=int16 scope=local
; .debug_var: __max_of @_l_v offset=8 size=2 type=int16 scope=local
    .var @_p_count = 12
; .debug_var: __max_of @_p_count offset=12 size=2 type=int16 scope=parameter

    ldax.param @_p_count
    stax.local __vr0
@entry:
    .loc "test_zpcall_mixed.c", 43
    leax.param @_p_count
    add.16 .AX, #2
    sta $20
    stx $21
    lda $20
    ldx $21
    stax.local __vr1
    .loc "test_zpcall_mixed.c", 44
    ldax.local __vr1
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $20
    stx $21
    ldax.local __vr1
    add.16 .AX, #2
    sta $22
    stx $23
    stax.local __vr1
    lda $20
    ldx $21
    stax.local __vr3
    .loc "test_zpcall_mixed.c", 46
    lda #1
    ldz #0
    staz.local __vr6
@for_cond6:
    ldax.local __vr0
    sta __zp_scratch2
    stx __zp_scratch2+1
    ldax.local __vr6
    cmp.16 .AX, __zp_scratch2
    bcc @for_body7
    bra @for_end9
@for_body7:
    .loc "test_zpcall_mixed.c", 47
    ldax.local __vr1
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
    ldax.local __vr1
    add.16 .AX, #2
    sta $24
    stx $25
    stax.local __vr1
    lda $22
    ldx $23
    stax.local __vr9
    .loc "test_zpcall_mixed.c", 48
    ldax.local __vr3
    sta __zp_scratch2
    stx __zp_scratch2+1
    ldax.local __vr9
    cmp.16 .AX, __zp_scratch2
    beq @if_end12
    bcs @if_then10
    bra @if_end12
@if_then10:
    ldax.local __vr9
    stax.local __vr3
@if_end12:
@for_inc8:
    .loc "test_zpcall_mixed.c", 46
    ldax.local __vr6
    sta $28
    stx $29
    lda $28
    clc
    adc #1
    sta $2A
    lda $29
    adc #0
    sta $2B
    lda $2A
    ldx $2B
    stax.local __vr6
    bra @for_cond6
@for_end9:
    .loc "test_zpcall_mixed.c", 50
    .loc "test_zpcall_mixed.c", 51
    ldax.local __vr3
@__return:
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    rts
    .func_flags stack_call, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 10
    endproc

; function _main
; SAC inline storage: 16 bytes
    _main__local_0: .word 0
    _main__local_1: .word 0
    _main__local_2: .word 0
    _main__local_3: .word 0
    _main__local_4: .word 0
    _main__local_19: .word 0
    _main__local_20: .word 0
    _main__local_21: .word 0
    _main__local_26: .word 0
    _main__local_27: .word 0
    _main__local_28: .word 0
    _main__local_29: .word 0
    _main__local_47: .word 0
    _main__local_51: .word 0
    _main__local_56: .word 0
    _main__local_57: .word 0
    _main__local_58: .word 0
    _main__local_74: .word 0
    _main__local_75: .word 0
    _main__local_76: .word 0
    _main__local_77: .word 0
    _main__local_78: .word 0
    _main__local_79: .word 0
    _main__local_87: .word 0
    _main__local_88: .word 0
    _main__local_89: .word 0
    _main__local_90: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_zpcall_mixed.c", 43
    .local @_l_d = 8
    .local @_l_r1 = 0
    .local @_l_r2 = 2
    .local @_l_r3 = 6
    .local @_l_r4 = 12
    .local @_l_r5 = 14
    .local @_l_s = 10
; .debug_var: __main @_l_d offset=8 size=2 type=int16 scope=local
; .debug_var: __main @_l_r1 offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_r2 offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_r3 offset=6 size=2 type=int16 scope=local
; .debug_var: __main @_l_r4 offset=12 size=2 type=int16 scope=local
; .debug_var: __main @_l_r5 offset=14 size=2 type=int16 scope=local
; .debug_var: __main @_l_s offset=10 size=2 type=int16 scope=local

@entry:
    .loc "test_zpcall_mixed.c", 57
    lda #30
    ldx #0
    push .ax
    lda #20
    ldx #0
    push .ax
    lda #10
    ldx #0
    push .ax
    lda #3
    ldx #0
    push .ax
    jsr _sum
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_0
    stx _main__local_0+1
    .loc "test_zpcall_mixed.c", 58
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
    .loc "test_zpcall_mixed.c", 59
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
    .loc "test_zpcall_mixed.c", 63
    lda #5
    ldx #0
    sta _main__local_20
    stx _main__local_20+1
    lda #7
    ldx #0
    sta _main__local_21
    stx _main__local_21+1
    lda _main__local_20
    ldx _main__local_20+1
    sta $20
    stx $21
    lda _main__local_21
    ldx _main__local_21+1
    sta $20
    stx $21
    .loc "test_zpcall_mixed.c", 29
    lda #30
    ldx #0
    push .ax
    lda #20
    ldx #0
    push .ax
    lda #10
    ldx #0
    push .ax
    lda #3
    ldx #0
    push .ax
    jsr _sum
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_zpcall_mixed.c", 30
    lda _main__local_21
    ldx _main__local_21+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_20
    ldx _main__local_20+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    clc
    adc $22
    sta $24
    lda $21
    adc $22+1
    sta $25
    lda $24
    ldx $25
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end13:
    .loc "test_zpcall_mixed.c", 63
    lda $22
    ldx $23
    sta _main__local_19
    stx _main__local_19+1
    .loc "test_zpcall_mixed.c", 64
    lda #255
    ldx #0
    sta $20
    stx $21
    lda _main__local_19
    ldx _main__local_19+1
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
    .loc "test_zpcall_mixed.c", 65
    lda _main__local_19
    ldx _main__local_19+1
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
    .loc "test_zpcall_mixed.c", 69
    lda #25
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_zpcall_mixed.c", 35
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_zpcall_mixed.c", 24
    lda $20
    clc
    adc #25
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
@inline_end16:
    .loc "test_zpcall_mixed.c", 35
    lda $22
    ldx $23
    sta _main__local_51
    stx _main__local_51+1
    .loc "test_zpcall_mixed.c", 36
    lda #2
    ldx #0
    sta _main__local_57
    stx _main__local_57+1
    lda #100
    ldx #0
    sta _main__local_58
    stx _main__local_58+1
    lda _main__local_58
    ldx _main__local_58+1
    sta $28
    stx $29
    lda _main__local_51
    ldx _main__local_51+1
    sta $2A
    stx $2B
    lda _main__local_57
    ldx _main__local_57+1
    sta $2C
    stx $2D
    lda $28
    ldx $29
    push .ax
    lda $2A
    ldx $2B
    push .ax
    lda $2C
    ldx $2D
    push .ax
    jsr _sum
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_56
    stx _main__local_56+1
    lda _main__local_56
    ldx _main__local_56+1
    sta $20
    stx $21
    .loc "test_zpcall_mixed.c", 37
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end15:
    .loc "test_zpcall_mixed.c", 69
    lda $22
    ldx $23
    sta _main__local_47
    stx _main__local_47+1
    .loc "test_zpcall_mixed.c", 70
    lda #255
    ldx #0
    sta $20
    stx $21
    lda _main__local_47
    ldx _main__local_47+1
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
    lda #4
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
    .loc "test_zpcall_mixed.c", 71
    lda _main__local_47
    ldx _main__local_47+1
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
    lda #5
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
    .loc "test_zpcall_mixed.c", 75
    lda #42
    ldx #0
    push .ax
    lda #3
    ldx #0
    push .ax
    lda #99
    ldx #0
    push .ax
    lda #5
    ldx #0
    push .ax
    lda #4
    ldx #0
    push .ax
    jsr _max_of
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    plz
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_74
    stx _main__local_74+1
    .loc "test_zpcall_mixed.c", 76
    lda #255
    ldx #0
    sta $20
    stx $21
    lda _main__local_74
    ldx _main__local_74+1
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
    lda #6
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
    .loc "test_zpcall_mixed.c", 80
    lda #25
    ldx #0
    push .ax
    lda #15
    ldx #0
    push .ax
    lda #2
    ldx #0
    push .ax
    jsr _sum
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_zpcall_mixed.c", 24
    lda $20
    clc
    adc $20
    sta $22
    lda $21
    adc $20+1
    sta $23
    lda $22
    ldx $23
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end19:
    .loc "test_zpcall_mixed.c", 80
    lda $22
    ldx $23
    sta _main__local_87
    stx _main__local_87+1
    .loc "test_zpcall_mixed.c", 81
    lda #255
    ldx #0
    sta $20
    stx $21
    lda _main__local_87
    ldx _main__local_87+1
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
    lda #7
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
    .loc "test_zpcall_mixed.c", 84
    lda #170
    sta $20
    lda _results
    ldx _results+1
    sta $22
    stx $23
    lda #8
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
    .loc "test_zpcall_mixed.c", 86
    brk
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 16
    endproc


__zp_save_buf:

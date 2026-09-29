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

    .global _sum
    .global _max_of
    .global _count_above
    .global _no_varargs
    .global _main

    .segment "code"

; function _sum
    proc _sum, W#@_p_count
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdarg.h", 4
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
    .loc "test_variadic.c", 7
    leax.param @_p_count
    add.16 .AX, #2
    sta $20
    stx $21
    lda $20
    ldx $21
    stax.local __vr1
    .loc "test_variadic.c", 8
    lda #0
    taz
    staz.local __vr3
    .loc "test_variadic.c", 10
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
    .loc "test_variadic.c", 11
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
    .loc "test_variadic.c", 10
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
    .loc "test_variadic.c", 13
    .loc "test_variadic.c", 14
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

; function _max_of
    proc _max_of, W#@_p_count
    .var _fp = 0
    .loc "test_variadic.c", 7
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
    .loc "test_variadic.c", 20
    leax.param @_p_count
    add.16 .AX, #2
    sta $20
    stx $21
    lda $20
    ldx $21
    stax.local __vr1
    .loc "test_variadic.c", 21
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
    .loc "test_variadic.c", 23
    lda #1
    ldz #0
    staz.local __vr6
@for_cond4:
    ldax.local __vr0
    sta __zp_scratch2
    stx __zp_scratch2+1
    ldax.local __vr6
    cmp.16 .AX, __zp_scratch2
    bcc @for_body5
    bra @for_end7
@for_body5:
    .loc "test_variadic.c", 24
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
    .loc "test_variadic.c", 25
    ldax.local __vr3
    sta __zp_scratch2
    stx __zp_scratch2+1
    ldax.local __vr9
    cmp.16 .AX, __zp_scratch2
    beq @if_end10
    bcs @if_then8
    bra @if_end10
@if_then8:
    ldax.local __vr9
    stax.local __vr3
@if_end10:
@for_inc6:
    .loc "test_variadic.c", 23
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
    bra @for_cond4
@for_end7:
    .loc "test_variadic.c", 27
    .loc "test_variadic.c", 28
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

; function _count_above
    proc _count_above, W#@_p_label, W#@_p_threshold, W#@_p_count
    .var _fp = 0
    .loc "test_variadic.c", 21
; frame: 14 bytes (frame-allocated vRegs only)
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    tsy
    tsx
    inx
    bne @__fp_no_carry_2
    iny
@__fp_no_carry_2:
    stx $FD
    sty $FE
    .frameptr_zp $FD
    leax.local 0
    sta $14
    stx $14+1
    .local __vr0 = 0
    .local __vr1 = 2
    .local __vr2 = 4
    .local __vr3 = 6
    .local __vr5 = 8
    .local __vr7 = 10
    .local __vr10 = 12
    .local @_l_ap = 6
    .local @_l_i = 10
    .local @_l_n = 8
    .local @_l_v = 12
; .debug_var: __count_above @_l_ap offset=6 size=2 type=int16 scope=local
; .debug_var: __count_above @_l_i offset=10 size=2 type=int16 scope=local
; .debug_var: __count_above @_l_n offset=8 size=2 type=int16 scope=local
; .debug_var: __count_above @_l_v offset=12 size=2 type=int16 scope=local
    .var @_p_label = 16
    .var @_p_threshold = 18
    .var @_p_count = 20
; .debug_var: __count_above @_p_label offset=16 size=2 type=ptr scope=parameter
; .debug_var: __count_above @_p_threshold offset=18 size=2 type=int16 scope=parameter
; .debug_var: __count_above @_p_count offset=20 size=2 type=int16 scope=parameter

    ldax.param @_p_label
    stax.local __vr0
    ldax.param @_p_threshold
    stax.local __vr1
    ldax.param @_p_count
    stax.local __vr2
@entry:
    .loc "test_variadic.c", 34
    leax.param @_p_count
    add.16 .AX, #2
    sta $20
    stx $21
    lda $20
    ldx $21
    stax.local __vr3
    .loc "test_variadic.c", 35
    lda #0
    taz
    staz.local __vr5
    .loc "test_variadic.c", 37
    lda #0
    taz
    staz.local __vr7
@for_cond11:
    ldax.local __vr2
    sta __zp_scratch2
    stx __zp_scratch2+1
    ldax.local __vr7
    cmp.16 .AX, __zp_scratch2
    bcc @for_body12
    bra @for_end14
@for_body12:
    .loc "test_variadic.c", 38
    ldax.local __vr3
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
    ldax.local __vr3
    add.16 .AX, #2
    sta $24
    stx $25
    stax.local __vr3
    lda $22
    ldx $23
    stax.local __vr10
    .loc "test_variadic.c", 39
    ldax.local __vr1
    sta __zp_scratch2
    stx __zp_scratch2+1
    ldax.local __vr10
    cmp.16 .AX, __zp_scratch2
    beq @if_end17
    bcs @if_then15
    bra @if_end17
@if_then15:
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
@if_end17:
@for_inc13:
    .loc "test_variadic.c", 37
    ldax.local __vr7
    sta $2C
    stx $2D
    lda $2C
    clc
    adc #1
    sta $2E
    lda $2D
    adc #0
    sta $2F
    lda $2E
    ldx $2F
    stax.local __vr7
    bra @for_cond11
@for_end14:
    .loc "test_variadic.c", 41
    .loc "test_variadic.c", 42
    ldax.local __vr5
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
    plz
    plz
    plz
    plz
    rts
    .func_flags stack_call, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 14
    endproc

; function _no_varargs
    proc _no_varargs, W#@_p_a
    .var _fp = 0
    .loc "test_variadic.c", 35
; frame: 2 bytes (frame-allocated vRegs only)
    phw #0
    tsy
    tsx
    inx
    bne @__fp_no_carry_3
    iny
@__fp_no_carry_3:
    stx $FD
    sty $FE
    .frameptr_zp $FD
    leax.local 0
    sta $16
    stx $16+1
    .local __vr0 = 0
    .var @_p_a = 4
; .debug_var: __no_varargs @_p_a offset=4 size=2 type=int16 scope=parameter

    ldax.param @_p_a
    stax.local __vr0
@entry:
    .loc "test_variadic.c", 47
    lda #2
    ldx #0
    sta $20
    stx $21
    ldax.local __vr0
    lsl.16 .AX
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    plz
    plz
    rts
    .func_flags stack_call, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z
    .frame_size 2
    endproc

; function _main
; SAC inline storage: 4 bytes
    _main__local_0: .word 0
    _main__local_2: .word 0
    _main__local_3: .word 0
    _main__local_4: .word 0
    _main__local_5: .word 0
    _main__local_11: .word 0
    _main__local_12: .word 0
    _main__local_18: .word 0
    _main__local_24: .word 0
    _main__local_25: .word 0
    _main__local_26: .word 0
    _main__local_27: .word 0
    _main__local_28: .word 0
    _main__local_29: .word 0
    _main__local_35: .word 0
    _main__local_36: .word 0
    _main__local_37: .word 0
    _main__local_38: .word 0
    _main__local_39: .word 0
    _main__local_45: .word 0
    _main__local_46: .word 0
    _main__local_52: .word 0
    _main__local_53: .word 0
    _main__local_54: .word 0
    _main__local_55: .word 0
    _main__local_56: .word 0
    _main__local_57: .word 0
    _main__local_58: .word 0
    _main__local_64: .word 0
    _main__local_70: .word 0
    _main__local_71: .word 0
    _main__local_72: .word 0
    _main__local_78: .word 0
    _main__local_79: .word 0
    _main__local_80: .word 0
    _main__local_81: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_variadic.c", 42
    .local @_l_fail = 0
    .local @_l_r = 2
; .debug_var: __main @_l_fail offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_r offset=2 size=2 type=int16 scope=local

@entry:
    .loc "test_variadic.c", 54
    lda #0
    sta _main__local_0
    sta _main__local_0+1
    .loc "test_variadic.c", 57
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
    cmp.16 .AX, #60
    bne @if_then18
    bra @if_end20
@if_then18:
    lda _main__local_0
    ldx _main__local_0+1
    sta $20
    stx $21
    lda $20
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta _main__local_0
    stx _main__local_0+1
@if_end20:
    .loc "test_variadic.c", 60
    lda #42
    ldx #0
    push .ax
    lda #1
    ldx #0
    push .ax
    jsr _sum
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #42
    bne @if_then21
    bra @if_end23
@if_then21:
    lda _main__local_0
    ldx _main__local_0+1
    sta $20
    stx $21
    lda $20
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta _main__local_0
    stx _main__local_0+1
@if_end23:
    .loc "test_variadic.c", 63
    lda #0
    ldx #0
    push .ax
    jsr _sum
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    sta $20
    stx $21
    lda $20
    ora $21
    bne @if_then24
    bra @if_end26
@if_then24:
    lda _main__local_0
    ldx _main__local_0+1
    sta $20
    stx $21
    lda $20
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta _main__local_0
    stx _main__local_0+1
@if_end26:
    .loc "test_variadic.c", 66
    lda #5
    ldx #0
    push .ax
    lda #4
    ldx #0
    push .ax
    lda #3
    ldx #0
    push .ax
    lda #2
    ldx #0
    push .ax
    lda #1
    ldx #0
    push .ax
    lda #5
    ldx #0
    push .ax
    jsr _sum
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
    cmp.16 .AX, #15
    bne @if_then27
    bra @if_end29
@if_then27:
    lda _main__local_0
    ldx _main__local_0+1
    sta $20
    stx $21
    lda $20
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta _main__local_0
    stx _main__local_0+1
@if_end29:
    .loc "test_variadic.c", 69
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
    cmp.16 .AX, #99
    bne @if_then30
    bra @if_end32
@if_then30:
    lda _main__local_0
    ldx _main__local_0+1
    sta $20
    stx $21
    lda $20
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta _main__local_0
    stx _main__local_0+1
@if_end32:
    .loc "test_variadic.c", 72
    lda #77
    ldx #0
    push .ax
    lda #1
    ldx #0
    push .ax
    jsr _max_of
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #77
    bne @if_then33
    bra @if_end35
@if_then33:
    lda _main__local_0
    ldx _main__local_0+1
    sta $20
    stx $21
    lda $20
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta _main__local_0
    stx _main__local_0+1
@if_end35:
    .loc "test_variadic.c", 75
    ldax #__str_39
    sta _main__local_52
    stx _main__local_52+1
    lda #50
    ldx #0
    sta _main__local_53
    stx _main__local_53+1
    lda #4
    ldx #0
    sta _main__local_54
    stx _main__local_54+1
    lda #10
    ldx #0
    sta _main__local_55
    stx _main__local_55+1
    lda #60
    ldx #0
    sta _main__local_56
    stx _main__local_56+1
    lda #30
    ldx #0
    sta _main__local_57
    stx _main__local_57+1
    lda #90
    ldx #0
    sta _main__local_58
    stx _main__local_58+1
    lda _main__local_58
    ldx _main__local_58+1
    sta $28
    stx $29
    lda _main__local_57
    ldx _main__local_57+1
    sta $2A
    stx $2B
    lda _main__local_56
    ldx _main__local_56+1
    sta $2C
    stx $2D
    lda _main__local_55
    ldx _main__local_55+1
    sta $2E
    stx $2F
    lda _main__local_54
    ldx _main__local_54+1
    sta $30
    stx $31
    lda _main__local_53
    ldx _main__local_53+1
    sta $32
    stx $33
    lda _main__local_52
    ldx _main__local_52+1
    sta $34
    stx $35
    lda $28
    ldx $29
    push .ax
    lda $2A
    ldx $2B
    push .ax
    lda $2C
    ldx $2D
    push .ax
    lda $2E
    ldx $2F
    push .ax
    lda $30
    ldx $31
    push .ax
    lda $32
    ldx $33
    push .ax
    lda $34
    ldx $35
    push .ax
    jsr _count_above
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    plz
    plz
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #2
    bne @if_then36
    bra @if_end38
@if_then36:
    lda _main__local_0
    ldx _main__local_0+1
    sta $20
    stx $21
    lda $20
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta _main__local_0
    stx _main__local_0+1
@if_end38:
    .loc "test_variadic.c", 78
    lda #7
    ldx #0
    push .ax
    jsr _no_varargs
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #14
    bne @if_then40
    bra @if_end42
@if_then40:
    lda _main__local_0
    ldx _main__local_0+1
    sta $20
    stx $21
    lda $20
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta _main__local_0
    stx _main__local_0+1
@if_end42:
    .loc "test_variadic.c", 81
    lda #144
    ldx #1
    push .ax
    lda #44
    ldx #1
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
    cmp.16 .AX, #700
    bne @if_then43
    bra @if_end45
@if_then43:
    lda _main__local_0
    ldx _main__local_0+1
    sta $20
    stx $21
    lda $20
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta _main__local_0
    stx _main__local_0+1
@if_end45:
    .loc "test_variadic.c", 84
    lda #200
    ldx #0
    push .ax
    lda #100
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
    clc
    adc #5
    sta $24
    lda $21
    adc #0
    sta $25
    lda $24
    ldx $25
    sta _main__local_78
    stx _main__local_78+1
    .loc "test_variadic.c", 85
    lda _main__local_78
    ldx _main__local_78+1
    cmp.16 .AX, #305
    bne @if_then46
    bra @if_end48
@if_then46:
    lda _main__local_0
    ldx _main__local_0+1
    sta $20
    stx $21
    lda $20
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta _main__local_0
    stx _main__local_0+1
@if_end48:
    .loc "test_variadic.c", 87
    lda _main__local_0
    ldx _main__local_0+1
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc


    .segment "data"
__str_39:
    .text "test"
    .byte 0

__zp_save_buf:

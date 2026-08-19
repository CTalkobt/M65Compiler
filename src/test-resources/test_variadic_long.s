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

    .global _sum_longs
    .global _main

    .segment "code"

; function _sum_longs

    ; Static buffer for struct return from _sum_longs
    _sum_longs__struct_buf:
    .byte 0, 0, 0, 0

    proc _sum_longs, W#@_p_count
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdarg.h", 4
; frame: 10 bytes (frame-allocated vRegs only)
    phw #0
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
    .local __vr3 = 6
    .local __vr5 = 4
    .local @_l_ap = 2
    .local @_l_i = 4
    .local @_l_total = 6
; .debug_var: __sum_longs @_l_ap offset=2 size=2 type=int16 scope=local
; .debug_var: __sum_longs @_l_i offset=4 size=2 type=int16 scope=local
; .debug_var: __sum_longs @_l_total offset=6 size=4 type=int32 scope=local
    .var @_p_count = 12
; .debug_var: __sum_longs @_p_count offset=12 size=2 type=int16 scope=parameter

    ldax.param @_p_count
    stax.local __vr0
@entry:
    .loc "test_variadic_long.c", 6
    leax.param @_p_count
    add.16 .AX, #2
    sta $20
    stx $21
    lda $20
    ldx $21
    stax.local __vr1
    .loc "test_variadic_long.c", 7
    lda #0
    taz
    staz.local __vr3
    .loc "test_variadic_long.c", 9
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
    .loc "test_variadic_long.c", 10
    ldax.local __vr1
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    taz
    ply
    plx
    pla
    sta $22
    stx $23
    sty $24
    stz $25
    ldax.local __vr1
    add.16 .AX, #4
    sta $26
    stx $27
    stax.local __vr1
    ldaxyz.local __vr3
    add.32 .AXYZ, $22
    sta $28
    stx $29
    sty $2A
    stz $2B
    staxyz.local __vr3
@for_inc2:
    .loc "test_variadic_long.c", 9
    ldax.local __vr5
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
    stax.local __vr5
    bra @for_cond0
@for_end3:
    .loc "test_variadic_long.c", 12
    .loc "test_variadic_long.c", 13
    ldaxyz.local __vr3
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
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 10
    endproc

; function _main
; SAC inline storage: 20 bytes
    _main__local_0: .long 0
    _main__local_2: .long 0
    _main__local_4: .long 0
    _main__local_6: .word 0
    _main__local_7: .word 0
    _main__local_8: .word 0
    _main__local_9: .word 0
    _main__local_14: .word 0
    _main__local_18: .word 0
    _main__local_19: .word 0
    _main__local_20: .long 0
    _main__local_24: .long 0
    _main__local_26: .long 0
    _main__local_28: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_variadic_long.c", 5
    .local @_l_a = 0
    .local @_l_b = 4
    .local @_l_c = 8
    .local @_l_x = 12
    .local @_l_y = 16
; .debug_var: __main @_l_a offset=0 size=4 type=int32 scope=local
; .debug_var: __main @_l_b offset=4 size=4 type=int32 scope=local
; .debug_var: __main @_l_c offset=8 size=4 type=int32 scope=local
; .debug_var: __main @_l_x offset=12 size=4 type=int32 scope=local
; .debug_var: __main @_l_y offset=16 size=4 type=int32 scope=local

@entry:
    .loc "test_variadic_long.c", 17
    lda #160
    ldx #134
    ldy #1
    ldz #0
    sta $20
    stx $21
    sty $22
    stz $23
    sta _main__local_0
    stx _main__local_0+1
    sty _main__local_0+2
    stz _main__local_0+3
    .loc "test_variadic_long.c", 18
    lda #64
    ldx #13
    ldy #3
    ldz #0
    sta $20
    stx $21
    sty $22
    stz $23
    sta _main__local_2
    stx _main__local_2+1
    sty _main__local_2+2
    stz _main__local_2+3
    .loc "test_variadic_long.c", 19
    lda #224
    ldx #147
    ldy #4
    ldz #0
    sta $20
    stx $21
    sty $22
    stz $23
    sta _main__local_4
    stx _main__local_4+1
    sty _main__local_4+2
    stz _main__local_4+3
    .loc "test_variadic_long.c", 22
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
    jsr _sum_longs
    sta __zp_scratch4
    stx __zp_scratch4+1
    sty __zp_scratch4+2
    stz __zp_scratch4+3
    plz
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    ldy __zp_scratch4+2
    ldz __zp_scratch4+3
    sta $20
    stx $21
    sty $22
    stz $23
    lda #60
    ldx #0
    sta $24
    stx $25
    lda $24
    ldx $25
    ldy #0
    ldz #0
    sta $26
    stx $27
    sty $28
    stz $29
    lda $20
    ldx $21
    ldy $22
    ldz $23
    cmp.32 .AXYZ, $26
    bne @if_then4
    bra @if_end6
@if_then4:
    lda #1
    ldx #0
    bra @__return
@if_end6:
    .loc "test_variadic_long.c", 25
    lda #2
    ldx #0
    sta _main__local_14
    stx _main__local_14+1
    lda _main__local_2
    ldx _main__local_2+1
    ldy _main__local_2+2
    ldz _main__local_2+3
    sta $28
    stx $29
    sty $2A
    stz $2B
    lda _main__local_0
    ldx _main__local_0+1
    ldy _main__local_0+2
    ldz _main__local_0+3
    sta $2C
    stx $2D
    sty $2E
    stz $2F
    lda _main__local_14
    ldx _main__local_14+1
    sta $30
    stx $31
    lda $28
    ldx $29
    ldy $2A
    ldz $2B
    push .axyz
    lda $2C
    ldx $2D
    ldy $2E
    ldz $2F
    push .axyz
    lda $30
    ldx $31
    push .ax
    jsr _sum_longs
    sta __zp_scratch4
    stx __zp_scratch4+1
    sty __zp_scratch4+2
    stz __zp_scratch4+3
    plz
    plz
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    ldy __zp_scratch4+2
    ldz __zp_scratch4+3
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    cmp.32 .AXYZ, #300000
    bne @if_then7
    bra @if_end9
@if_then7:
    lda #2
    ldx #0
    bra @__return
@if_end9:
    .loc "test_variadic_long.c", 28
    lda #6
    ldx #0
    push .ax
    lda #128
    ldx #26
    push .ax
    lda #5
    ldx #0
    push .ax
    lda #2
    ldx #0
    push .ax
    jsr _sum_longs
    sta __zp_scratch4
    stx __zp_scratch4+1
    sty __zp_scratch4+2
    stz __zp_scratch4+3
    plz
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    ldy __zp_scratch4+2
    ldz __zp_scratch4+3
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    cmp.32 .AXYZ, #400005
    bne @if_then10
    bra @if_end12
@if_then10:
    lda #3
    ldx #0
    bra @__return
@if_end12:
    .loc "test_variadic_long.c", 31
    lda #5
    sta _main__local_24
    lda #0
    sta _main__local_24+1
    .loc "test_variadic_long.c", 32
    lda #10
    sta _main__local_26
    lda #0
    sta _main__local_26+1
    .loc "test_variadic_long.c", 33
    lda #2
    ldx #0
    sta _main__local_28
    stx _main__local_28+1
    lda _main__local_26
    ldx _main__local_26+1
    ldy _main__local_26+2
    ldz _main__local_26+3
    sta $28
    stx $29
    sty $2A
    stz $2B
    lda _main__local_24
    ldx _main__local_24+1
    ldy _main__local_24+2
    ldz _main__local_24+3
    sta $2C
    stx $2D
    sty $2E
    stz $2F
    lda _main__local_28
    ldx _main__local_28+1
    sta $30
    stx $31
    lda $28
    ldx $29
    ldy $2A
    ldz $2B
    push .axyz
    lda $2C
    ldx $2D
    ldy $2E
    ldz $2F
    push .axyz
    lda $30
    ldx $31
    push .ax
    jsr _sum_longs
    sta __zp_scratch4
    stx __zp_scratch4+1
    sty __zp_scratch4+2
    stz __zp_scratch4+3
    plz
    plz
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    ldy __zp_scratch4+2
    ldz __zp_scratch4+3
    sta $20
    stx $21
    sty $22
    stz $23
    lda #15
    ldx #0
    sta $24
    stx $25
    lda $24
    ldx $25
    ldy #0
    ldz #0
    sta $26
    stx $27
    sty $28
    stz $29
    lda $20
    ldx $21
    ldy $22
    ldz $23
    cmp.32 .AXYZ, $26
    bne @if_then13
    bra @if_end15
@if_then13:
    lda #4
    ldx #0
    bra @__return
@if_end15:
    .loc "test_variadic_long.c", 35
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 20
    endproc


__zp_save_buf:

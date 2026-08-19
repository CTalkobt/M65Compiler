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

    .global _char_double
    .global _int_add_ten
    .global _char_identity
    .global _add
    .global _main

    .segment "code"

; function _char_double
; SAC inline storage: 2 bytes
    .global _char_double__param_x
    _char_double__param_x: .word 0
    _char_double__local_0: .word 0
    _char_double__local_2: .word 0
    _char_double__local_4: .word 0
    proc _char_double
    .sac
    .var _fp = 0
    .loc "test_regparm.c", 3
    .var @_p_x = 2
; .debug_var: __char_double @_p_x offset=2 size=2 type=int8 scope=parameter

@entry:
    lda _char_double__param_x
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    clc
    adc $20
    sta $22
    stx $23
    lda $22
    ldx $23
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _int_add_ten
; SAC inline storage: 2 bytes
    .global _int_add_ten__param_x
    _int_add_ten__param_x: .word 0
    _int_add_ten__local_0: .word 0
    proc _int_add_ten
    .sac
    .var _fp = 0
    .loc "test_regparm.c", 4
    .var @_p_x = 2
; .debug_var: __int_add_ten @_p_x offset=2 size=2 type=int16 scope=parameter

@entry:
    lda _int_add_ten__param_x
    ldx _int_add_ten__param_x+1
    add.16 .AX, #10
    sta $22
    stx $23
    lda $22
    ldx $23
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _char_identity
; SAC inline storage: 2 bytes
    .global _char_identity__param_x
    _char_identity__param_x: .word 0
    _char_identity__local_0: .word 0
    proc _char_identity
    .sac
    .var _fp = 0
    .loc "test_regparm.c", 5
    .var @_p_x = 2
; .debug_var: __char_identity @_p_x offset=2 size=2 type=int8 scope=parameter

@entry:
    lda _char_identity__param_x
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers N, Z
    .frame_size 2
    endproc

; function _add
; SAC inline storage: 4 bytes
    .global _add__param_a
    _add__param_a: .word 0
    .global _add__param_b
    _add__param_b: .word 0
    _add__local_0: .word 0
    _add__local_1: .word 0
    proc _add, W#@_p_b
    .sac
    .var _fp = 0
    .loc "test_regparm.c", 8
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __add @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __add @_p_b offset=4 size=2 type=int16 scope=parameter

@entry:
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

; function _main
; SAC zero-alloc leaf: no storage overhead
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_regparm.c", 10

@entry:
    .loc "test_regparm.c", 12
    lda #21
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda $22
    ldx #0
    sta $20
    lda $22
    ldx #0
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_regparm.c", 3
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
@inline_end3:
    .loc "test_regparm.c", 12
    lda #42
    sta $22
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx #0
    ldx #0
    sta $20
    stx $21
    lda $24
    ldx $25
    cmp.16 .AX, $20
    bne @if_then0
    bra @if_end2
@if_then0:
    lda #1
    ldx #0
    bra @__return
@if_end2:
    .loc "test_regparm.c", 13
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    lda $22
    ldx #0
    sta $20
    lda $22
    ldx #0
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_regparm.c", 3
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
@inline_end8:
    .loc "test_regparm.c", 13
    lda #0
    sta $22
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx #0
    ldx #0
    sta $20
    stx $21
    lda $24
    ldx $25
    cmp.16 .AX, $20
    bne @if_then5
    bra @if_end7
@if_then5:
    lda #2
    ldx #0
    bra @__return
@if_end7:
    .loc "test_regparm.c", 14
    lda #127
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda $22
    ldx #0
    sta $20
    lda $22
    ldx #0
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_regparm.c", 3
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
@inline_end13:
    .loc "test_regparm.c", 14
    lda #254
    sta $22
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx #0
    ldx #0
    sta $20
    stx $21
    lda $24
    ldx $25
    cmp.16 .AX, $20
    bne @if_then10
    bra @if_end12
@if_then10:
    lda #3
    ldx #0
    bra @__return
@if_end12:
    .loc "test_regparm.c", 17
    lda #32
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_regparm.c", 4
    lda $20
    clc
    adc #10
    sta $24
    lda $21
    adc #0
    sta $25
    lda $24
    ldx $25
    sta $20
    stx $21
@inline_end18:
    .loc "test_regparm.c", 17
    lda $20
    ldx $21
    cmp.16 .AX, #42
    bne @if_then15
    bra @if_end17
@if_then15:
    lda #4
    ldx #0
    bra @__return
@if_end17:
    .loc "test_regparm.c", 18
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_regparm.c", 4
    lda $20
    clc
    adc #10
    sta $24
    lda $21
    adc #0
    sta $25
    lda $24
    ldx $25
    sta $20
    stx $21
@inline_end23:
    .loc "test_regparm.c", 18
    lda $20
    ldx $21
    cmp.16 .AX, #10
    bne @if_then20
    bra @if_end22
@if_then20:
    lda #5
    ldx #0
    bra @__return
@if_end22:
    .loc "test_regparm.c", 19
    lda #232
    ldx #3
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_regparm.c", 4
    lda $20
    clc
    adc #10
    sta $24
    lda $21
    adc #0
    sta $25
    lda $24
    ldx $25
    sta $20
    stx $21
@inline_end28:
    .loc "test_regparm.c", 19
    lda $20
    ldx $21
    cmp.16 .AX, #1010
    bne @if_then25
    bra @if_end27
@if_then25:
    lda #6
    ldx #0
    bra @__return
@if_end27:
    .loc "test_regparm.c", 22
    lda #255
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda $22
    ldx #0
    sta $20
    lda $22
    ldx #0
    ldx #0
    sta $20
    stx $21
    .loc "test_regparm.c", 5
    lda $20
    ldx $21
    sta $22
@inline_end33:
    .loc "test_regparm.c", 22
    lda #255
    sta $20
    lda $22
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $24
    ldx $25
    cmp.16 .AX, $22
    bne @if_then30
    bra @if_end32
@if_then30:
    lda #7
    ldx #0
    bra @__return
@if_end32:
    .loc "test_regparm.c", 23
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    lda $22
    ldx #0
    sta $20
    lda $22
    ldx #0
    ldx #0
    sta $20
    stx $21
    .loc "test_regparm.c", 5
    lda $20
    ldx $21
    sta $22
@inline_end38:
    .loc "test_regparm.c", 23
    lda #0
    sta $20
    lda $22
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $24
    ldx $25
    cmp.16 .AX, $22
    bne @if_then35
    bra @if_end37
@if_then35:
    lda #8
    ldx #0
    bra @__return
@if_end37:
    .loc "test_regparm.c", 26
    lda #10
    ldx #0
    sta $20
    stx $21
    lda #20
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    sta $24
    stx $25
    lda $22
    ldx $23
    sta $24
    stx $25
    .loc "test_regparm.c", 8
    lda $20
    clc
    adc #20
    sta $24
    lda $21
    adc #0
    sta $25
    lda $24
    ldx $25
    sta $20
    stx $21
@inline_end43:
    .loc "test_regparm.c", 26
    lda $20
    ldx $21
    cmp.16 .AX, #30
    bne @if_then40
    bra @if_end42
@if_then40:
    lda #9
    ldx #0
    bra @__return
@if_end42:
    .loc "test_regparm.c", 27
    lda #100
    ldx #0
    sta $20
    stx $21
    lda #200
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    sta $24
    stx $25
    lda $22
    ldx $23
    sta $24
    stx $25
    .loc "test_regparm.c", 8
    lda $20
    clc
    adc #200
    sta $24
    lda $21
    adc #0
    sta $25
    lda $24
    ldx $25
    sta $20
    stx $21
@inline_end48:
    .loc "test_regparm.c", 27
    lda $20
    ldx $21
    cmp.16 .AX, #300
    bne @if_then45
    bra @if_end47
@if_then45:
    lda #10
    ldx #0
    bra @__return
@if_end47:
    .loc "test_regparm.c", 29
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, zeroalloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 0
    endproc


__zp_save_buf:

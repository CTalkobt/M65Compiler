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

    .global _char_and
    .global _char_or
    .global _char_xor
    .global _char_add
    .global _char_sub
    .global _char_and_lit
    .global _char_or_lit
    .global _char_add_lit
    .global _char_inc
    .global _char_dec
    .global _char_lt
    .global _char_eq
    .global _int_plus_char
    .global _long_plus_char
    .global _long_add_const
    .global _main

    .segment "code"

; function _char_and
; SAC inline storage: 4 bytes
    .global _char_and__param_a
    _char_and__param_a: .word 0
    .global _char_and__param_b
    _char_and__param_b: .word 0
    _char_and__local_0: .word 0
    _char_and__local_1: .word 0
    _char_and__local_5: .word 0
    proc _char_and, B#@_p_a, B#@_p_b
    .sac
    .var _fp = 0
    .loc "test_i8_arith.c", 5
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __char_and @_p_a offset=2 size=2 type=int8 scope=parameter
; .debug_var: __char_and @_p_b offset=4 size=2 type=int8 scope=parameter

@entry:
    lda _char_and__param_a
    ldx #0
    sta $20
    stx $21
    lda _char_and__param_b
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
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers N, Z
    .frame_size 4
    endproc

; function _char_or
; SAC inline storage: 4 bytes
    .global _char_or__param_a
    _char_or__param_a: .word 0
    .global _char_or__param_b
    _char_or__param_b: .word 0
    _char_or__local_0: .word 0
    _char_or__local_1: .word 0
    _char_or__local_5: .word 0
    proc _char_or, B#@_p_a, B#@_p_b
    .sac
    .var _fp = 0
    .loc "test_i8_arith.c", 6
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __char_or @_p_a offset=2 size=2 type=int8 scope=parameter
; .debug_var: __char_or @_p_b offset=4 size=2 type=int8 scope=parameter

@entry:
    lda _char_or__param_a
    ldx #0
    sta $20
    stx $21
    lda _char_or__param_b
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    ora $22
    sta $24
    stx $25
    lda $24
    ldx $25
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers N, Z
    .frame_size 4
    endproc

; function _char_xor
; SAC inline storage: 4 bytes
    .global _char_xor__param_a
    _char_xor__param_a: .word 0
    .global _char_xor__param_b
    _char_xor__param_b: .word 0
    _char_xor__local_0: .word 0
    _char_xor__local_1: .word 0
    _char_xor__local_5: .word 0
    proc _char_xor, B#@_p_a, B#@_p_b
    .sac
    .var _fp = 0
    .loc "test_i8_arith.c", 7
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __char_xor @_p_a offset=2 size=2 type=int8 scope=parameter
; .debug_var: __char_xor @_p_b offset=4 size=2 type=int8 scope=parameter

@entry:
    lda _char_xor__param_a
    ldx #0
    sta $20
    stx $21
    lda _char_xor__param_b
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    eor $22
    sta $24
    stx $25
    lda $24
    ldx $25
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers N, Z
    .frame_size 4
    endproc

; function _char_add
; SAC inline storage: 4 bytes
    .global _char_add__param_a
    _char_add__param_a: .word 0
    .global _char_add__param_b
    _char_add__param_b: .word 0
    _char_add__local_0: .word 0
    _char_add__local_1: .word 0
    _char_add__local_5: .word 0
    proc _char_add, B#@_p_a, B#@_p_b
    .sac
    .var _fp = 0
    .loc "test_i8_arith.c", 8
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __char_add @_p_a offset=2 size=2 type=int8 scope=parameter
; .debug_var: __char_add @_p_b offset=4 size=2 type=int8 scope=parameter

@entry:
    lda _char_add__param_a
    ldx #0
    sta $20
    stx $21
    lda _char_add__param_b
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    clc
    adc $22
    sta $24
    stx $25
    lda $24
    ldx $25
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _char_sub
; SAC inline storage: 4 bytes
    .global _char_sub__param_a
    _char_sub__param_a: .word 0
    .global _char_sub__param_b
    _char_sub__param_b: .word 0
    _char_sub__local_0: .word 0
    _char_sub__local_1: .word 0
    proc _char_sub, B#@_p_a, B#@_p_b
    .sac
    .var _fp = 0
    .loc "test_i8_arith.c", 9
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __char_sub @_p_a offset=2 size=2 type=int8 scope=parameter
; .debug_var: __char_sub @_p_b offset=4 size=2 type=int8 scope=parameter

@entry:
    lda _char_sub__param_a
    ldx #0
    sta $20
    stx $21
    lda _char_sub__param_b
    ldx #0
    sta $22
    stx $23
    lda $20
    sec
    sbc $22
    sta $24
    lda $21
    sbc $22+1
    sta $25
    lda $24
    ldx $25
    sta $20
    lda $20
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _char_and_lit
; SAC inline storage: 2 bytes
    .global _char_and_lit__param_a
    _char_and_lit__param_a: .word 0
    _char_and_lit__local_0: .word 0
    proc _char_and_lit, B#@_p_a
    .sac
    .var _fp = 0
    .loc "test_i8_arith.c", 10
    .var @_p_a = 2
; .debug_var: __char_and_lit @_p_a offset=2 size=2 type=int8 scope=parameter

@entry:
    lda #15
    sta $20
    lda _char_and_lit__param_a
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    and $24
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers N, Z
    .frame_size 2
    endproc

; function _char_or_lit
; SAC inline storage: 2 bytes
    .global _char_or_lit__param_a
    _char_or_lit__param_a: .word 0
    _char_or_lit__local_0: .word 0
    proc _char_or_lit, B#@_p_a
    .sac
    .var _fp = 0
    .loc "test_i8_arith.c", 11
    .var @_p_a = 2
; .debug_var: __char_or_lit @_p_a offset=2 size=2 type=int8 scope=parameter

@entry:
    lda #128
    sta $20
    lda _char_or_lit__param_a
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    ora $24
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers N, Z
    .frame_size 2
    endproc

; function _char_add_lit
; SAC inline storage: 2 bytes
    .global _char_add_lit__param_a
    _char_add_lit__param_a: .word 0
    _char_add_lit__local_0: .word 0
    proc _char_add_lit, B#@_p_a
    .sac
    .var _fp = 0
    .loc "test_i8_arith.c", 12
    .var @_p_a = 2
; .debug_var: __char_add_lit @_p_a offset=2 size=2 type=int8 scope=parameter

@entry:
    lda #10
    sta $20
    lda _char_add_lit__param_a
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    clc
    adc $24
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _char_inc
; SAC inline storage: 2 bytes
    .global _char_inc__param_a
    _char_inc__param_a: .word 0
    _char_inc__local_0: .word 0
    proc _char_inc, B#@_p_a
    .sac
    .var _fp = 0
    .loc "test_i8_arith.c", 14
    .var @_p_a = 2
; .debug_var: __char_inc @_p_a offset=2 size=2 type=int8 scope=parameter

@entry:
    lda _char_inc__param_a
    sta $20
    lda $20
    inc a
    sta $22
    sta _char_inc__param_a
    lda _char_inc__param_a
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _char_dec
; SAC inline storage: 2 bytes
    .global _char_dec__param_a
    _char_dec__param_a: .word 0
    _char_dec__local_0: .word 0
    proc _char_dec, B#@_p_a
    .sac
    .var _fp = 0
    .loc "test_i8_arith.c", 15
    .var @_p_a = 2
; .debug_var: __char_dec @_p_a offset=2 size=2 type=int8 scope=parameter

@entry:
    lda _char_dec__param_a
    sta $20
    lda $20
    dec a
    sta $22
    sta _char_dec__param_a
    lda _char_dec__param_a
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _char_lt
; SAC inline storage: 4 bytes
    .global _char_lt__param_a
    _char_lt__param_a: .word 0
    .global _char_lt__param_b
    _char_lt__param_b: .word 0
    _char_lt__local_0: .word 0
    _char_lt__local_1: .word 0
    proc _char_lt, B#@_p_a, B#@_p_b
    .sac
    .var _fp = 0
    .loc "test_i8_arith.c", 19
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __char_lt @_p_a offset=2 size=2 type=int8 scope=parameter
; .debug_var: __char_lt @_p_b offset=4 size=2 type=int8 scope=parameter

@entry:
    lda _char_lt__param_a
    ldx #0
    sta $20
    stx $21
    lda _char_lt__param_b
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    cmp.16 .AX, $22
    bcc @__cmp_set_0
    bra @__cmp_zero_0
@__cmp_set_0:
    lda #1
    ldx #0
    bra @__cmp_done_0
@__cmp_zero_0:
    lda #0
    ldx #0
@__cmp_done_0:
    sta $24
    lda $24
    ldx #0
    ldx #0
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

; function _char_eq
; SAC inline storage: 4 bytes
    .global _char_eq__param_a
    _char_eq__param_a: .word 0
    .global _char_eq__param_b
    _char_eq__param_b: .word 0
    _char_eq__local_0: .word 0
    _char_eq__local_1: .word 0
    proc _char_eq, B#@_p_a, B#@_p_b
    .sac
    .var _fp = 0
    .loc "test_i8_arith.c", 20
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __char_eq @_p_a offset=2 size=2 type=int8 scope=parameter
; .debug_var: __char_eq @_p_b offset=4 size=2 type=int8 scope=parameter

@entry:
    lda _char_eq__param_a
    ldx #0
    sta $20
    stx $21
    lda _char_eq__param_b
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    cmp.16 .AX, $22
    beq @__cmp_set_1
    bra @__cmp_zero_1
@__cmp_set_1:
    lda #1
    ldx #0
    bra @__cmp_done_1
@__cmp_zero_1:
    lda #0
    ldx #0
@__cmp_done_1:
    sta $24
    lda $24
    ldx #0
    ldx #0
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

; function _int_plus_char
; SAC inline storage: 4 bytes
    .global _int_plus_char__param_a
    _int_plus_char__param_a: .word 0
    .global _int_plus_char__param_b
    _int_plus_char__param_b: .word 0
    _int_plus_char__local_0: .word 0
    _int_plus_char__local_1: .word 0
    proc _int_plus_char, W#@_p_a, B#@_p_b
    .sac
    .var _fp = 0
    .loc "test_i8_arith.c", 24
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __int_plus_char @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __int_plus_char @_p_b offset=4 size=2 type=int8 scope=parameter

@entry:
    lda _int_plus_char__param_b
    ldx #0
    sta $20
    stx $21
    lda _int_plus_char__param_a
    ldx _int_plus_char__param_a+1
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
    .frame_size 4
    endproc

; function _long_plus_char

    ; Static buffer for struct return from _long_plus_char
    _long_plus_char__struct_buf:
    .byte 0, 0, 0, 0

; SAC inline storage: 6 bytes
    .global _long_plus_char__param_l
    _long_plus_char__param_l: .long 0
    .global _long_plus_char__param_c
    _long_plus_char__param_c: .word 0
    _long_plus_char__local_0: .long 0
    _long_plus_char__local_1: .word 0
    proc _long_plus_char, D#@_p_l, B#@_p_c
    .sac
    .var _fp = 0
    .loc "test_i8_arith.c", 27
    .var @_p_l = 2
    .var @_p_c = 6
; .debug_var: __long_plus_char @_p_l offset=2 size=4 type=int32 scope=parameter
; .debug_var: __long_plus_char @_p_c offset=6 size=2 type=int8 scope=parameter

@entry:
    lda _long_plus_char__param_c
    ldx #0
    ldx #0
    ldy #0
    ldz #0
    sta $20
    stx $21
    sty $22
    stz $23
    .loc "test_i8_arith.c", 28
    lda _long_plus_char__param_l
    ldx _long_plus_char__param_l+1
    ldy _long_plus_char__param_l+2
    ldz _long_plus_char__param_l+3
    add.32 .AXYZ, $20
    sta $24
    stx $25
    sty $26
    stz $27
    sta _long_plus_char__param_l
    stx _long_plus_char__param_l+1
    sty _long_plus_char__param_l+2
    stz _long_plus_char__param_l+3
    .loc "test_i8_arith.c", 29
    lda _long_plus_char__param_l
    ldx _long_plus_char__param_l+1
    ldy _long_plus_char__param_l+2
    ldz _long_plus_char__param_l+3
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _long_add_const

    ; Static buffer for struct return from _long_add_const
    _long_add_const__struct_buf:
    .byte 0, 0, 0, 0

; SAC inline storage: 4 bytes
    _long_add_const__local_0: .long 0
    proc _long_add_const
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_i8_arith.c", 32
    .local @_l_l = 0
; .debug_var: __long_add_const @_l_l offset=0 size=4 type=int32 scope=local

@entry:
    .loc "test_i8_arith.c", 33
    lda #1
    sta _long_add_const__local_0
    sta _long_add_const__local_0+1
    .loc "test_i8_arith.c", 34
    lda #20
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    ldy #0
    ldz #0
    sta $22
    stx $23
    sty $24
    stz $25
    lda _long_add_const__local_0
    ldx _long_add_const__local_0+1
    ldy _long_add_const__local_0+2
    ldz _long_add_const__local_0+3
    add.32 .AXYZ, $22
    sta $26
    stx $27
    sty $28
    stz $29
    sta _long_add_const__local_0
    stx _long_add_const__local_0+1
    sty _long_add_const__local_0+2
    stz _long_add_const__local_0+3
    .loc "test_i8_arith.c", 35
    lda _long_add_const__local_0
    ldx _long_add_const__local_0+1
    ldy _long_add_const__local_0+2
    ldz _long_add_const__local_0+3
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _main
; SAC inline storage: 4 bytes
    _main__local_305: .long 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_i8_arith.c", 38
    .local @_l_l = 0
; .debug_var: __main @_l_l offset=0 size=4 type=int32 scope=local

@entry:
    .loc "test_i8_arith.c", 40
    lda #255
    ldx #0
    sta $20
    stx $21
    lda #15
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    sta $24
    lda $22
    ldx $23
    sta $20
    lda $24
    ldx #0
    sta $22
    lda $20
    ldx #0
    sta $22
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    .loc "test_i8_arith.c", 5
    lda $22
    ldx $23
    and.16 .AX, $24
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
@inline_end3:
    .loc "test_i8_arith.c", 40
    lda #15
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
    bne @if_then0
    bra @if_end2
@if_then0:
    lda #1
    ldx #0
    bra @__return
@if_end2:
    .loc "test_i8_arith.c", 41
    lda #170
    ldx #0
    sta $20
    stx $21
    lda #85
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    sta $24
    lda $22
    ldx $23
    sta $20
    lda $24
    ldx #0
    sta $22
    lda $20
    ldx #0
    sta $22
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    .loc "test_i8_arith.c", 5
    lda $22
    ldx $23
    and.16 .AX, $24
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
@inline_end8:
    .loc "test_i8_arith.c", 41
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
    bne @if_then5
    bra @if_end7
@if_then5:
    lda #2
    ldx #0
    bra @__return
@if_end7:
    .loc "test_i8_arith.c", 44
    lda #15
    ldx #0
    sta $20
    stx $21
    lda #240
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    sta $24
    lda $22
    ldx $23
    sta $20
    lda $24
    ldx #0
    sta $22
    lda $20
    ldx #0
    sta $22
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    .loc "test_i8_arith.c", 6
    lda $22
    ldx $23
    ora.16 .AX, $24
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
@inline_end13:
    .loc "test_i8_arith.c", 44
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
    bne @if_then10
    bra @if_end12
@if_then10:
    lda #3
    ldx #0
    bra @__return
@if_end12:
    .loc "test_i8_arith.c", 45
    lda #0
    sta $20
    sta $21
    lda #66
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    sta $24
    lda $22
    ldx $23
    sta $20
    lda $24
    ldx #0
    sta $22
    lda $20
    ldx #0
    sta $22
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    .loc "test_i8_arith.c", 6
    lda $22
    ldx $23
    ora.16 .AX, $24
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
@inline_end18:
    .loc "test_i8_arith.c", 45
    lda #66
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
    bne @if_then15
    bra @if_end17
@if_then15:
    lda #4
    ldx #0
    bra @__return
@if_end17:
    .loc "test_i8_arith.c", 48
    lda #255
    ldx #0
    sta $20
    stx $21
    lda #255
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    sta $24
    lda $22
    ldx $23
    sta $20
    lda $24
    ldx #0
    sta $22
    lda $20
    ldx #0
    sta $22
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    .loc "test_i8_arith.c", 7
    lda $22
    ldx $23
    eor.16 .AX, $24
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
@inline_end23:
    .loc "test_i8_arith.c", 48
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
    bne @if_then20
    bra @if_end22
@if_then20:
    lda #5
    ldx #0
    bra @__return
@if_end22:
    .loc "test_i8_arith.c", 49
    lda #170
    ldx #0
    sta $20
    stx $21
    lda #15
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    sta $24
    lda $22
    ldx $23
    sta $20
    lda $24
    ldx #0
    sta $22
    lda $20
    ldx #0
    sta $22
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    .loc "test_i8_arith.c", 7
    lda $22
    ldx $23
    eor.16 .AX, $24
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
@inline_end28:
    .loc "test_i8_arith.c", 49
    lda #165
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
    bne @if_then25
    bra @if_end27
@if_then25:
    lda #6
    ldx #0
    bra @__return
@if_end27:
    .loc "test_i8_arith.c", 52
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
    lda $22
    ldx $23
    sta $20
    lda $24
    ldx #0
    sta $22
    lda $20
    ldx #0
    sta $22
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    .loc "test_i8_arith.c", 8
    lda $22
    clc
    adc $24
    sta $20
    lda $23
    adc $24+1
    sta $21
    lda $20
    ldx $21
    sta $22
@inline_end33:
    .loc "test_i8_arith.c", 52
    lda #30
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
    .loc "test_i8_arith.c", 53
    lda #200
    ldx #0
    sta $20
    stx $21
    lda #56
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    sta $24
    lda $22
    ldx $23
    sta $20
    lda $24
    ldx #0
    sta $22
    lda $20
    ldx #0
    sta $22
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    .loc "test_i8_arith.c", 8
    lda $22
    clc
    adc $24
    sta $20
    lda $23
    adc $24+1
    sta $21
    lda $20
    ldx $21
    sta $22
@inline_end38:
    .loc "test_i8_arith.c", 53
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
    .loc "test_i8_arith.c", 56
    lda #100
    ldx #0
    sta $20
    stx $21
    lda #30
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    sta $24
    lda $22
    ldx $23
    sta $20
    lda $24
    ldx #0
    sta $22
    lda $20
    ldx #0
    sta $22
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    .loc "test_i8_arith.c", 9
    lda $22
    sec
    sbc $24
    sta $20
    lda $23
    sbc $24+1
    sta $21
    lda $20
    ldx $21
    sta $22
@inline_end43:
    .loc "test_i8_arith.c", 56
    lda #70
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
    bne @if_then40
    bra @if_end42
@if_then40:
    lda #9
    ldx #0
    bra @__return
@if_end42:
    .loc "test_i8_arith.c", 59
    lda #171
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda $22
    ldx #0
    sta $20
    .loc "test_i8_arith.c", 10
    lda #15
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
    and.16 .AX, $22
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
@inline_end48:
    .loc "test_i8_arith.c", 59
    lda #11
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
    bne @if_then45
    bra @if_end47
@if_then45:
    lda #10
    ldx #0
    bra @__return
@if_end47:
    .loc "test_i8_arith.c", 62
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda $22
    ldx #0
    sta $20
    .loc "test_i8_arith.c", 11
    lda #128
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
    ora.16 .AX, $22
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
@inline_end53:
    .loc "test_i8_arith.c", 62
    lda #129
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
    bne @if_then50
    bra @if_end52
@if_then50:
    lda #11
    ldx #0
    bra @__return
@if_end52:
    .loc "test_i8_arith.c", 65
    lda #5
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda $22
    ldx #0
    sta $20
    .loc "test_i8_arith.c", 12
    lda #10
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
    clc
    adc $22
    sta $20
    lda $25
    adc $22+1
    sta $21
    lda $20
    ldx $21
    sta $22
@inline_end58:
    .loc "test_i8_arith.c", 65
    lda #15
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
    bne @if_then55
    bra @if_end57
@if_then55:
    lda #12
    ldx #0
    bra @__return
@if_end57:
    .loc "test_i8_arith.c", 68
    lda #41
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda $22
    ldx #0
    sta $20
    .loc "test_i8_arith.c", 14
    lda $22
    ldx #0
    sta $20
    lda $20
    inc a
    sta $24
    lda $24
    ldy #0
    sta ($22),y
    lda $22
    ldx #0
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
@inline_end63:
    .loc "test_i8_arith.c", 68
    lda #42
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
    bne @if_then60
    bra @if_end62
@if_then60:
    lda #13
    ldx #0
    bra @__return
@if_end62:
    .loc "test_i8_arith.c", 69
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
    .loc "test_i8_arith.c", 14
    lda $22
    ldx #0
    sta $20
    lda $20
    inc a
    sta $24
    lda $24
    ldy #0
    sta ($22),y
    lda $22
    ldx #0
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
@inline_end68:
    .loc "test_i8_arith.c", 69
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
    bne @if_then65
    bra @if_end67
@if_then65:
    lda #14
    ldx #0
    bra @__return
@if_end67:
    .loc "test_i8_arith.c", 72
    lda #43
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda $22
    ldx #0
    sta $20
    .loc "test_i8_arith.c", 15
    lda $22
    ldx #0
    sta $20
    lda $20
    dec a
    sta $24
    lda $24
    ldy #0
    sta ($22),y
    lda $22
    ldx #0
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
@inline_end73:
    .loc "test_i8_arith.c", 72
    lda #42
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
    bne @if_then70
    bra @if_end72
@if_then70:
    lda #15
    ldx #0
    bra @__return
@if_end72:
    .loc "test_i8_arith.c", 73
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    lda $22
    ldx #0
    sta $20
    .loc "test_i8_arith.c", 15
    lda $22
    ldx #0
    sta $20
    lda $20
    dec a
    sta $24
    lda $24
    ldy #0
    sta ($22),y
    lda $22
    ldx #0
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
@inline_end78:
    .loc "test_i8_arith.c", 73
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
    bne @if_then75
    bra @if_end77
@if_then75:
    lda #16
    ldx #0
    bra @__return
@if_end77:
    .loc "test_i8_arith.c", 76
    lda #5
    ldx #0
    sta $20
    stx $21
    lda #10
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    sta $24
    lda $22
    ldx $23
    sta $20
    lda $24
    ldx #0
    sta $22
    lda $20
    ldx #0
    sta $22
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    .loc "test_i8_arith.c", 19
    lda $22
    ldx $23
    cmp.16 .AX, $24
    bcc @__cmp_set_2
    bra @__cmp_zero_2
@__cmp_set_2:
    lda #1
    ldx #0
    bra @__cmp_done_2
@__cmp_zero_2:
    lda #0
    ldx #0
@__cmp_done_2:
    sta $20
    lda $20
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end83:
    .loc "test_i8_arith.c", 76
    lda $20
    ldx $21
    cmp.16 .AX, #1
    bne @if_then80
    bra @if_end82
@if_then80:
    lda #17
    ldx #0
    bra @__return
@if_end82:
    .loc "test_i8_arith.c", 77
    lda #10
    ldx #0
    sta $20
    stx $21
    lda #5
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    sta $24
    lda $22
    ldx $23
    sta $20
    lda $24
    ldx #0
    sta $22
    lda $20
    ldx #0
    sta $22
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    .loc "test_i8_arith.c", 19
    lda $22
    ldx $23
    cmp.16 .AX, $24
    bcc @__cmp_set_3
    bra @__cmp_zero_3
@__cmp_set_3:
    lda #1
    ldx #0
    bra @__cmp_done_3
@__cmp_zero_3:
    lda #0
    ldx #0
@__cmp_done_3:
    sta $20
    lda $20
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end88:
    .loc "test_i8_arith.c", 77
    lda $20
    ora $21
    bne @if_then85
    bra @if_end87
@if_then85:
    lda #18
    ldx #0
    bra @__return
@if_end87:
    .loc "test_i8_arith.c", 78
    lda #42
    ldx #0
    sta $20
    stx $21
    lda #42
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    sta $24
    lda $22
    ldx $23
    sta $20
    lda $24
    ldx #0
    sta $22
    lda $20
    ldx #0
    sta $22
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    .loc "test_i8_arith.c", 20
    lda $22
    ldx $23
    cmp.16 .AX, $24
    beq @__cmp_set_4
    bra @__cmp_zero_4
@__cmp_set_4:
    lda #1
    ldx #0
    bra @__cmp_done_4
@__cmp_zero_4:
    lda #0
    ldx #0
@__cmp_done_4:
    sta $20
    lda $20
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end93:
    .loc "test_i8_arith.c", 78
    lda $20
    ldx $21
    cmp.16 .AX, #1
    bne @if_then90
    bra @if_end92
@if_then90:
    lda #19
    ldx #0
    bra @__return
@if_end92:
    .loc "test_i8_arith.c", 79
    lda #42
    ldx #0
    sta $20
    stx $21
    lda #43
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    sta $24
    lda $22
    ldx $23
    sta $20
    lda $24
    ldx #0
    sta $22
    lda $20
    ldx #0
    sta $22
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    .loc "test_i8_arith.c", 20
    lda $22
    ldx $23
    cmp.16 .AX, $24
    beq @__cmp_set_5
    bra @__cmp_zero_5
@__cmp_set_5:
    lda #1
    ldx #0
    bra @__cmp_done_5
@__cmp_zero_5:
    lda #0
    ldx #0
@__cmp_done_5:
    sta $20
    lda $20
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end98:
    .loc "test_i8_arith.c", 79
    lda $20
    ora $21
    bne @if_then95
    bra @if_end97
@if_then95:
    lda #20
    ldx #0
    bra @__return
@if_end97:
    .loc "test_i8_arith.c", 82
    lda #232
    ldx #3
    sta $20
    stx $21
    lda #5
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $24
    ldx #0
    sta $22
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    .loc "test_i8_arith.c", 24
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
@inline_end103:
    .loc "test_i8_arith.c", 82
    lda $20
    ldx $21
    cmp.16 .AX, #1005
    bne @if_then100
    bra @if_end102
@if_then100:
    lda #21
    ldx #0
    bra @__return
@if_end102:
    .loc "test_i8_arith.c", 83
    lda #0
    ldx #1
    sta $20
    stx $21
    lda #1
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $24
    ldx #0
    sta $22
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    .loc "test_i8_arith.c", 24
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
@inline_end108:
    .loc "test_i8_arith.c", 83
    lda $20
    ldx $21
    cmp.16 .AX, #257
    bne @if_then105
    bra @if_end107
@if_then105:
    lda #22
    ldx #0
    bra @__return
@if_end107:
    .loc "test_i8_arith.c", 86
    lda #0
    ldx #1
    sta $20
    stx $21
    lda #1
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    ldy #0
    ldz #0
    sta $24
    stx $25
    sty $26
    stz $27
    lda $22
    ldx $23
    sta $20
    lda $24
    ldx $25
    ldy $26
    ldz $27
    sta $28
    stx $29
    sty $2A
    stz $2B
    lda $20
    ldx #0
    sta $22
    lda $20
    ldx #0
    ldx #0
    ldx #0
    ldy #0
    ldz #0
    sta $28
    stx $29
    sty $2A
    stz $2B
    .loc "test_i8_arith.c", 28
    lda $24
    ldx $25
    ldy $26
    ldz $27
    add.32 .AXYZ, $28
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    ldy #0
    sty __zp_scratch3
    stz __zp_scratch3+1
    sta ($24),y
    txa
    iny
    sta ($24),y
    lda __zp_scratch3
    iny
    sta ($24),y
    lda __zp_scratch3+1
    iny
    sta ($24),y
    lda $24
    ldx $25
    ldy $26
    ldz $27
    sta $20
    stx $21
    .loc "test_i8_arith.c", 29
    lda $20
    ldx $21
    sta $22
    stx $23
    sty $24
    stz $25
@inline_end113:
    .loc "test_i8_arith.c", 86
    lda #1
    sta $20
    sta $21
    lda $20
    ldx $21
    ldy #0
    ldz #0
    sta $26
    stx $27
    sty $28
    stz $29
    lda $22
    ldx $23
    ldy $24
    ldz $25
    cmp.32 .AXYZ, $26
    bne @if_then110
    bra @if_end112
@if_then110:
    lda #23
    ldx #0
    bra @__return
@if_end112:
    .loc "test_i8_arith.c", 87
    lda #232
    ldx #3
    sta $20
    stx $21
    lda #234
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    ldy #0
    ldz #0
    sta $24
    stx $25
    sty $26
    stz $27
    lda $22
    ldx $23
    sta $20
    lda $24
    ldx $25
    ldy $26
    ldz $27
    sta $28
    stx $29
    sty $2A
    stz $2B
    lda $20
    ldx #0
    sta $22
    lda $20
    ldx #0
    ldx #0
    ldx #0
    ldy #0
    ldz #0
    sta $28
    stx $29
    sty $2A
    stz $2B
    .loc "test_i8_arith.c", 28
    lda $24
    ldx $25
    ldy $26
    ldz $27
    add.32 .AXYZ, $28
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    ldy #0
    sty __zp_scratch3
    stz __zp_scratch3+1
    sta ($24),y
    txa
    iny
    sta ($24),y
    lda __zp_scratch3
    iny
    sta ($24),y
    lda __zp_scratch3+1
    iny
    sta ($24),y
    lda $24
    ldx $25
    ldy $26
    ldz $27
    sta $20
    stx $21
    .loc "test_i8_arith.c", 29
    lda $20
    ldx $21
    sta $22
    stx $23
    sty $24
    stz $25
@inline_end118:
    .loc "test_i8_arith.c", 87
    lda #210
    ldx #4
    sta $20
    stx $21
    lda $20
    ldx $21
    ldy #0
    ldz #0
    sta $26
    stx $27
    sty $28
    stz $29
    lda $22
    ldx $23
    ldy $24
    ldz $25
    cmp.32 .AXYZ, $26
    bne @if_then115
    bra @if_end117
@if_then115:
    lda #24
    ldx #0
    bra @__return
@if_end117:
    .loc "test_i8_arith.c", 33
    lda #1
    sta _main__local_305
    sta _main__local_305+1
    .loc "test_i8_arith.c", 34
    lda #20
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    ldy #0
    ldz #0
    sta $22
    stx $23
    sty $24
    stz $25
    lda _main__local_305
    ldx _main__local_305+1
    ldy _main__local_305+2
    ldz _main__local_305+3
    add.32 .AXYZ, $22
    sta $26
    stx $27
    sty $28
    stz $29
    sta _main__local_305
    stx _main__local_305+1
    sty _main__local_305+2
    stz _main__local_305+3
    lda _main__local_305
    ldx _main__local_305+1
    ldy _main__local_305+2
    ldz _main__local_305+3
    sta $20
    stx $21
    .loc "test_i8_arith.c", 35
    lda $20
    ldx $21
    sta $22
    stx $23
    sty $24
    stz $25
@inline_end123:
    .loc "test_i8_arith.c", 90
    lda #21
    ldx #1
    sta $20
    stx $21
    lda $20
    ldx $21
    ldy #0
    ldz #0
    sta $26
    stx $27
    sty $28
    stz $29
    lda $22
    ldx $23
    ldy $24
    ldz $25
    cmp.32 .AXYZ, $26
    bne @if_then120
    bra @if_end122
@if_then120:
    lda #25
    ldx #0
    bra @__return
@if_end122:
    .loc "test_i8_arith.c", 92
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc


__zp_save_buf:

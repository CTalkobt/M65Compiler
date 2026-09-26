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

    .global _test_long_plus_int
    .global _test_long_minus_int
    .global _test_long_plus_char
    .global _test_int_plus_char
    .global _test_int_minus_char
    .global _test_char_plus_char
    .global _test_long_and_int
    .global _test_long_or_int
    .global _test_int_shl_char
    .global _test_int_shr_char
    .global _add_long_char
    .global _add_long_int
    .global _main

    .segment "code"

; function _test_long_plus_int

    ; Static buffer for struct return from _test_long_plus_int
    _test_long_plus_int__struct_buf:
    .byte 0, 0, 0, 0

; SAC inline storage: 6 bytes
    _test_long_plus_int__local_0: .long 0
    _test_long_plus_int__local_2: .word 0
    proc _test_long_plus_int
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_compound_types.c", 4
    .local @_l_x = 2
    .local @_l_y = 0
; .debug_var: __test_long_plus_int @_l_x offset=2 size=4 type=int32 scope=local
; .debug_var: __test_long_plus_int @_l_y offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_compound_types.c", 5
    lda #232
    sta _test_long_plus_int__local_0
    lda #3
    sta _test_long_plus_int__local_0+1
    .loc "test_compound_types.c", 6
    lda #234
    sta _test_long_plus_int__local_2
    lda #0
    sta _test_long_plus_int__local_2+1
    lda _test_long_plus_int__local_2
    ldx _test_long_plus_int__local_2+1
    ldy #0
    ldz #0
    sta $20
    stx $21
    sty $22
    stz $23
    .loc "test_compound_types.c", 7
    lda _test_long_plus_int__local_0
    ldx _test_long_plus_int__local_0+1
    ldy _test_long_plus_int__local_0+2
    ldz _test_long_plus_int__local_0+3
    add.32 .AXYZ, $20
    sta $24
    stx $25
    sty $26
    stz $27
    sta _test_long_plus_int__local_0
    stx _test_long_plus_int__local_0+1
    sty _test_long_plus_int__local_0+2
    stz _test_long_plus_int__local_0+3
    .loc "test_compound_types.c", 8
    lda _test_long_plus_int__local_0
    ldx _test_long_plus_int__local_0+1
    ldy _test_long_plus_int__local_0+2
    ldz _test_long_plus_int__local_0+3
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _test_long_minus_int

    ; Static buffer for struct return from _test_long_minus_int
    _test_long_minus_int__struct_buf:
    .byte 0, 0, 0, 0

; SAC inline storage: 6 bytes
    _test_long_minus_int__local_0: .long 0
    _test_long_minus_int__local_2: .word 0
    proc _test_long_minus_int
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_compound_types.c", 12
    .local @_l_x = 2
    .local @_l_y = 0
; .debug_var: __test_long_minus_int @_l_x offset=2 size=4 type=int32 scope=local
; .debug_var: __test_long_minus_int @_l_y offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_compound_types.c", 13
    lda #232
    sta _test_long_minus_int__local_0
    lda #3
    sta _test_long_minus_int__local_0+1
    .loc "test_compound_types.c", 14
    lda #234
    sta _test_long_minus_int__local_2
    lda #0
    sta _test_long_minus_int__local_2+1
    lda _test_long_minus_int__local_2
    ldx _test_long_minus_int__local_2+1
    ldy #0
    ldz #0
    sta $20
    stx $21
    sty $22
    stz $23
    .loc "test_compound_types.c", 15
    lda _test_long_minus_int__local_0
    ldx _test_long_minus_int__local_0+1
    ldy _test_long_minus_int__local_0+2
    ldz _test_long_minus_int__local_0+3
    sub.32 .AXYZ, $20
    sta $24
    stx $25
    sty $26
    stz $27
    sta _test_long_minus_int__local_0
    stx _test_long_minus_int__local_0+1
    sty _test_long_minus_int__local_0+2
    stz _test_long_minus_int__local_0+3
    .loc "test_compound_types.c", 16
    lda _test_long_minus_int__local_0
    ldx _test_long_minus_int__local_0+1
    ldy _test_long_minus_int__local_0+2
    ldz _test_long_minus_int__local_0+3
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _test_long_plus_char

    ; Static buffer for struct return from _test_long_plus_char
    _test_long_plus_char__struct_buf:
    .byte 0, 0, 0, 0

; SAC inline storage: 6 bytes
    _test_long_plus_char__local_0: .long 0
    _test_long_plus_char__local_2: .word 0
    proc _test_long_plus_char
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_compound_types.c", 20
    .local @_l_c = 0
    .local @_l_x = 2
; .debug_var: __test_long_plus_char @_l_c offset=0 size=2 type=int8 scope=local
; .debug_var: __test_long_plus_char @_l_x offset=2 size=4 type=int32 scope=local

@entry:
    .loc "test_compound_types.c", 21
    lda #0
    sta _test_long_plus_char__local_0
    lda #1
    sta _test_long_plus_char__local_0+1
    .loc "test_compound_types.c", 22
    lda #1
    sta _test_long_plus_char__local_2
    lda #0
    sta _test_long_plus_char__local_2+1
    lda _test_long_plus_char__local_2
    ldx #0
    ldx #0
    ldy #0
    ldz #0
    sta $20
    stx $21
    sty $22
    stz $23
    .loc "test_compound_types.c", 23
    lda _test_long_plus_char__local_0
    ldx _test_long_plus_char__local_0+1
    ldy _test_long_plus_char__local_0+2
    ldz _test_long_plus_char__local_0+3
    add.32 .AXYZ, $20
    sta $24
    stx $25
    sty $26
    stz $27
    sta _test_long_plus_char__local_0
    stx _test_long_plus_char__local_0+1
    sty _test_long_plus_char__local_0+2
    stz _test_long_plus_char__local_0+3
    .loc "test_compound_types.c", 24
    lda _test_long_plus_char__local_0
    ldx _test_long_plus_char__local_0+1
    ldy _test_long_plus_char__local_0+2
    ldz _test_long_plus_char__local_0+3
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _test_int_plus_char
; SAC inline storage: 4 bytes
    _test_int_plus_char__local_0: .word 0
    _test_int_plus_char__local_2: .word 0
    proc _test_int_plus_char
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_compound_types.c", 28
    .local @_l_c = 2
    .local @_l_x = 0
; .debug_var: __test_int_plus_char @_l_c offset=2 size=2 type=int8 scope=local
; .debug_var: __test_int_plus_char @_l_x offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_compound_types.c", 29
    lda #232
    sta _test_int_plus_char__local_0
    lda #3
    sta _test_int_plus_char__local_0+1
    .loc "test_compound_types.c", 30
    lda #200
    sta _test_int_plus_char__local_2
    lda #0
    sta _test_int_plus_char__local_2+1
    lda _test_int_plus_char__local_2
    ldx #0
    sta $20
    stx $21
    .loc "test_compound_types.c", 31
    lda _test_int_plus_char__local_0
    ldx _test_int_plus_char__local_0+1
    add.16 .AX, $20
    sta $22
    stx $23
    sta _test_int_plus_char__local_0
    stx _test_int_plus_char__local_0+1
    .loc "test_compound_types.c", 32
    lda _test_int_plus_char__local_0
    ldx _test_int_plus_char__local_0+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _test_int_minus_char
; SAC inline storage: 4 bytes
    _test_int_minus_char__local_0: .word 0
    _test_int_minus_char__local_2: .word 0
    proc _test_int_minus_char
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_compound_types.c", 36
    .local @_l_c = 2
    .local @_l_x = 0
; .debug_var: __test_int_minus_char @_l_c offset=2 size=2 type=int8 scope=local
; .debug_var: __test_int_minus_char @_l_x offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_compound_types.c", 37
    lda #232
    sta _test_int_minus_char__local_0
    lda #3
    sta _test_int_minus_char__local_0+1
    .loc "test_compound_types.c", 38
    lda #50
    sta _test_int_minus_char__local_2
    lda #0
    sta _test_int_minus_char__local_2+1
    lda _test_int_minus_char__local_2
    ldx #0
    sta $20
    stx $21
    .loc "test_compound_types.c", 39
    lda _test_int_minus_char__local_0
    ldx _test_int_minus_char__local_0+1
    sub.16 .AX, $20
    sta $22
    stx $23
    sta _test_int_minus_char__local_0
    stx _test_int_minus_char__local_0+1
    .loc "test_compound_types.c", 40
    lda _test_int_minus_char__local_0
    ldx _test_int_minus_char__local_0+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _test_char_plus_char
; SAC inline storage: 4 bytes
    _test_char_plus_char__local_0: .word 0
    _test_char_plus_char__local_2: .word 0
    proc _test_char_plus_char
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_compound_types.c", 44
    .local @_l_a = 0
    .local @_l_b = 2
; .debug_var: __test_char_plus_char @_l_a offset=0 size=2 type=int8 scope=local
; .debug_var: __test_char_plus_char @_l_b offset=2 size=2 type=int8 scope=local

@entry:
    .loc "test_compound_types.c", 45
    lda #200
    sta _test_char_plus_char__local_0
    lda #0
    sta _test_char_plus_char__local_0+1
    .loc "test_compound_types.c", 46
    lda #100
    sta _test_char_plus_char__local_2
    lda #0
    sta _test_char_plus_char__local_2+1
    .loc "test_compound_types.c", 47
    lda _test_char_plus_char__local_0
    lda _test_char_plus_char__local_2
    sta __zp_scratch2
    stx __zp_scratch2+1
    clc
    adc __zp_scratch2
    sta $20
    sta _test_char_plus_char__local_0
    lda _test_char_plus_char__local_0
    ldx #0
    sta $20
    stx $21
    .loc "test_compound_types.c", 49
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _test_long_and_int

    ; Static buffer for struct return from _test_long_and_int
    _test_long_and_int__struct_buf:
    .byte 0, 0, 0, 0

; SAC inline storage: 6 bytes
    _test_long_and_int__local_0: .long 0
    _test_long_and_int__local_2: .word 0
    proc _test_long_and_int
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_compound_types.c", 53
    .local @_l_mask = 0
    .local @_l_x = 2
; .debug_var: __test_long_and_int @_l_mask offset=0 size=2 type=int16 scope=local
; .debug_var: __test_long_and_int @_l_x offset=2 size=4 type=int32 scope=local

@entry:
    .loc "test_compound_types.c", 54
    lda #255
    sta _test_long_and_int__local_0
    lda #0
    sta _test_long_and_int__local_0+1
    .loc "test_compound_types.c", 55
    lda #15
    sta _test_long_and_int__local_2
    lda #0
    sta _test_long_and_int__local_2+1
    lda _test_long_and_int__local_2
    ldx _test_long_and_int__local_2+1
    ldy #0
    ldz #0
    sta $20
    stx $21
    sty $22
    stz $23
    .loc "test_compound_types.c", 56
    lda _test_long_and_int__local_0
    ldx _test_long_and_int__local_0+1
    ldy _test_long_and_int__local_0+2
    ldz _test_long_and_int__local_0+3
    and.32 .AXYZ, $20
    sta $24
    stx $25
    sty $26
    stz $27
    lda $24
    ldx $25
    ldy $26
    ldz $27
    sta _test_long_and_int__local_0
    stx _test_long_and_int__local_0+1
    sty _test_long_and_int__local_0+2
    stz _test_long_and_int__local_0+3
    .loc "test_compound_types.c", 57
    lda _test_long_and_int__local_0
    ldx _test_long_and_int__local_0+1
    ldy _test_long_and_int__local_0+2
    ldz _test_long_and_int__local_0+3
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers N, Z
    .frame_size 6
    endproc

; function _test_long_or_int

    ; Static buffer for struct return from _test_long_or_int
    _test_long_or_int__struct_buf:
    .byte 0, 0, 0, 0

; SAC inline storage: 6 bytes
    _test_long_or_int__local_0: .long 0
    _test_long_or_int__local_2: .word 0
    proc _test_long_or_int
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_compound_types.c", 61
    .local @_l_bits = 0
    .local @_l_x = 2
; .debug_var: __test_long_or_int @_l_bits offset=0 size=2 type=int16 scope=local
; .debug_var: __test_long_or_int @_l_x offset=2 size=4 type=int32 scope=local

@entry:
    .loc "test_compound_types.c", 62
    lda #0
    sta _test_long_or_int__local_0
    lda #1
    sta _test_long_or_int__local_0+1
    .loc "test_compound_types.c", 63
    lda #15
    sta _test_long_or_int__local_2
    lda #0
    sta _test_long_or_int__local_2+1
    lda _test_long_or_int__local_2
    ldx _test_long_or_int__local_2+1
    ldy #0
    ldz #0
    sta $20
    stx $21
    sty $22
    stz $23
    .loc "test_compound_types.c", 64
    lda _test_long_or_int__local_0
    ldx _test_long_or_int__local_0+1
    ldy _test_long_or_int__local_0+2
    ldz _test_long_or_int__local_0+3
    ora.32 .AXYZ, $20
    sta $24
    stx $25
    sty $26
    stz $27
    lda $24
    ldx $25
    ldy $26
    ldz $27
    sta _test_long_or_int__local_0
    stx _test_long_or_int__local_0+1
    sty _test_long_or_int__local_0+2
    stz _test_long_or_int__local_0+3
    .loc "test_compound_types.c", 65
    lda _test_long_or_int__local_0
    ldx _test_long_or_int__local_0+1
    ldy _test_long_or_int__local_0+2
    ldz _test_long_or_int__local_0+3
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers N, Z
    .frame_size 6
    endproc

; function _test_int_shl_char
; SAC inline storage: 4 bytes
    _test_int_shl_char__local_0: .word 0
    _test_int_shl_char__local_2: .word 0
    proc _test_int_shl_char
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_compound_types.c", 69
    .local @_l_n = 2
    .local @_l_x = 0
; .debug_var: __test_int_shl_char @_l_n offset=2 size=2 type=int8 scope=local
; .debug_var: __test_int_shl_char @_l_x offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_compound_types.c", 70
    lda #1
    sta _test_int_shl_char__local_0
    lda #0
    sta _test_int_shl_char__local_0+1
    .loc "test_compound_types.c", 71
    lda #8
    sta _test_int_shl_char__local_2
    lda #0
    sta _test_int_shl_char__local_2+1
    lda _test_int_shl_char__local_2
    ldx #0
    sta $20
    stx $21
    .loc "test_compound_types.c", 72
    lda _test_int_shl_char__local_0
    ldx _test_int_shl_char__local_0+1
    ldy $20
    beq @__shl_done_1
@__shl_loop_0:
    lsl.16 .AX
    dey
    bne @__shl_loop_0
@__shl_done_1:
    sta $22
    stx $23
    lda $22
    ldx $23
    sta _test_int_shl_char__local_0
    stx _test_int_shl_char__local_0+1
    .loc "test_compound_types.c", 73
    lda _test_int_shl_char__local_0
    ldx _test_int_shl_char__local_0+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z
    .frame_size 4
    endproc

; function _test_int_shr_char
; SAC inline storage: 4 bytes
    _test_int_shr_char__local_0: .word 0
    _test_int_shr_char__local_2: .word 0
    proc _test_int_shr_char
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_compound_types.c", 77
    .local @_l_n = 2
    .local @_l_x = 0
; .debug_var: __test_int_shr_char @_l_n offset=2 size=2 type=int8 scope=local
; .debug_var: __test_int_shr_char @_l_x offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_compound_types.c", 78
    lda #0
    sta _test_int_shr_char__local_0
    lda #1
    sta _test_int_shr_char__local_0+1
    .loc "test_compound_types.c", 79
    lda #4
    sta _test_int_shr_char__local_2
    lda #0
    sta _test_int_shr_char__local_2+1
    lda _test_int_shr_char__local_2
    ldx #0
    sta $20
    stx $21
    .loc "test_compound_types.c", 80
    lda _test_int_shr_char__local_0
    ldx _test_int_shr_char__local_0+1
    ldy $20
    beq @__shr_done_3
@__shr_loop_2:
    lsr.16 .AX
    dey
    bne @__shr_loop_2
@__shr_done_3:
    sta $22
    stx $23
    lda $22
    ldx $23
    sta _test_int_shr_char__local_0
    stx _test_int_shr_char__local_0+1
    .loc "test_compound_types.c", 81
    lda _test_int_shr_char__local_0
    ldx _test_int_shr_char__local_0+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z
    .frame_size 4
    endproc

; function _add_long_char

    ; Static buffer for struct return from _add_long_char
    _add_long_char__struct_buf:
    .byte 0, 0, 0, 0

; SAC inline storage: 6 bytes
    .global _add_long_char__param_a
    _add_long_char__param_a: .long 0
    .global _add_long_char__param_b
    _add_long_char__param_b: .word 0
    _add_long_char__local_0: .long 0
    _add_long_char__local_1: .word 0
    proc _add_long_char, D#@_p_a, B#@_p_b
    .sac
    .var _fp = 0
    .loc "test_compound_types.c", 85
    .var @_p_a = 2
    .var @_p_b = 6
; .debug_var: __add_long_char @_p_a offset=2 size=4 type=int32 scope=parameter
; .debug_var: __add_long_char @_p_b offset=6 size=2 type=int8 scope=parameter

@entry:
    lda _add_long_char__param_b
    ldx #0
    ldx #0
    ldy #0
    ldz #0
    sta $20
    stx $21
    sty $22
    stz $23
    .loc "test_compound_types.c", 86
    lda _add_long_char__param_a
    ldx _add_long_char__param_a+1
    ldy _add_long_char__param_a+2
    ldz _add_long_char__param_a+3
    add.32 .AXYZ, $20
    sta $24
    stx $25
    sty $26
    stz $27
    lda $24
    ldx $25
    ldy $26
    ldz $27
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _add_long_int

    ; Static buffer for struct return from _add_long_int
    _add_long_int__struct_buf:
    .byte 0, 0, 0, 0

; SAC inline storage: 6 bytes
    .global _add_long_int__param_a
    _add_long_int__param_a: .long 0
    .global _add_long_int__param_b
    _add_long_int__param_b: .word 0
    _add_long_int__local_0: .long 0
    _add_long_int__local_1: .word 0
    proc _add_long_int, D#@_p_a, W#@_p_b
    .sac
    .var _fp = 0
    .loc "test_compound_types.c", 89
    .var @_p_a = 2
    .var @_p_b = 6
; .debug_var: __add_long_int @_p_a offset=2 size=4 type=int32 scope=parameter
; .debug_var: __add_long_int @_p_b offset=6 size=2 type=int16 scope=parameter

@entry:
    lda _add_long_int__param_b
    ldx _add_long_int__param_b+1
    ldy #0
    ldz #0
    sta $20
    stx $21
    sty $22
    stz $23
    .loc "test_compound_types.c", 90
    lda _add_long_int__param_a
    ldx _add_long_int__param_a+1
    ldy _add_long_int__param_a+2
    ldz _add_long_int__param_a+3
    add.32 .AXYZ, $20
    sta $24
    stx $25
    sty $26
    stz $27
    lda $24
    ldx $25
    ldy $26
    ldz $27
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _main
; SAC inline storage: 44 bytes
    _main__local_1: .long 0
    _main__local_12: .long 0
    _main__local_14: .word 0
    _main__local_23: .long 0
    _main__local_45: .word 0
    _main__local_52: .word 0
    _main__local_54: .word 0
    _main__local_61: .long 0
    _main__local_63: .word 0
    _main__local_72: .long 0
    _main__local_74: .word 0
    _main__local_92: .word 0
    _main__local_94: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_compound_types.c", 93
    .local @_l_a = 20
    .local @_l_b = 22
    .local @_l_bits = 30
    .local @_l_c = 18
    .local @_l_mask = 26
    .local @_l_n = 38
    .local @_l_x = 36
    .local @_l_y = 6
; .debug_var: __main @_l_a offset=20 size=2 type=int8 scope=local
; .debug_var: __main @_l_b offset=22 size=2 type=int8 scope=local
; .debug_var: __main @_l_bits offset=30 size=2 type=int16 scope=local
; .debug_var: __main @_l_c offset=18 size=2 type=int8 scope=local
; .debug_var: __main @_l_mask offset=26 size=2 type=int16 scope=local
; .debug_var: __main @_l_n offset=38 size=2 type=int8 scope=local
; .debug_var: __main @_l_x offset=36 size=2 type=int16 scope=local
; .debug_var: __main @_l_y offset=6 size=2 type=int16 scope=local

@entry:
    .loc "test_compound_types.c", 5
    lda #232
    sta _main__local_1
    lda #3
    sta _main__local_1+1
    .loc "test_compound_types.c", 6
    lda #234
    sta $22
    ldx #0
    stx $23
    lda $22
    ldx $23
    ldy #0
    ldz #0
    sta $24
    stx $25
    sty $26
    stz $27
    .loc "test_compound_types.c", 7
    lda _main__local_1
    ldx _main__local_1+1
    ldy _main__local_1+2
    ldz _main__local_1+3
    add.32 .AXYZ, $24
    sta $20
    stx $21
    sty $22
    stz $23
    sta _main__local_1
    stx _main__local_1+1
    sty _main__local_1+2
    stz _main__local_1+3
    lda _main__local_1
    ldx _main__local_1+1
    ldy _main__local_1+2
    ldz _main__local_1+3
    sta $20
    stx $21
    .loc "test_compound_types.c", 8
    lda $20
    ldx $21
    sta $22
    stx $23
    sty $24
    stz $25
@inline_end3:
    .loc "test_compound_types.c", 95
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
    bne @if_then0
    bra @if_end2
@if_then0:
    lda #1
    ldx #0
    bra @__return
@if_end2:
    .loc "test_compound_types.c", 13
    lda #232
    sta _main__local_12
    lda #3
    sta _main__local_12+1
    .loc "test_compound_types.c", 14
    lda #234
    sta _main__local_14
    lda #0
    sta _main__local_14+1
    lda _main__local_14
    ldx _main__local_14+1
    ldy #0
    ldz #0
    sta $20
    stx $21
    sty $22
    stz $23
    .loc "test_compound_types.c", 15
    lda _main__local_12
    ldx _main__local_12+1
    ldy _main__local_12+2
    ldz _main__local_12+3
    sub.32 .AXYZ, $20
    sta $24
    stx $25
    sty $26
    stz $27
    sta _main__local_12
    stx _main__local_12+1
    sty _main__local_12+2
    stz _main__local_12+3
    lda _main__local_12
    ldx _main__local_12+1
    ldy _main__local_12+2
    ldz _main__local_12+3
    sta $20
    stx $21
    .loc "test_compound_types.c", 16
    lda $20
    ldx $21
    sta $22
    stx $23
    sty $24
    stz $25
@inline_end8:
    .loc "test_compound_types.c", 98
    lda #254
    ldx #2
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
    bne @if_then5
    bra @if_end7
@if_then5:
    lda #2
    ldx #0
    bra @__return
@if_end7:
    .loc "test_compound_types.c", 21
    lda #0
    sta _main__local_23
    lda #1
    sta _main__local_23+1
    .loc "test_compound_types.c", 22
    lda #1
    sta $22
    ldx #0
    stx $23
    lda $22
    ldx #0
    ldx #0
    ldx #0
    ldy #0
    ldz #0
    sta $24
    stx $25
    sty $26
    stz $27
    .loc "test_compound_types.c", 23
    lda _main__local_23
    ldx _main__local_23+1
    ldy _main__local_23+2
    ldz _main__local_23+3
    add.32 .AXYZ, $24
    sta $20
    stx $21
    sty $22
    stz $23
    sta _main__local_23
    stx _main__local_23+1
    sty _main__local_23+2
    stz _main__local_23+3
    lda _main__local_23
    ldx _main__local_23+1
    ldy _main__local_23+2
    ldz _main__local_23+3
    sta $20
    stx $21
    .loc "test_compound_types.c", 24
    lda $20
    ldx $21
    sta $22
    stx $23
    sty $24
    stz $25
@inline_end13:
    .loc "test_compound_types.c", 101
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
    bne @if_then10
    bra @if_end12
@if_then10:
    lda #3
    ldx #0
    bra @__return
@if_end12:
    .loc "test_compound_types.c", 29
    lda #232
    sta $22
    ldx #3
    stx $23
    .loc "test_compound_types.c", 30
    lda #200
    sta $24
    ldx #0
    stx $25
    lda $24
    ldx #0
    ldx #0
    sta $20
    stx $21
    .loc "test_compound_types.c", 31
    lda $22
    clc
    adc $20
    sta $22
    lda $23
    adc $20+1
    sta $23
    .loc "test_compound_types.c", 32
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end18:
    .loc "test_compound_types.c", 104
    lda $20
    ldx $21
    cmp.16 .AX, #1200
    bne @if_then15
    bra @if_end17
@if_then15:
    lda #4
    ldx #0
    bra @__return
@if_end17:
    .loc "test_compound_types.c", 37
    lda #232
    sta $22
    ldx #3
    stx $23
    .loc "test_compound_types.c", 38
    lda #50
    sta _main__local_45
    lda #0
    sta _main__local_45+1
    lda _main__local_45
    ldx #0
    sta $20
    stx $21
    .loc "test_compound_types.c", 39
    lda $22
    sec
    sbc $20
    sta $22
    lda $23
    sbc $20+1
    sta $23
    .loc "test_compound_types.c", 40
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end23:
    .loc "test_compound_types.c", 107
    lda $20
    ldx $21
    cmp.16 .AX, #950
    bne @if_then20
    bra @if_end22
@if_then20:
    lda #5
    ldx #0
    bra @__return
@if_end22:
    .loc "test_compound_types.c", 45
    lda #200
    sta _main__local_52
    lda #0
    sta _main__local_52+1
    .loc "test_compound_types.c", 46
    lda #100
    sta _main__local_54
    lda #0
    sta _main__local_54+1
    .loc "test_compound_types.c", 47
    lda _main__local_52
    lda _main__local_54
    sta __zp_scratch2
    stx __zp_scratch2+1
    clc
    adc __zp_scratch2
    sta $20
    sta _main__local_52
    lda _main__local_52
    ldx #0
    sta $20
    stx $21
    .loc "test_compound_types.c", 49
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end28:
    .loc "test_compound_types.c", 110
    lda $22
    ldx $23
    cmp.16 .AX, #44
    bne @if_then25
    bra @if_end27
@if_then25:
    lda #6
    ldx #0
    bra @__return
@if_end27:
    .loc "test_compound_types.c", 54
    lda #255
    sta _main__local_61
    lda #0
    sta _main__local_61+1
    .loc "test_compound_types.c", 55
    lda #15
    sta _main__local_63
    lda #0
    sta _main__local_63+1
    lda _main__local_63
    ldx _main__local_63+1
    ldy #0
    ldz #0
    sta $20
    stx $21
    sty $22
    stz $23
    .loc "test_compound_types.c", 56
    lda _main__local_61
    ldx _main__local_61+1
    ldy _main__local_61+2
    ldz _main__local_61+3
    and.32 .AXYZ, $20
    sta $24
    stx $25
    sty $26
    stz $27
    lda $24
    ldx $25
    ldy $26
    ldz $27
    sta _main__local_61
    stx _main__local_61+1
    sty _main__local_61+2
    stz _main__local_61+3
    lda _main__local_61
    ldx _main__local_61+1
    ldy _main__local_61+2
    ldz _main__local_61+3
    sta $20
    stx $21
    .loc "test_compound_types.c", 57
    lda $20
    ldx $21
    sta $22
    stx $23
    sty $24
    stz $25
@inline_end33:
    .loc "test_compound_types.c", 113
    lda #15
    ldx #0
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
    bne @if_then30
    bra @if_end32
@if_then30:
    lda #7
    ldx #0
    bra @__return
@if_end32:
    .loc "test_compound_types.c", 62
    lda #0
    sta _main__local_72
    lda #1
    sta _main__local_72+1
    .loc "test_compound_types.c", 63
    lda #15
    sta _main__local_74
    lda #0
    sta _main__local_74+1
    lda _main__local_74
    ldx _main__local_74+1
    ldy #0
    ldz #0
    sta $20
    stx $21
    sty $22
    stz $23
    .loc "test_compound_types.c", 64
    lda _main__local_72
    ldx _main__local_72+1
    ldy _main__local_72+2
    ldz _main__local_72+3
    ora.32 .AXYZ, $20
    sta $24
    stx $25
    sty $26
    stz $27
    lda $24
    ldx $25
    ldy $26
    ldz $27
    sta _main__local_72
    stx _main__local_72+1
    sty _main__local_72+2
    stz _main__local_72+3
    lda _main__local_72
    ldx _main__local_72+1
    ldy _main__local_72+2
    ldz _main__local_72+3
    sta $20
    stx $21
    .loc "test_compound_types.c", 65
    lda $20
    ldx $21
    sta $22
    stx $23
    sty $24
    stz $25
@inline_end38:
    .loc "test_compound_types.c", 116
    lda #15
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
    bne @if_then35
    bra @if_end37
@if_then35:
    lda #8
    ldx #0
    bra @__return
@if_end37:
    .loc "test_compound_types.c", 70
    lda #1
    sta $22
    ldx #0
    stx $23
    .loc "test_compound_types.c", 71
    lda #8
    sta $24
    ldx #0
    stx $25
    lda $24
    ldx #0
    ldx #0
    sta $20
    stx $21
    .loc "test_compound_types.c", 72
    lda $22
    ldx $23
    ldy $20
    beq @__shl_done_5
@__shl_loop_4:
    lsl.16 .AX
    dey
    bne @__shl_loop_4
@__shl_done_5:
    sta $24
    stx $25
    lda $24
    ldx $25
    sta $22
    stx $23
    .loc "test_compound_types.c", 73
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end43:
    .loc "test_compound_types.c", 119
    lda $20
    ldx $21
    cmp.16 .AX, #256
    bne @if_then40
    bra @if_end42
@if_then40:
    lda #9
    ldx #0
    bra @__return
@if_end42:
    .loc "test_compound_types.c", 78
    lda #0
    sta _main__local_92
    lda #1
    sta _main__local_92+1
    .loc "test_compound_types.c", 79
    lda #4
    sta _main__local_94
    lda #0
    sta _main__local_94+1
    lda _main__local_94
    ldx #0
    sta $20
    stx $21
    .loc "test_compound_types.c", 80
    lda _main__local_92
    ldx _main__local_92+1
    ldy $20
    beq @__shr_done_7
@__shr_loop_6:
    lsr.16 .AX
    dey
    bne @__shr_loop_6
@__shr_done_7:
    sta $22
    stx $23
    lda $22
    ldx $23
    sta _main__local_92
    stx _main__local_92+1
    .loc "test_compound_types.c", 81
    lda _main__local_92
    ldx _main__local_92+1
    sta $20
    stx $21
@inline_end48:
    .loc "test_compound_types.c", 122
    lda $20
    ldx $21
    cmp.16 .AX, #16
    bne @if_then45
    bra @if_end47
@if_then45:
    lda #10
    ldx #0
    bra @__return
@if_end47:
    .loc "test_compound_types.c", 125
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
    .loc "test_compound_types.c", 86
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
    sta $24
    stx $25
    lda $24
    ldx $25
    sta $20
    stx $21
    sty $22
    stz $23
@inline_end53:
    .loc "test_compound_types.c", 125
    lda #1
    sta $24
    sta $25
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
    bne @if_then50
    bra @if_end52
@if_then50:
    lda #11
    ldx #0
    bra @__return
@if_end52:
    .loc "test_compound_types.c", 126
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
    .loc "test_compound_types.c", 86
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
    sta $24
    stx $25
    lda $24
    ldx $25
    sta $20
    stx $21
    sty $22
    stz $23
@inline_end58:
    .loc "test_compound_types.c", 126
    lda #210
    ldx #4
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
    bne @if_then55
    bra @if_end57
@if_then55:
    lda #12
    ldx #0
    bra @__return
@if_end57:
    .loc "test_compound_types.c", 129
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
    lda $24
    ldx $25
    ldy $26
    ldz $27
    sta $28
    stx $29
    sty $2A
    stz $2B
    lda $22
    ldx $23
    sta $20
    stx $21
    lda $22
    ldx $23
    ldy #0
    ldz #0
    sta $28
    stx $29
    sty $2A
    stz $2B
    .loc "test_compound_types.c", 90
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
    sta $24
    stx $25
    lda $24
    ldx $25
    sta $20
    stx $21
    sty $22
    stz $23
@inline_end63:
    .loc "test_compound_types.c", 129
    lda #1
    sta $24
    sta $25
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
    bne @if_then60
    bra @if_end62
@if_then60:
    lda #13
    ldx #0
    bra @__return
@if_end62:
    .loc "test_compound_types.c", 130
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
    lda $24
    ldx $25
    ldy $26
    ldz $27
    sta $28
    stx $29
    sty $2A
    stz $2B
    lda $22
    ldx $23
    sta $20
    stx $21
    lda $22
    ldx $23
    ldy #0
    ldz #0
    sta $28
    stx $29
    sty $2A
    stz $2B
    .loc "test_compound_types.c", 90
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
    sta $24
    stx $25
    lda $24
    ldx $25
    sta $20
    stx $21
    sty $22
    stz $23
@inline_end68:
    .loc "test_compound_types.c", 130
    lda #210
    ldx #4
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
    bne @if_then65
    bra @if_end67
@if_then65:
    lda #14
    ldx #0
    bra @__return
@if_end67:
    .loc "test_compound_types.c", 132
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 44
    endproc


__zp_save_buf:

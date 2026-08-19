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
    .global _inner_clobber
    .global _nested_test
    .global _level3
    .global _level2
    .global _level1
    .global _write_through_ptr
    .global _addr_of_param
    .global _read_through_ptr
    .global _addr_of_read
    .global _add_long
    .global _factorial
    .global _is_even
    .global _is_odd
    .global _sum4
    .global _char_add
    .global _main

    .segment "data"
    .byte 0
_results:
; .debug_var: @global _results offset=0 size=2 type=ptr scope=global
    .word 16384

    .segment "code"

; function _inner_clobber
; SAC inline storage: 4 bytes
    .global _inner_clobber__param_a
    _inner_clobber__param_a: .word 0
    .global _inner_clobber__param_b
    _inner_clobber__param_b: .word 0
    _inner_clobber__local_0: .word 0
    _inner_clobber__local_1: .word 0
    proc _inner_clobber, W#@_p_a, W#@_p_b
    .sac
    .var _fp = 0
    .loc "test_zpcall_regression.c", 8
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __inner_clobber @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __inner_clobber @_p_b offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_zpcall_regression.c", 9
    lda _inner_clobber__param_b
    ldx _inner_clobber__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _inner_clobber__param_a
    ldx _inner_clobber__param_a+1
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

; function _nested_test
; SAC inline storage: 8 bytes
    .global _nested_test__param_x
    _nested_test__param_x: .word 0
    .global _nested_test__param_y
    _nested_test__param_y: .word 0
    .global _nested_test__param_z
    _nested_test__param_z: .word 0
    _nested_test__local_0: .word 0
    _nested_test__local_1: .word 0
    _nested_test__local_2: .word 0
    _nested_test__local_3: .word 0
    proc _nested_test, W#@_p_x, W#@_p_y, W#@_p_z
    .sac
    .var _fp = 0
    .loc "test_zpcall_regression.c", 12
    .local @_l_product = 6
; .debug_var: __nested_test @_l_product offset=6 size=2 type=int16 scope=local
    .var @_p_x = 2
    .var @_p_y = 4
    .var @_p_z = 6
; .debug_var: __nested_test @_p_x offset=2 size=2 type=int16 scope=parameter
; .debug_var: __nested_test @_p_y offset=4 size=2 type=int16 scope=parameter
; .debug_var: __nested_test @_p_z offset=6 size=2 type=int16 scope=parameter

@entry:
    .loc "test_zpcall_regression.c", 13
    lda _nested_test__param_x
    ldx _nested_test__param_x+1
    sta $20
    stx $21
    lda _nested_test__param_y
    ldx _nested_test__param_y+1
    sta $20
    stx $21
    .loc "test_zpcall_regression.c", 9
    lda _nested_test__param_y
    ldx _nested_test__param_y+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _nested_test__param_x
    ldx _nested_test__param_x+1
    mul.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end0:
    .loc "test_zpcall_regression.c", 13
    lda $22
    ldx $23
    sta _nested_test__local_3
    stx _nested_test__local_3+1
    .loc "test_zpcall_regression.c", 14
    lda _nested_test__param_z
    ldx _nested_test__param_z+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _nested_test__local_3
    ldx _nested_test__local_3+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 8
    endproc

; function _level3
; SAC inline storage: 2 bytes
    .global _level3__param_a
    _level3__param_a: .word 0
    _level3__local_0: .word 0
    proc _level3, W#@_p_a
    .sac
    .var _fp = 0
    .loc "test_zpcall_regression.c", 18
    .var @_p_a = 2
; .debug_var: __level3 @_p_a offset=2 size=2 type=int16 scope=parameter

@entry:
    .loc "test_zpcall_regression.c", 19
    lda _level3__param_a
    ldx _level3__param_a+1
    add.16 .AX, #1
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

; function _level2
; SAC inline storage: 6 bytes
    .global _level2__param_a
    _level2__param_a: .word 0
    .global _level2__param_b
    _level2__param_b: .word 0
    _level2__local_0: .word 0
    _level2__local_1: .word 0
    _level2__local_2: .word 0
    proc _level2, W#@_p_a, W#@_p_b
    .sac
    .var _fp = 0
    .loc "test_zpcall_regression.c", 22
    .local @_l_r = 4
; .debug_var: __level2 @_l_r offset=4 size=2 type=int16 scope=local
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __level2 @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __level2 @_p_b offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_zpcall_regression.c", 23
    lda _level2__param_a
    ldx _level2__param_a+1
    sta $20
    stx $21
    .loc "test_zpcall_regression.c", 19
    lda _level2__param_a
    ldx _level2__param_a+1
    add.16 .AX, #1
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end2:
    .loc "test_zpcall_regression.c", 23
    lda $20
    ldx $21
    sta _level2__local_2
    stx _level2__local_2+1
    .loc "test_zpcall_regression.c", 24
    lda _level2__param_b
    ldx _level2__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _level2__local_2
    ldx _level2__local_2+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _level1
; SAC inline storage: 10 bytes
    .global _level1__param_a
    _level1__param_a: .word 0
    .global _level1__param_b
    _level1__param_b: .word 0
    .global _level1__param_c
    _level1__param_c: .word 0
    _level1__local_0: .word 0
    _level1__local_1: .word 0
    _level1__local_2: .word 0
    _level1__local_7: .word 0
    proc _level1, W#@_p_a, W#@_p_b, W#@_p_c
    .sac
    .var _fp = 0
    .loc "test_zpcall_regression.c", 27
    .local @_l_r = 8
; .debug_var: __level1 @_l_r offset=8 size=2 type=int16 scope=local
    .var @_p_a = 2
    .var @_p_b = 4
    .var @_p_c = 6
; .debug_var: __level1 @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __level1 @_p_b offset=4 size=2 type=int16 scope=parameter
; .debug_var: __level1 @_p_c offset=6 size=2 type=int16 scope=parameter

@entry:
    .loc "test_zpcall_regression.c", 28
    lda _level1__param_a
    ldx _level1__param_a+1
    sta $20
    stx $21
    lda _level1__param_b
    ldx _level1__param_b+1
    sta $20
    stx $21
    .loc "test_zpcall_regression.c", 23
    lda _level1__param_a
    ldx _level1__param_a+1
    sta $22
    stx $23
    .loc "test_zpcall_regression.c", 19
    lda _level1__param_a
    ldx _level1__param_a+1
    add.16 .AX, #1
    sta $24
    stx $25
    lda $24
    ldx $25
    sta $22
    stx $23
@inline_end5:
    .loc "test_zpcall_regression.c", 23
    lda $22
    ldx $23
    sta _level1__local_7
    stx _level1__local_7+1
    .loc "test_zpcall_regression.c", 24
    lda _level1__local_7
    ldx _level1__local_7+1
    add.16 .AX, $20
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end4:
    .loc "test_zpcall_regression.c", 28
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_zpcall_regression.c", 29
    lda _level1__param_c
    ldx _level1__param_c+1
    add.16 .AX, $22
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 10
    endproc

; function _write_through_ptr
; SAC inline storage: 4 bytes
    .global _write_through_ptr__param_p
    _write_through_ptr__param_p: .word 0
    .global _write_through_ptr__param_val
    _write_through_ptr__param_val: .word 0
    _write_through_ptr__local_0: .word 0
    _write_through_ptr__local_1: .word 0
    proc _write_through_ptr, W#@_p_p, W#@_p_val
    .sac
    .var _fp = 0
    .loc "test_zpcall_regression.c", 33
    .var @_p_p = 2
    .var @_p_val = 4
; .debug_var: __write_through_ptr @_p_p offset=2 size=2 type=ptr scope=parameter
; .debug_var: __write_through_ptr @_p_val offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_zpcall_regression.c", 34
    lda _write_through_ptr__param_p
    ldx _write_through_ptr__param_p+1
    sta $20
    stx $21
    lda _write_through_ptr__param_val
    ldx _write_through_ptr__param_val+1
    ldy #0
    sta ($20),y
    txa
    iny
    sta ($20),y
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers N, Z
    .frame_size 4
    endproc

; function _addr_of_param
; SAC inline storage: 2 bytes
    .global _addr_of_param__param_x
    _addr_of_param__param_x: .word 0
    _addr_of_param__local_0: .word 0
    proc _addr_of_param, W#@_p_x
    .sac
    .var _fp = 0
    .loc "test_zpcall_regression.c", 37
    .var @_p_x = 2
; .debug_var: __addr_of_param @_p_x offset=2 size=2 type=int16 scope=parameter

@entry:
    .loc "test_zpcall_regression.c", 38
    leax.local 0
    sta $20
    stx $21
    lda #77
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
    .loc "test_zpcall_regression.c", 34
    lda $22
    ldx $23
    ldy #0
    sta ($20),y
    txa
    iny
    sta ($20),y
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z
    .frame_size 2
    endproc

; function _read_through_ptr
; SAC inline storage: 2 bytes
    .global _read_through_ptr__param_p
    _read_through_ptr__param_p: .word 0
    _read_through_ptr__local_0: .word 0
    proc _read_through_ptr, W#@_p_p
    .sac
    .var _fp = 0
    .loc "test_zpcall_regression.c", 43
    .var @_p_p = 2
; .debug_var: __read_through_ptr @_p_p offset=2 size=2 type=ptr scope=parameter

@entry:
    .loc "test_zpcall_regression.c", 44
    lda _read_through_ptr__param_p
    ldx _read_through_ptr__param_p+1
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
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers N, Z
    .frame_size 2
    endproc

; function _addr_of_read
; SAC inline storage: 4 bytes
    .global _addr_of_read__param_x
    _addr_of_read__param_x: .word 0
    _addr_of_read__local_0: .word 0
    _addr_of_read__local_1: .word 0
    proc _addr_of_read, W#@_p_x
    .sac
    .var _fp = 0
    .loc "test_zpcall_regression.c", 47
    .local @_l_val = 2
; .debug_var: __addr_of_read @_l_val offset=2 size=2 type=int16 scope=local
    .var @_p_x = 2
; .debug_var: __addr_of_read @_p_x offset=2 size=2 type=int16 scope=parameter

@entry:
    .loc "test_zpcall_regression.c", 48
    leax.local 0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_zpcall_regression.c", 44
    ldy #0
    lda ($20),y
    pha
    iny
    lda ($20),y
    tax
    pla
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end9:
    .loc "test_zpcall_regression.c", 48
    lda $20
    ldx $21
    sta _addr_of_read__local_1
    stx _addr_of_read__local_1+1
    .loc "test_zpcall_regression.c", 49
    lda _addr_of_read__local_1
    ldx _addr_of_read__local_1+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z
    .frame_size 4
    endproc

; function _add_long

    ; Static buffer for struct return from _add_long
    _add_long__struct_buf:
    .byte 0, 0, 0, 0

; SAC inline storage: 8 bytes
    .global _add_long__param_a
    _add_long__param_a: .long 0
    .global _add_long__param_b
    _add_long__param_b: .long 0
    _add_long__local_0: .long 0
    _add_long__local_1: .long 0
    proc _add_long, D#@_p_a, D#@_p_b
    .sac
    .var _fp = 0
    .loc "test_zpcall_regression.c", 53
    .var @_p_a = 2
    .var @_p_b = 6
; .debug_var: __add_long @_p_a offset=2 size=4 type=int32 scope=parameter
; .debug_var: __add_long @_p_b offset=6 size=4 type=int32 scope=parameter

@entry:
    .loc "test_zpcall_regression.c", 54
    lda _add_long__param_b
    ldx _add_long__param_b+1
    ldy _add_long__param_b+2
    ldz _add_long__param_b+3
    sta $10
    stx $11
    sty $12
    stz $13
    lda _add_long__param_a
    ldx _add_long__param_a+1
    ldy _add_long__param_a+2
    ldz _add_long__param_a+3
    add.32 .AXYZ, $10
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 8
    endproc

; function _factorial
    proc _factorial, W#@_p_n
    .var _fp = 0
    .loc "test_zpcall_regression.c", 58
; frame: 2 bytes (frame-allocated vRegs only)
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
    .local __vr0 = 0
    .local __vr5 = 2
    .local __vr6 = 4
    .local __vr11 = 6
    .var @_p_n = 4
; .debug_var: __factorial @_p_n offset=4 size=2 type=int16 scope=parameter

    ldax.param @_p_n
    stax.local __vr0
@entry:
    .loc "test_zpcall_regression.c", 59
    ldax.local __vr0
    cmp.16 .AX, #1
    bcc @if_then11
    beq @if_then11
    bra @if_end13
@if_then11:
    lda #1
    ldx #0
    bra @__return
@if_end13:
    .loc "test_zpcall_regression.c", 60
    ldax.local __vr0
    sub.16 .AX, #1
    sta $22
    stx $23
    lda $22
    ldx $23
    stax.local __vr5
    .loc "test_zpcall_regression.c", 59
    lda $22
    ldx $23
    cmp.16 .AX, #1
    bcc @if_then15
    beq @if_then15
    bra @if_end17
@if_then15:
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    stax.local __vr6
    bra @inline_end14
@if_end17:
    .loc "test_zpcall_regression.c", 60
    ldax.local __vr5
    sub.16 .AX, #1
    stax.local __vr11
    ldax.local __vr11
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    jsr _factorial
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    sta $20
    stx $21
    ldax.local __vr5
    mul.16 .AX, $20
    sta $22
    stx $23
    lda $22
    ldx $23
    stax.local __vr6
@inline_end14:
    ldax.local __vr6
    sta __zp_scratch2
    stx __zp_scratch2+1
    ldax.local __vr0
    mul.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    plz
    plz
    rts
    .func_flags stack_call
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _is_even
; SAC inline storage: 2 bytes
    .global _is_even__param_n
    _is_even__param_n: .word 0
    _is_even__local_0: .word 0
    _is_even__local_4: .word 0
    proc _is_even, W#@_p_n
    .sac
    .var _fp = 0
    .loc "test_zpcall_regression.c", 66
    .var @_p_n = 2
; .debug_var: __is_even @_p_n offset=2 size=2 type=int16 scope=parameter

@entry:
    .loc "test_zpcall_regression.c", 67
    lda _is_even__param_n
    ldx _is_even__param_n+1
    stx __zp_scratch
    ora __zp_scratch
    beq @if_then20
    bra @if_end22
@if_then20:
    lda #1
    ldx #0
    bra @__return
@if_end22:
    .loc "test_zpcall_regression.c", 68
    lda _is_even__param_n
    ldx _is_even__param_n+1
    sub.16 .AX, #1
    sta _is_even__local_4
    stx _is_even__local_4+1
    lda _is_even__local_4
    ldx _is_even__local_4+1
    sta _is_odd__param_0
    stx _is_odd__param_0+1
    jsr _is_odd
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _is_odd
; SAC inline storage: 2 bytes
    .global _is_odd__param_n
    _is_odd__param_n: .word 0
    _is_odd__local_0: .word 0
    _is_odd__local_6: .word 0
    _is_odd__local_13: .word 0
    _is_odd__local_18: .word 0
    proc _is_odd, W#@_p_n
    .sac
    .var _fp = 0
    .loc "test_zpcall_regression.c", 71
    .var @_p_n = 2
; .debug_var: __is_odd @_p_n offset=2 size=2 type=int16 scope=parameter

@entry:
    .loc "test_zpcall_regression.c", 72
    lda _is_odd__param_n
    ldx _is_odd__param_n+1
    stx __zp_scratch
    ora __zp_scratch
    beq @if_then23
    bra @if_end25
@if_then23:
    lda #0
    ldx #0
    bra @__return
@if_end25:
    .loc "test_zpcall_regression.c", 73
    lda _is_odd__param_n
    ldx _is_odd__param_n+1
    sub.16 .AX, #1
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
    .loc "test_zpcall_regression.c", 67
    lda $22
    ora $23
    beq @if_then27
    bra @if_end29
@if_then27:
    lda #1
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    sta _is_odd__local_6
    stx _is_odd__local_6+1
    bra @inline_end26
@if_end29:
    .loc "test_zpcall_regression.c", 68
    lda $20
    sec
    sbc #1
    sta $24
    lda $21
    sbc #0
    sta $25
    lda $24
    ldx $25
    sta $20
    stx $21
    .loc "test_zpcall_regression.c", 72
    lda $24
    ora $25
    beq @if_then32
    bra @if_end34
@if_then32:
    lda #0
    sta $22
    sta $23
    lda $22
    ldx $23
    sta _is_odd__local_13
    stx _is_odd__local_13+1
    bra @inline_end31
@if_end34:
    .loc "test_zpcall_regression.c", 73
    lda $20
    ldx $21
    sub.16 .AX, #1
    sta _is_odd__local_18
    stx _is_odd__local_18+1
    lda _is_odd__local_18
    ldx _is_odd__local_18+1
    sta _is_even__param_n
    stx _is_even__param_n+1
    jsr _is_even
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _is_odd__local_13
    stx _is_odd__local_13+1
@inline_end31:
    .loc "test_zpcall_regression.c", 68
    lda _is_odd__local_13
    ldx _is_odd__local_13+1
    sta _is_odd__local_6
    stx _is_odd__local_6+1
@inline_end26:
    .loc "test_zpcall_regression.c", 73
    lda _is_odd__local_6
    ldx _is_odd__local_6+1
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _sum4
; SAC inline storage: 8 bytes
    .global _sum4__param_a
    _sum4__param_a: .word 0
    .global _sum4__param_b
    _sum4__param_b: .word 0
    .global _sum4__param_c
    _sum4__param_c: .word 0
    .global _sum4__param_d
    _sum4__param_d: .word 0
    _sum4__local_0: .word 0
    _sum4__local_1: .word 0
    _sum4__local_2: .word 0
    _sum4__local_3: .word 0
    proc _sum4, W#@_p_a, W#@_p_b, W#@_p_c, W#@_p_d
    .sac
    .var _fp = 0
    .loc "test_zpcall_regression.c", 77
    .var @_p_a = 2
    .var @_p_b = 4
    .var @_p_c = 6
    .var @_p_d = 8
; .debug_var: __sum4 @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __sum4 @_p_b offset=4 size=2 type=int16 scope=parameter
; .debug_var: __sum4 @_p_c offset=6 size=2 type=int16 scope=parameter
; .debug_var: __sum4 @_p_d offset=8 size=2 type=int16 scope=parameter

@entry:
    .loc "test_zpcall_regression.c", 78
    lda _sum4__param_b
    ldx _sum4__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _sum4__param_a
    ldx _sum4__param_a+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda _sum4__param_c
    ldx _sum4__param_c+1
    add.16 .AX, $20
    sta $22
    stx $23
    lda _sum4__param_d
    ldx _sum4__param_d+1
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

; function _char_add
; SAC inline storage: 6 bytes
    .global _char_add__param_a
    _char_add__param_a: .word 0
    .global _char_add__param_b
    _char_add__param_b: .word 0
    .global _char_add__param_c
    _char_add__param_c: .word 0
    _char_add__local_0: .word 0
    _char_add__local_1: .word 0
    _char_add__local_2: .word 0
    proc _char_add, B#@_p_a, B#@_p_b, B#@_p_c
    .sac
    .var _fp = 0
    .loc "test_zpcall_regression.c", 82
    .var @_p_a = 2
    .var @_p_b = 4
    .var @_p_c = 6
; .debug_var: __char_add @_p_a offset=2 size=2 type=int8 scope=parameter
; .debug_var: __char_add @_p_b offset=4 size=2 type=int8 scope=parameter
; .debug_var: __char_add @_p_c offset=6 size=2 type=int8 scope=parameter

@entry:
    lda _char_add__param_a
    ldx #0
    sta $20
    stx $21
    lda _char_add__param_b
    ldx #0
    sta $22
    stx $23
    .loc "test_zpcall_regression.c", 83
    lda $20
    ldx $21
    clc
    adc $22
    sta $24
    stx $25
    lda $24
    ldx $25
    sta $20
    lda $24
    ldx $25
    sta $20
    stx $21
    lda _char_add__param_c
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
    sta $20
    lda $24
    ldx $25
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _main
; SAC inline storage: 26 bytes
    _main__local_0: .word 0
    _main__local_8: .word 0
    _main__local_29: .word 0
    _main__local_41: .word 0
    _main__local_55: .word 0
    _main__local_57: .word 0
    _main__local_77: .word 0
    _main__local_79: .word 0
    _main__local_81: .word 0
    _main__local_101: .long 0
    _main__local_128: .word 0
    _main__local_130: .word 0
    _main__local_131: .word 0
    _main__local_137: .word 0
    _main__local_156: .word 0
    _main__local_164: .word 0
    _main__local_170: .word 0
    _main__local_180: .word 0
    _main__local_188: .word 0
    _main__local_194: .word 0
    _main__local_202: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_zpcall_regression.c", 86
    .local @_l_l1 = 20
    .local @_l_product = 2
    .local @_l_r = 8
    .local @_l_r1 = 0
    .local @_l_r2 = 4
    .local @_l_r3 = 10
    .local @_l_r4 = 12
    .local @_l_r6 = 16
    .local @_l_r8 = 18
    .local @_l_val = 14
; .debug_var: __main @_l_l1 offset=20 size=4 type=int32 scope=local
; .debug_var: __main @_l_product offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_r offset=8 size=2 type=int16 scope=local
; .debug_var: __main @_l_r1 offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_r2 offset=4 size=2 type=int16 scope=local
; .debug_var: __main @_l_r3 offset=10 size=2 type=int16 scope=local
; .debug_var: __main @_l_r4 offset=12 size=2 type=int16 scope=local
; .debug_var: __main @_l_r6 offset=16 size=2 type=int16 scope=local
; .debug_var: __main @_l_r8 offset=18 size=2 type=int16 scope=local
; .debug_var: __main @_l_val offset=14 size=2 type=int16 scope=local

@entry:
    .loc "test_zpcall_regression.c", 88
    lda #3
    ldx #0
    sta $20
    stx $21
    lda #4
    ldx #0
    sta $22
    stx $23
    lda #100
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx $21
    sta $26
    stx $27
    lda $22
    ldx $23
    sta $26
    stx $27
    lda $24
    ldx $25
    sta $26
    stx $27
    .loc "test_zpcall_regression.c", 13
    lda $20
    ldx $21
    sta $24
    stx $25
    lda $22
    ldx $23
    sta $24
    stx $25
    .loc "test_zpcall_regression.c", 9
    lda $20
    ldx $21
    mul.16 .AX, $22
    sta $24
    stx $25
    lda $24
    ldx $25
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end39:
    .loc "test_zpcall_regression.c", 13
    lda $22
    ldx $23
    sta _main__local_8
    stx _main__local_8+1
    .loc "test_zpcall_regression.c", 14
    lda _main__local_8
    ldx _main__local_8+1
    add.16 .AX, $26
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
@inline_end38:
    .loc "test_zpcall_regression.c", 88
    lda $20
    ldx $21
    sta _main__local_0
    stx _main__local_0+1
    .loc "test_zpcall_regression.c", 89
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
    .loc "test_zpcall_regression.c", 90
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
    .loc "test_zpcall_regression.c", 93
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
    lda $20
    ldx $21
    sta $26
    stx $27
    lda $22
    ldx $23
    sta $26
    stx $27
    lda $24
    ldx $25
    sta $26
    stx $27
    .loc "test_zpcall_regression.c", 28
    lda $20
    ldx $21
    sta $24
    stx $25
    lda $22
    ldx $23
    sta $24
    stx $25
    .loc "test_zpcall_regression.c", 23
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_zpcall_regression.c", 19
    lda $20
    clc
    adc #1
    sta $28
    lda $21
    adc #0
    sta $29
    lda $28
    ldx $29
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end44:
    .loc "test_zpcall_regression.c", 23
    lda $22
    ldx $23
    sta _main__local_41
    stx _main__local_41+1
    .loc "test_zpcall_regression.c", 24
    lda _main__local_41
    ldx _main__local_41+1
    add.16 .AX, $24
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
@inline_end43:
    .loc "test_zpcall_regression.c", 28
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_zpcall_regression.c", 29
    lda $22
    clc
    adc $26
    sta $20
    lda $23
    adc $26+1
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end42:
    .loc "test_zpcall_regression.c", 93
    lda $20
    ldx $21
    sta _main__local_29
    stx _main__local_29+1
    .loc "test_zpcall_regression.c", 94
    lda _main__local_29
    ldx _main__local_29+1
    sta $20
    lda _results
    ldx _results+1
    sta $22
    stx $23
    lda #2
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
    .loc "test_zpcall_regression.c", 97
    lda #99
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_57
    stx _main__local_57+1
    .loc "test_zpcall_regression.c", 38
    lda #32
    ldx #0
    sta $22
    stx $23
    lda #77
    ldx #0
    sta $20
    stx $21
    lda $22
    ldx $23
    sta $24
    stx $25
    lda $20
    ldx $21
    sta $24
    stx $25
    .loc "test_zpcall_regression.c", 34
    lda $20
    ldx $21
    ldy #0
    sta ($22),y
    txa
    iny
    sta ($22),y
@inline_end49:
    lda _main__local_57
    ldx _main__local_57+1
    sta $20
    stx $21
    .loc "test_zpcall_regression.c", 39
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end48:
    .loc "test_zpcall_regression.c", 97
    lda $22
    ldx $23
    sta _main__local_55
    stx _main__local_55+1
    .loc "test_zpcall_regression.c", 98
    lda #255
    ldx #0
    sta $20
    stx $21
    lda _main__local_55
    ldx _main__local_55+1
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
    .loc "test_zpcall_regression.c", 99
    lda _main__local_55
    ldx _main__local_55+1
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
    lda #4
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
    .loc "test_zpcall_regression.c", 102
    lda #55
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_79
    stx _main__local_79+1
    .loc "test_zpcall_regression.c", 48
    lda #32
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
    .loc "test_zpcall_regression.c", 44
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
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
@inline_end52:
    .loc "test_zpcall_regression.c", 48
    lda $20
    ldx $21
    sta _main__local_81
    stx _main__local_81+1
    lda _main__local_81
    ldx _main__local_81+1
    sta $20
    stx $21
    .loc "test_zpcall_regression.c", 49
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end51:
    .loc "test_zpcall_regression.c", 102
    lda $22
    ldx $23
    sta _main__local_77
    stx _main__local_77+1
    .loc "test_zpcall_regression.c", 103
    lda #255
    ldx #0
    sta $20
    stx $21
    lda _main__local_77
    ldx _main__local_77+1
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
    lda #5
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
    .loc "test_zpcall_regression.c", 104
    lda _main__local_77
    ldx _main__local_77+1
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
    lda #6
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
    .loc "test_zpcall_regression.c", 107
    lda #160
    ldx #134
    ldy #1
    ldz #0
    sta $20
    stx $21
    sty $22
    stz $23
    lda #64
    ldx #13
    ldy #3
    ldz #0
    sta $24
    stx $25
    sty $26
    stz $27
    lda $20
    ldx $21
    ldy $22
    ldz $23
    sta $28
    stx $29
    sty $2A
    stz $2B
    lda $24
    ldx $25
    ldy $26
    ldz $27
    sta $28
    stx $29
    sty $2A
    stz $2B
    .loc "test_zpcall_regression.c", 54
    lda $20
    ldx $21
    ldy $22
    ldz $23
    add.32 .AXYZ, $24
    sta $28
    stx $29
    sty $2A
    stz $2B
    lda $28
    ldx $29
    ldy $2A
    ldz $2B
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    sty $24
    stz $25
@inline_end55:
    .loc "test_zpcall_regression.c", 107
    lda $22
    ldx $23
    ldy $24
    ldz $25
    sta _main__local_101
    stx _main__local_101+1
    sty _main__local_101+2
    stz _main__local_101+3
    .loc "test_zpcall_regression.c", 108
    lda _main__local_101
    ldx _main__local_101+1
    ldy _main__local_101+2
    ldz _main__local_101+3
    sta $20
    lda _results
    ldx _results+1
    sta $22
    stx $23
    lda #7
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
    .loc "test_zpcall_regression.c", 109
    lda _main__local_101
    ldx _main__local_101+1
    ldy _main__local_101+2
    ldz _main__local_101+3
    txa
    ldx #0
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    sta $24
    lda _results
    ldx _results+1
    sta $24
    stx $25
    lda #8
    ldx #0
    sta $26
    stx $27
    lda $20
    ldx $21
    ldy $22
    ldz $23
    pha
    lda $26
    ldx $27
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $24
    ldx $24+1
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
    .loc "test_zpcall_regression.c", 110
    lda _main__local_101
    ldx _main__local_101+1
    ldy _main__local_101+2
    ldz _main__local_101+3
    lda #0
    ldx #0
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    sta $24
    lda _results
    ldx _results+1
    sta $24
    stx $25
    lda #9
    ldx #0
    sta $26
    stx $27
    lda $20
    ldx $21
    ldy $22
    ldz $23
    pha
    lda $26
    ldx $27
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $24
    ldx $24+1
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
    .loc "test_zpcall_regression.c", 111
    lda _main__local_101
    ldx _main__local_101+1
    ldy _main__local_101+2
    ldz _main__local_101+3
    lda #0
    ldx #0
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    sta $24
    lda _results
    ldx _results+1
    sta $24
    stx $25
    lda #10
    ldx #0
    sta $26
    stx $27
    lda $20
    ldx $21
    ldy $22
    ldz $23
    pha
    lda $26
    ldx $27
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $24
    ldx $24+1
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
    .loc "test_zpcall_regression.c", 114
    lda #5
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_130
    stx _main__local_130+1
    .loc "test_zpcall_regression.c", 59
    lda $20
    ldx $21
    cmp.16 .AX, #1
    bcc @if_then58
    beq @if_then58
    bra @if_end60
@if_then58:
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    ldx $23
    sta _main__local_131
    stx _main__local_131+1
    bra @inline_end57
@if_end60:
    .loc "test_zpcall_regression.c", 60
    lda _main__local_130
    ldx _main__local_130+1
    sub.16 .AX, #1
    sta _main__local_137
    stx _main__local_137+1
    lda _main__local_137
    ldx _main__local_137+1
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    jsr _factorial
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    sta $20
    stx $21
    lda _main__local_130
    ldx _main__local_130+1
    mul.16 .AX, $20
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_131
    stx _main__local_131+1
@inline_end57:
    .loc "test_zpcall_regression.c", 114
    lda _main__local_131
    ldx _main__local_131+1
    sta _main__local_128
    stx _main__local_128+1
    .loc "test_zpcall_regression.c", 115
    lda #255
    ldx #0
    sta $20
    stx $21
    lda _main__local_128
    ldx _main__local_128+1
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
    lda #11
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
    .loc "test_zpcall_regression.c", 116
    lda _main__local_128
    ldx _main__local_128+1
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
    lda #12
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
    .loc "test_zpcall_regression.c", 119
    lda #4
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_zpcall_regression.c", 67
    lda $20
    ora $21
    beq @if_then64
    bra @if_end66
@if_then64:
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $24
    stx $25
    lda $24
    ldx $25
    sta _main__local_156
    stx _main__local_156+1
    bra @inline_end63
@if_end66:
    .loc "test_zpcall_regression.c", 68
    lda $22
    sec
    sbc #1
    sta $24
    lda $23
    sbc #0
    sta $25
    lda $24
    ldx $25
    sta $20
    stx $21
    .loc "test_zpcall_regression.c", 72
    lda $24
    ora $25
    beq @if_then69
    bra @if_end71
@if_then69:
    lda #0
    sta $22
    sta $23
    lda $22
    ldx $23
    sta $24
    stx $25
    lda $24
    ldx $25
    sta _main__local_164
    stx _main__local_164+1
    bra @inline_end68
@if_end71:
    .loc "test_zpcall_regression.c", 73
    lda $20
    ldx $21
    sub.16 .AX, #1
    sta _main__local_170
    stx _main__local_170+1
    lda _main__local_170
    ldx _main__local_170+1
    sta _is_even__param_n
    stx _is_even__param_n+1
    jsr _is_even
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    ldx $23
    sta _main__local_164
    stx _main__local_164+1
@inline_end68:
    lda _main__local_164
    ldx _main__local_164+1
    sta $20
    stx $21
    .loc "test_zpcall_regression.c", 68
    lda $20
    ldx $21
    sta _main__local_156
    stx _main__local_156+1
@inline_end63:
    .loc "test_zpcall_regression.c", 119
    lda _main__local_156
    ldx _main__local_156+1
    sta $20
    lda _results
    ldx _results+1
    sta $22
    stx $23
    lda #13
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
    .loc "test_zpcall_regression.c", 120
    lda #3
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_zpcall_regression.c", 67
    lda $20
    ora $21
    beq @if_then76
    bra @if_end78
@if_then76:
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $24
    stx $25
    lda $24
    ldx $25
    sta _main__local_180
    stx _main__local_180+1
    bra @inline_end75
@if_end78:
    .loc "test_zpcall_regression.c", 68
    lda $22
    sec
    sbc #1
    sta $24
    lda $23
    sbc #0
    sta $25
    lda $24
    ldx $25
    sta $20
    stx $21
    .loc "test_zpcall_regression.c", 72
    lda $24
    ora $25
    beq @if_then81
    bra @if_end83
@if_then81:
    lda #0
    sta $22
    sta $23
    lda $22
    ldx $23
    sta $24
    stx $25
    lda $24
    ldx $25
    sta _main__local_188
    stx _main__local_188+1
    bra @inline_end80
@if_end83:
    .loc "test_zpcall_regression.c", 73
    lda $20
    ldx $21
    sub.16 .AX, #1
    sta _main__local_194
    stx _main__local_194+1
    lda _main__local_194
    ldx _main__local_194+1
    sta _is_even__param_n
    stx _is_even__param_n+1
    jsr _is_even
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    ldx $23
    sta _main__local_188
    stx _main__local_188+1
@inline_end80:
    lda _main__local_188
    ldx _main__local_188+1
    sta $20
    stx $21
    .loc "test_zpcall_regression.c", 68
    lda $20
    ldx $21
    sta _main__local_180
    stx _main__local_180+1
@inline_end75:
    .loc "test_zpcall_regression.c", 120
    lda _main__local_180
    ldx _main__local_180+1
    sta $20
    lda _results
    ldx _results+1
    sta $22
    stx $23
    lda #14
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
    .loc "test_zpcall_regression.c", 123
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
    .loc "test_zpcall_regression.c", 78
    lda $20
    clc
    adc #20
    sta $28
    lda $21
    adc #0
    sta $29
    lda $28
    clc
    adc #30
    sta $20
    lda $29
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
    ldx $23
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end87:
    .loc "test_zpcall_regression.c", 123
    lda $22
    ldx $23
    sta _main__local_202
    stx _main__local_202+1
    .loc "test_zpcall_regression.c", 124
    lda _main__local_202
    ldx _main__local_202+1
    sta $20
    lda _results
    ldx _results+1
    sta $22
    stx $23
    lda #15
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
    .loc "test_zpcall_regression.c", 127
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
    lda $20
    ldx $21
    sta $26
    lda $22
    ldx $23
    sta $20
    lda $24
    ldx $25
    sta $22
    lda $26
    ldx #0
    sta $24
    lda $20
    ldx #0
    sta $24
    lda $22
    ldx #0
    sta $24
    lda $26
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx #0
    ldx #0
    sta $26
    stx $27
    .loc "test_zpcall_regression.c", 83
    lda $24
    ldx $25
    clc
    adc $26
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $24
    lda $20
    ldx $21
    sta $24
    stx $25
    lda $22
    ldx #0
    ldx #0
    sta $20
    stx $21
    lda $24
    clc
    adc $20
    sta $22
    lda $25
    adc $20+1
    sta $23
    lda $22
    ldx $23
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
@inline_end89:
    .loc "test_zpcall_regression.c", 127
    lda _results
    ldx _results+1
    sta $20
    stx $21
    lda #16
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx #0
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
    .loc "test_zpcall_regression.c", 130
    lda #170
    sta $20
    lda _results
    ldx _results+1
    sta $22
    stx $23
    lda #17
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
    .loc "test_zpcall_regression.c", 132
    brk
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 26
    endproc


__zp_save_buf:

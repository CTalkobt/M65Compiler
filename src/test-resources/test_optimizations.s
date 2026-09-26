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

    .global _test_ptr_const_prop
    .global _test_fused_cmp
    .global _test_and_branch
    .global _test_or_branch
    .global _test_dead_local_keeps_used
    .global _test_while1_break
    .global _void_func
    .global _test_void_call
    .global _return_zero
    .global _return_42
    .global _return_char
    .global _test_branch_inversion
    .global _test_compound_assign
    .global _test_char_wrap
    .global _test_i8_cmp
    .global _main

    .segment "code"

; function _test_ptr_const_prop
; SAC inline storage: 4 bytes
    _test_ptr_const_prop__local_0: .word 0
    _test_ptr_const_prop__local_2: .word 0
    proc _test_ptr_const_prop
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_optimizations.c", 5
    .local @_l_p = 0
    .local @_l_val = 2
; .debug_var: __test_ptr_const_prop @_l_p offset=0 size=2 type=ptr scope=local
; .debug_var: __test_ptr_const_prop @_l_val offset=2 size=2 type=int8 scope=local

@entry:
    .loc "test_optimizations.c", 6
    .loc "test_optimizations.c", 7
    lda #42
    sta $4000
    .loc "test_optimizations.c", 8
    lda #99
    sta $4000
    .loc "test_optimizations.c", 10
    lda #0
    ldx #64
    sta $20
    stx $21
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
    sta _test_ptr_const_prop__local_2
    .loc "test_optimizations.c", 11
    lda #99
    sta $20
    lda _test_ptr_const_prop__local_2
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
    cmp.16 .AX, $24
    bne @if_then0
    bra @if_end2
@if_then0:
    lda #1
    ldx #0
    bra @__return
@if_end2:
    .loc "test_optimizations.c", 12
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _test_fused_cmp
; SAC inline storage: 4 bytes
    .global _test_fused_cmp__param_a
    _test_fused_cmp__param_a: .word 0
    .global _test_fused_cmp__param_b
    _test_fused_cmp__param_b: .word 0
    _test_fused_cmp__local_0: .word 0
    _test_fused_cmp__local_1: .word 0
    proc _test_fused_cmp, W#@_p_a, W#@_p_b
    .sac
    .var _fp = 0
    .loc "test_optimizations.c", 16
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __test_fused_cmp @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __test_fused_cmp @_p_b offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_optimizations.c", 17
    lda _test_fused_cmp__param_b
    ldx _test_fused_cmp__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _test_fused_cmp__param_a
    ldx _test_fused_cmp__param_a+1
    cmp.16 .AX, __zp_scratch2
    bcc @if_then3
    bra @if_end5
@if_then3:
    lda #1
    ldx #0
    bra @__return
@if_end5:
    .loc "test_optimizations.c", 18
    lda _test_fused_cmp__param_b
    ldx _test_fused_cmp__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _test_fused_cmp__param_a
    ldx _test_fused_cmp__param_a+1
    cmp.16 .AX, __zp_scratch2
    beq @if_end8
    bcs @if_then6
    bra @if_end8
@if_then6:
    lda #2
    ldx #0
    bra @__return
@if_end8:
    .loc "test_optimizations.c", 19
    lda _test_fused_cmp__param_b
    ldx _test_fused_cmp__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _test_fused_cmp__param_a
    ldx _test_fused_cmp__param_a+1
    cmp.16 .AX, __zp_scratch2
    beq @if_then9
    bra @if_end11
@if_then9:
    lda #3
    ldx #0
    bra @__return
@if_end11:
    .loc "test_optimizations.c", 20
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _test_and_branch
; SAC inline storage: 2 bytes
    .global _test_and_branch__param_flags
    _test_and_branch__param_flags: .word 0
    _test_and_branch__local_0: .word 0
    proc _test_and_branch, B#@_p_flags
    .sac
    .var _fp = 0
    .loc "test_optimizations.c", 24
    .var @_p_flags = 2
; .debug_var: __test_and_branch @_p_flags offset=2 size=2 type=int8 scope=parameter

@entry:
    .loc "test_optimizations.c", 25
    lda #1
    sta $20
    lda _test_and_branch__param_flags
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
    and.16 .AX, $24
    sta $20
    stx $21
    lda $20
    ldx $21
    bne @if_then12
    cmp #$00
    bne @if_then12
    bra @if_end14
@if_then12:
    lda #1
    ldx #0
    bra @__return
@if_end14:
    .loc "test_optimizations.c", 26
    lda #2
    sta $20
    lda _test_and_branch__param_flags
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
    and.16 .AX, $24
    sta $20
    stx $21
    lda $20
    ldx $21
    bne @if_then15
    cmp #$00
    bne @if_then15
    bra @if_end17
@if_then15:
    lda #2
    ldx #0
    bra @__return
@if_end17:
    .loc "test_optimizations.c", 27
    lda #4
    sta $20
    lda _test_and_branch__param_flags
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
    and.16 .AX, $24
    sta $20
    stx $21
    lda $20
    ldx $21
    bne @if_then18
    cmp #$00
    bne @if_then18
    bra @if_end20
@if_then18:
    lda #4
    ldx #0
    bra @__return
@if_end20:
    .loc "test_optimizations.c", 28
    lda #128
    sta $20
    lda _test_and_branch__param_flags
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
    and.16 .AX, $24
    sta $20
    stx $21
    lda $20
    ldx $21
    bne @if_then21
    cmp #$00
    bne @if_then21
    bra @if_end23
@if_then21:
    lda #128
    ldx #0
    bra @__return
@if_end23:
    .loc "test_optimizations.c", 29
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers N, Z
    .frame_size 2
    endproc

; function _test_or_branch
; SAC inline storage: 4 bytes
    .global _test_or_branch__param_a
    _test_or_branch__param_a: .word 0
    .global _test_or_branch__param_b
    _test_or_branch__param_b: .word 0
    _test_or_branch__local_0: .word 0
    _test_or_branch__local_1: .word 0
    proc _test_or_branch, B#@_p_a, B#@_p_b
    .sac
    .var _fp = 0
    .loc "test_optimizations.c", 32
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __test_or_branch @_p_a offset=2 size=2 type=int8 scope=parameter
; .debug_var: __test_or_branch @_p_b offset=4 size=2 type=int8 scope=parameter

@entry:
    lda _test_or_branch__param_a
    ldx #0
    sta $20
    stx $21
    lda _test_or_branch__param_b
    ldx #0
    sta $22
    stx $23
    .loc "test_optimizations.c", 33
    lda $20
    ldx $21
    ora.16 .AX, $22
    sta $24
    stx $25
    lda $24
    ldx $25
    bne @if_then24
    cmp #$00
    bne @if_then24
    bra @if_end26
@if_then24:
    lda #1
    ldx #0
    bra @__return
@if_end26:
    .loc "test_optimizations.c", 34
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers N, Z
    .frame_size 4
    endproc

; function _test_dead_local_keeps_used
; SAC inline storage: 4 bytes
    _test_dead_local_keeps_used__local_0: .word 0
    _test_dead_local_keeps_used__local_2: .word 0
    proc _test_dead_local_keeps_used
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_optimizations.c", 38
    .local @_l_unused = 2
    .local @_l_used = 0
; .debug_var: __test_dead_local_keeps_used @_l_unused offset=2 size=2 type=int16 scope=local
; .debug_var: __test_dead_local_keeps_used @_l_used offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_optimizations.c", 39
    lda #42
    sta _test_dead_local_keeps_used__local_0
    lda #0
    sta _test_dead_local_keeps_used__local_0+1
    .loc "test_optimizations.c", 40
    .loc "test_optimizations.c", 41
    lda _test_dead_local_keeps_used__local_0
    ldx _test_dead_local_keeps_used__local_0+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers N, Z
    .frame_size 4
    endproc

; function _test_while1_break
; SAC inline storage: 2 bytes
    _test_while1_break__local_0: .word 0
    proc _test_while1_break
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_optimizations.c", 45
    .local @_l_count = 0
; .debug_var: __test_while1_break @_l_count offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_optimizations.c", 46
    lda #0
    sta _test_while1_break__local_0
    sta _test_while1_break__local_0+1
@while_body27:
    .loc "test_optimizations.c", 48
    lda _test_while1_break__local_0
    ldx _test_while1_break__local_0+1
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
    sta _test_while1_break__local_0
    stx _test_while1_break__local_0+1
    .loc "test_optimizations.c", 49
    lda _test_while1_break__local_0
    ldx _test_while1_break__local_0+1
    cmp.16 .AX, #5
    beq @if_then29
    bra @while_body27
@if_then29:
    bra @while_body27
@while_end28:
    .loc "test_optimizations.c", 51
    lda _test_while1_break__local_0
    ldx _test_while1_break__local_0+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _void_func
; SAC zero-alloc leaf: no storage overhead
    proc _void_func
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_optimizations.c", 55

@entry:
@__return:
    rts
    .func_flags stack_call, static_alloc, zeroalloc, leaf
    .frame_size 0
    endproc

; function _test_void_call
; SAC zero-alloc leaf: no storage overhead
    proc _test_void_call
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_optimizations.c", 57

@entry:
@__return:
    rts
    .func_flags stack_call, static_alloc, zeroalloc, leaf
    .frame_size 0
    endproc

; function _return_zero
; SAC zero-alloc leaf: no storage overhead
    proc _return_zero
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_optimizations.c", 63

@entry:
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, zeroalloc, leaf
    .reg_clobbers A, X
    .flag_clobbers N, Z
    .frame_size 0
    endproc

; function _return_42
; SAC zero-alloc leaf: no storage overhead
    proc _return_42
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_optimizations.c", 64

@entry:
    lda #42
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, zeroalloc, leaf
    .reg_clobbers A, X
    .flag_clobbers N, Z
    .frame_size 0
    endproc

; function _return_char
; SAC zero-alloc leaf: no storage overhead
    proc _return_char
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_optimizations.c", 65

@entry:
    lda #255
@__return:
    rts
    .func_flags stack_call, static_alloc, zeroalloc, leaf
    .reg_clobbers A, X
    .flag_clobbers N, Z
    .frame_size 0
    endproc

; function _test_branch_inversion
; SAC inline storage: 4 bytes
    .global _test_branch_inversion__param_x
    _test_branch_inversion__param_x: .word 0
    _test_branch_inversion__local_0: .word 0
    _test_branch_inversion__local_1: .word 0
    proc _test_branch_inversion, W#@_p_x
    .sac
    .var _fp = 0
    .loc "test_optimizations.c", 68
    .local @_l_result = 2
; .debug_var: __test_branch_inversion @_l_result offset=2 size=2 type=int16 scope=local
    .var @_p_x = 2
; .debug_var: __test_branch_inversion @_p_x offset=2 size=2 type=int16 scope=parameter

@entry:
    .loc "test_optimizations.c", 69
    lda #0
    sta _test_branch_inversion__local_1
    sta _test_branch_inversion__local_1+1
    .loc "test_optimizations.c", 70
    lda _test_branch_inversion__param_x
    ldx _test_branch_inversion__param_x+1
    cmp.16 .AX, #10
    bcc @if_then33
    bra @if_else34
@if_then33:
    .loc "test_optimizations.c", 71
    lda #1
    sta _test_branch_inversion__local_1
    lda #0
    sta _test_branch_inversion__local_1+1
    bra @if_end35
@if_else34:
    .loc "test_optimizations.c", 73
    lda #2
    sta _test_branch_inversion__local_1
    lda #0
    sta _test_branch_inversion__local_1+1
@if_end35:
    .loc "test_optimizations.c", 75
    lda _test_branch_inversion__local_1
    ldx _test_branch_inversion__local_1+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _test_compound_assign
; SAC inline storage: 2 bytes
    _test_compound_assign__local_0: .word 0
    proc _test_compound_assign
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_optimizations.c", 79
    .local @_l_x = 0
; .debug_var: __test_compound_assign @_l_x offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_optimizations.c", 80
    lda #100
    sta _test_compound_assign__local_0
    lda #0
    sta _test_compound_assign__local_0+1
    .loc "test_optimizations.c", 81
    lda _test_compound_assign__local_0
    ldx _test_compound_assign__local_0+1
    add.16 .AX, #50
    sta $22
    stx $23
    sta _test_compound_assign__local_0
    stx _test_compound_assign__local_0+1
    .loc "test_optimizations.c", 82
    lda _test_compound_assign__local_0
    ldx _test_compound_assign__local_0+1
    sub.16 .AX, #25
    sta $22
    stx $23
    sta _test_compound_assign__local_0
    stx _test_compound_assign__local_0+1
    .loc "test_optimizations.c", 83
    lda #2
    ldx #0
    sta $20
    stx $21
    lda _test_compound_assign__local_0
    ldx _test_compound_assign__local_0+1
    lsl.16 .AX
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _test_compound_assign__local_0
    stx _test_compound_assign__local_0+1
    .loc "test_optimizations.c", 84
    lda _test_compound_assign__local_0
    ldx _test_compound_assign__local_0+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _test_char_wrap
; SAC inline storage: 2 bytes
    _test_char_wrap__local_0: .word 0
    proc _test_char_wrap
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_optimizations.c", 88
    .local @_l_c = 0
; .debug_var: __test_char_wrap @_l_c offset=0 size=2 type=int8 scope=local

@entry:
    .loc "test_optimizations.c", 89
    lda #255
    sta _test_char_wrap__local_0
    lda #0
    sta _test_char_wrap__local_0+1
    .loc "test_optimizations.c", 90
    lda _test_char_wrap__local_0
    sta $20
    lda $20
    inc a
    sta $22
    sta _test_char_wrap__local_0
    .loc "test_optimizations.c", 91
    lda #0
    sta $20
    lda _test_char_wrap__local_0
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
    cmp.16 .AX, $24
    bne @if_then36
    bra @if_end38
@if_then36:
    lda #1
    ldx #0
    bra @__return
@if_end38:
    .loc "test_optimizations.c", 92
    lda _test_char_wrap__local_0
    sta $20
    lda $20
    dec a
    sta $22
    sta _test_char_wrap__local_0
    .loc "test_optimizations.c", 93
    lda #255
    sta $20
    lda _test_char_wrap__local_0
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
    cmp.16 .AX, $24
    bne @if_then39
    bra @if_end41
@if_then39:
    lda #2
    ldx #0
    bra @__return
@if_end41:
    .loc "test_optimizations.c", 94
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _test_i8_cmp
; SAC inline storage: 2 bytes
    .global _test_i8_cmp__param_val
    _test_i8_cmp__param_val: .word 0
    _test_i8_cmp__local_0: .word 0
    proc _test_i8_cmp, B#@_p_val
    .sac
    .var _fp = 0
    .loc "test_optimizations.c", 98
    .var @_p_val = 2
; .debug_var: __test_i8_cmp @_p_val offset=2 size=2 type=int8 scope=parameter

@entry:
    .loc "test_optimizations.c", 99
    lda #10
    sta $20
    lda _test_i8_cmp__param_val
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
    cmp.16 .AX, $24
    bcc @if_then42
    bra @if_end44
@if_then42:
    lda #1
    ldx #0
    bra @__return
@if_end44:
    .loc "test_optimizations.c", 100
    lda #200
    sta $20
    lda _test_i8_cmp__param_val
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
    cmp.16 .AX, $24
    beq @if_end47
    bcs @if_then45
    bra @if_end47
@if_then45:
    lda #2
    ldx #0
    bra @__return
@if_end47:
    .loc "test_optimizations.c", 101
    lda #100
    sta $20
    lda _test_i8_cmp__param_val
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
    cmp.16 .AX, $24
    beq @if_then48
    bra @if_end50
@if_then48:
    lda #3
    ldx #0
    bra @__return
@if_end50:
    .loc "test_optimizations.c", 102
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _main
; SAC inline storage: 18 bytes
    _main__local_1: .word 0
    _main__local_3: .word 0
    _main__local_57: .word 0
    _main__local_62: .word 0
    _main__local_67: .word 0
    _main__local_72: .word 0
    _main__local_77: .word 0
    _main__local_124: .word 0
    _main__local_126: .word 0
    _main__local_131: .word 0
    _main__local_171: .word 0
    _main__local_180: .word 0
    _main__local_191: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_optimizations.c", 105
    .local @_l_c = 16
    .local @_l_count = 8
    .local @_l_p = 0
    .local @_l_result = 12
    .local @_l_unused = 6
    .local @_l_used = 4
    .local @_l_val = 2
    .local @_l_x = 14
; .debug_var: __main @_l_c offset=16 size=2 type=int8 scope=local
; .debug_var: __main @_l_count offset=8 size=2 type=int16 scope=local
; .debug_var: __main @_l_p offset=0 size=2 type=ptr scope=local
; .debug_var: __main @_l_result offset=12 size=2 type=int16 scope=local
; .debug_var: __main @_l_unused offset=6 size=2 type=int16 scope=local
; .debug_var: __main @_l_used offset=4 size=2 type=int16 scope=local
; .debug_var: __main @_l_val offset=2 size=2 type=int8 scope=local
; .debug_var: __main @_l_x offset=14 size=2 type=int16 scope=local

@entry:
    .loc "test_optimizations.c", 6
    .loc "test_optimizations.c", 7
    lda #42
    sta $4000
    .loc "test_optimizations.c", 8
    lda #99
    sta $4000
    .loc "test_optimizations.c", 10
    lda #0
    ldx #64
    sta $20
    stx $21
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
    sta _main__local_3
    .loc "test_optimizations.c", 11
    lda #99
    sta $20
    lda _main__local_3
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
    cmp.16 .AX, $24
    bne @if_then55
    bra @if_end57
@if_then55:
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end54
@if_end57:
    .loc "test_optimizations.c", 12
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end54:
    .loc "test_optimizations.c", 107
    lda $22
    ora $23
    bne @if_then51
    bra @if_end53
@if_then51:
    lda #1
    ldx #0
    bra @__return
@if_end53:
    .loc "test_optimizations.c", 110
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
    stx $25
    lda $22
    ldx $23
    sta $26
    stx $27
    .loc "test_optimizations.c", 17
    lda $20
    ldx $21
    cmp.16 .AX, #10
    bcc @if_then64
    bra @if_end66
@if_then64:
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end63
@if_end66:
    .loc "test_optimizations.c", 18
    lda $24
    ldx $25
    cmp.16 .AX, $26
    beq @if_end70
    bcs @if_then68
    bra @if_end70
@if_then68:
    lda #2
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end63
@if_end70:
    .loc "test_optimizations.c", 19
    lda $24
    ldx $25
    cmp.16 .AX, $26
    beq @if_then72
    bra @if_end74
@if_then72:
    lda #3
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end63
@if_end74:
    .loc "test_optimizations.c", 20
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end63:
    .loc "test_optimizations.c", 110
    lda $22
    ldx $23
    cmp.16 .AX, #1
    bne @if_then60
    bra @if_end62
@if_then60:
    lda #2
    ldx #0
    bra @__return
@if_end62:
    .loc "test_optimizations.c", 111
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
    stx $25
    lda $22
    ldx $23
    sta $26
    stx $27
    .loc "test_optimizations.c", 17
    lda $20
    ldx $21
    cmp.16 .AX, #5
    bcc @if_then81
    bra @if_end83
@if_then81:
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end80
@if_end83:
    .loc "test_optimizations.c", 18
    lda $24
    ldx $25
    cmp.16 .AX, $26
    beq @if_end87
    bcs @if_then85
    bra @if_end87
@if_then85:
    lda #2
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end80
@if_end87:
    .loc "test_optimizations.c", 19
    lda $24
    ldx $25
    cmp.16 .AX, $26
    beq @if_then89
    bra @if_end91
@if_then89:
    lda #3
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end80
@if_end91:
    .loc "test_optimizations.c", 20
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end80:
    .loc "test_optimizations.c", 111
    lda $22
    ldx $23
    cmp.16 .AX, #2
    bne @if_then77
    bra @if_end79
@if_then77:
    lda #3
    ldx #0
    bra @__return
@if_end79:
    .loc "test_optimizations.c", 112
    lda #7
    ldx #0
    sta $20
    stx $21
    lda #7
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    sta $24
    stx $25
    lda $22
    ldx $23
    sta $26
    stx $27
    .loc "test_optimizations.c", 17
    lda $20
    ldx $21
    cmp.16 .AX, #7
    bcc @if_then98
    bra @if_end100
@if_then98:
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end97
@if_end100:
    .loc "test_optimizations.c", 18
    lda $24
    ldx $25
    cmp.16 .AX, $26
    beq @if_end104
    bcs @if_then102
    bra @if_end104
@if_then102:
    lda #2
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end97
@if_end104:
    .loc "test_optimizations.c", 19
    lda $24
    ldx $25
    cmp.16 .AX, $26
    beq @if_then106
    bra @if_end108
@if_then106:
    lda #3
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end97
@if_end108:
    .loc "test_optimizations.c", 20
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end97:
    .loc "test_optimizations.c", 112
    lda $22
    ldx $23
    cmp.16 .AX, #3
    bne @if_then94
    bra @if_end96
@if_then94:
    lda #4
    ldx #0
    bra @__return
@if_end96:
    .loc "test_optimizations.c", 115
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_57
    lda _main__local_57
    sta _test_and_branch__param_flags
    stx _test_and_branch__param_flags+1
    jsr _test_and_branch
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #1
    bne @if_then111
    bra @if_end113
@if_then111:
    lda #5
    ldx #0
    bra @__return
@if_end113:
    .loc "test_optimizations.c", 116
    lda #2
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_62
    lda _main__local_62
    sta _test_and_branch__param_flags
    stx _test_and_branch__param_flags+1
    jsr _test_and_branch
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #2
    bne @if_then114
    bra @if_end116
@if_then114:
    lda #6
    ldx #0
    bra @__return
@if_end116:
    .loc "test_optimizations.c", 117
    lda #4
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_67
    lda _main__local_67
    sta _test_and_branch__param_flags
    stx _test_and_branch__param_flags+1
    jsr _test_and_branch
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #4
    bne @if_then117
    bra @if_end119
@if_then117:
    lda #7
    ldx #0
    bra @__return
@if_end119:
    .loc "test_optimizations.c", 118
    lda #128
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_72
    lda _main__local_72
    sta _test_and_branch__param_flags
    stx _test_and_branch__param_flags+1
    jsr _test_and_branch
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #128
    bne @if_then120
    bra @if_end122
@if_then120:
    lda #8
    ldx #0
    bra @__return
@if_end122:
    .loc "test_optimizations.c", 119
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta _main__local_77
    lda _main__local_77
    sta _test_and_branch__param_flags
    stx _test_and_branch__param_flags+1
    jsr _test_and_branch
    sta $20
    stx $21
    lda $20
    ora $21
    bne @if_then123
    bra @if_end125
@if_then123:
    lda #9
    ldx #0
    bra @__return
@if_end125:
    .loc "test_optimizations.c", 120
    lda #0
    sta $20
    sta $21
    lda #0
    sta $22
    sta $23
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
    .loc "test_optimizations.c", 33
    lda $22
    ldx $23
    ora.16 .AX, $24
    sta $20
    stx $21
    lda $20
    ldx $21
    bne @if_then130
    cmp #$00
    bne @if_then130
    bra @if_end132
@if_then130:
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end129
@if_end132:
    .loc "test_optimizations.c", 34
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end129:
    .loc "test_optimizations.c", 120
    lda $22
    ora $23
    bne @if_then126
    bra @if_end128
@if_then126:
    lda #10
    ldx #0
    bra @__return
@if_end128:
    .loc "test_optimizations.c", 121
    lda #1
    ldx #0
    sta $20
    stx $21
    lda #0
    sta $22
    sta $23
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
    .loc "test_optimizations.c", 33
    lda $22
    ldx $23
    ora.16 .AX, $24
    sta $20
    stx $21
    lda $20
    ldx $21
    bne @if_then139
    cmp #$00
    bne @if_then139
    bra @if_end141
@if_then139:
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end138
@if_end141:
    .loc "test_optimizations.c", 34
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end138:
    .loc "test_optimizations.c", 121
    lda $22
    ldx $23
    cmp.16 .AX, #1
    bne @if_then135
    bra @if_end137
@if_then135:
    lda #11
    ldx #0
    bra @__return
@if_end137:
    .loc "test_optimizations.c", 122
    lda #0
    sta $20
    sta $21
    lda #1
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
    .loc "test_optimizations.c", 33
    lda $22
    ldx $23
    ora.16 .AX, $24
    sta $20
    stx $21
    lda $20
    ldx $21
    bne @if_then148
    cmp #$00
    bne @if_then148
    bra @if_end150
@if_then148:
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end147
@if_end150:
    .loc "test_optimizations.c", 34
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end147:
    .loc "test_optimizations.c", 122
    lda $22
    ldx $23
    cmp.16 .AX, #1
    bne @if_then144
    bra @if_end146
@if_then144:
    lda #12
    ldx #0
    bra @__return
@if_end146:
    .loc "test_optimizations.c", 39
    lda #42
    sta _main__local_124
    lda #0
    sta _main__local_124+1
    .loc "test_optimizations.c", 40
    .loc "test_optimizations.c", 41
    lda _main__local_124
    ldx _main__local_124+1
    sta $20
    stx $21
@inline_end156:
    .loc "test_optimizations.c", 125
    lda $20
    ldx $21
    cmp.16 .AX, #42
    bne @if_then153
    bra @if_end155
@if_then153:
    lda #13
    ldx #0
    bra @__return
@if_end155:
    .loc "test_optimizations.c", 46
    lda #0
    sta _main__local_131
    sta _main__local_131+1
@while_body162:
    .loc "test_optimizations.c", 48
    lda _main__local_131
    ldx _main__local_131+1
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
    sta _main__local_131
    stx _main__local_131+1
    .loc "test_optimizations.c", 49
    lda _main__local_131
    ldx _main__local_131+1
    cmp.16 .AX, #5
    beq @if_then164
    bra @while_body162
@if_then164:
    bra @while_body162
@while_end163:
    .loc "test_optimizations.c", 51
    lda _main__local_131
    ldx _main__local_131+1
    sta $20
    stx $21
@inline_end161:
    .loc "test_optimizations.c", 128
    lda $20
    ldx $21
    cmp.16 .AX, #5
    bne @if_then158
    bra @if_end160
@if_then158:
    lda #14
    ldx #0
    bra @__return
@if_end160:
@inline_end172:
    .loc "test_optimizations.c", 59
    lda #42
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end171:
    .loc "test_optimizations.c", 131
    lda $22
    ldx $23
    cmp.16 .AX, #42
    bne @if_then168
    bra @if_end170
@if_then168:
    lda #15
    ldx #0
    bra @__return
@if_end170:
    .loc "test_optimizations.c", 63
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end177:
    .loc "test_optimizations.c", 134
    lda $22
    ora $23
    bne @if_then174
    bra @if_end176
@if_then174:
    lda #16
    ldx #0
    bra @__return
@if_end176:
    .loc "test_optimizations.c", 64
    lda #42
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end182:
    .loc "test_optimizations.c", 135
    lda $22
    ldx $23
    cmp.16 .AX, #42
    bne @if_then179
    bra @if_end181
@if_then179:
    lda #17
    ldx #0
    bra @__return
@if_end181:
    .loc "test_optimizations.c", 65
    lda #255
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
@inline_end187:
    .loc "test_optimizations.c", 136
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
    bne @if_then184
    bra @if_end186
@if_then184:
    lda #18
    ldx #0
    bra @__return
@if_end186:
    .loc "test_optimizations.c", 139
    lda #5
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_optimizations.c", 69
    lda #0
    sta $24
    sta $25
    .loc "test_optimizations.c", 70
    lda $20
    ldx $21
    cmp.16 .AX, #10
    bcc @if_then193
    bra @if_else194
@if_then193:
    .loc "test_optimizations.c", 71
    lda #1
    sta $24
    ldx #0
    stx $25
    bra @if_end195
@if_else194:
    .loc "test_optimizations.c", 73
    lda #2
    sta $24
    ldx #0
    stx $25
@if_end195:
    .loc "test_optimizations.c", 75
    lda $24
    ldx $25
    sta $20
    stx $21
@inline_end192:
    .loc "test_optimizations.c", 139
    lda $20
    ldx $21
    cmp.16 .AX, #1
    bne @if_then189
    bra @if_end191
@if_then189:
    lda #19
    ldx #0
    bra @__return
@if_end191:
    .loc "test_optimizations.c", 140
    lda #15
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_optimizations.c", 69
    lda #0
    sta _main__local_171
    sta _main__local_171+1
    .loc "test_optimizations.c", 70
    lda $20
    ldx $21
    cmp.16 .AX, #10
    bcc @if_then201
    bra @if_else202
@if_then201:
    .loc "test_optimizations.c", 71
    lda #1
    sta _main__local_171
    lda #0
    sta _main__local_171+1
    bra @if_end203
@if_else202:
    .loc "test_optimizations.c", 73
    lda #2
    sta _main__local_171
    lda #0
    sta _main__local_171+1
@if_end203:
    .loc "test_optimizations.c", 75
    lda _main__local_171
    ldx _main__local_171+1
    sta $20
    stx $21
@inline_end200:
    .loc "test_optimizations.c", 140
    lda $20
    ldx $21
    cmp.16 .AX, #2
    bne @if_then197
    bra @if_end199
@if_then197:
    lda #20
    ldx #0
    bra @__return
@if_end199:
    .loc "test_optimizations.c", 80
    lda #100
    sta _main__local_180
    lda #0
    sta _main__local_180+1
    .loc "test_optimizations.c", 81
    lda _main__local_180
    ldx _main__local_180+1
    add.16 .AX, #50
    sta $22
    stx $23
    sta _main__local_180
    stx _main__local_180+1
    .loc "test_optimizations.c", 82
    lda _main__local_180
    ldx _main__local_180+1
    sub.16 .AX, #25
    sta $22
    stx $23
    sta _main__local_180
    stx _main__local_180+1
    .loc "test_optimizations.c", 83
    lda #2
    ldx #0
    sta $20
    stx $21
    lda _main__local_180
    ldx _main__local_180+1
    lsl.16 .AX
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_180
    stx _main__local_180+1
    .loc "test_optimizations.c", 84
    lda _main__local_180
    ldx _main__local_180+1
    sta $20
    stx $21
@inline_end208:
    .loc "test_optimizations.c", 143
    lda $20
    ldx $21
    cmp.16 .AX, #250
    bne @if_then205
    bra @if_end207
@if_then205:
    lda #21
    ldx #0
    bra @__return
@if_end207:
    .loc "test_optimizations.c", 89
    lda #255
    sta _main__local_191
    lda #0
    sta _main__local_191+1
    .loc "test_optimizations.c", 90
    lda _main__local_191
    sta $20
    lda $20
    inc a
    sta $22
    sta _main__local_191
    .loc "test_optimizations.c", 91
    lda #0
    sta $20
    lda _main__local_191
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
    cmp.16 .AX, $24
    bne @if_then214
    bra @if_end216
@if_then214:
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end213
@if_end216:
    .loc "test_optimizations.c", 92
    lda _main__local_191
    sta $20
    lda $20
    dec a
    sta $24
    sta _main__local_191
    .loc "test_optimizations.c", 93
    lda #255
    sta $20
    lda _main__local_191
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $24
    ldx $25
    cmp.16 .AX, $26
    bne @if_then218
    bra @if_end220
@if_then218:
    lda #2
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end213
@if_end220:
    .loc "test_optimizations.c", 94
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end213:
    .loc "test_optimizations.c", 146
    lda $22
    ora $23
    bne @if_then210
    bra @if_end212
@if_then210:
    lda #22
    ldx #0
    bra @__return
@if_end212:
    .loc "test_optimizations.c", 149
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
    .loc "test_optimizations.c", 99
    lda #10
    sta $24
    lda $22
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $26
    ldx $27
    cmp.16 .AX, $22
    bcc @if_then227
    bra @if_end229
@if_then227:
    lda #1
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    bra @inline_end226
@if_end229:
    .loc "test_optimizations.c", 100
    lda #200
    sta $22
    lda $20
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $22
    ldx #0
    ldx #0
    sta $28
    stx $29
    lda $26
    ldx $27
    cmp.16 .AX, $28
    beq @if_end233
    bcs @if_then231
    bra @if_end233
@if_then231:
    lda #2
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    bra @inline_end226
@if_end233:
    .loc "test_optimizations.c", 101
    lda #100
    sta $22
    lda $20
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $22
    ldx #0
    ldx #0
    sta $20
    stx $21
    lda $26
    ldx $27
    cmp.16 .AX, $20
    beq @if_then235
    bra @if_end237
@if_then235:
    lda #3
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $24
    stx $25
    bra @inline_end226
@if_end237:
    .loc "test_optimizations.c", 102
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $24
    stx $25
@inline_end226:
    .loc "test_optimizations.c", 149
    lda $24
    ldx $25
    cmp.16 .AX, #1
    bne @if_then223
    bra @if_end225
@if_then223:
    lda #23
    ldx #0
    bra @__return
@if_end225:
    .loc "test_optimizations.c", 150
    lda #250
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda $22
    ldx #0
    sta $20
    .loc "test_optimizations.c", 99
    lda #10
    sta $24
    lda $22
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $26
    ldx $27
    cmp.16 .AX, $22
    bcc @if_then244
    bra @if_end246
@if_then244:
    lda #1
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    bra @inline_end243
@if_end246:
    .loc "test_optimizations.c", 100
    lda #200
    sta $22
    lda $20
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $22
    ldx #0
    ldx #0
    sta $28
    stx $29
    lda $26
    ldx $27
    cmp.16 .AX, $28
    beq @if_end250
    bcs @if_then248
    bra @if_end250
@if_then248:
    lda #2
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    bra @inline_end243
@if_end250:
    .loc "test_optimizations.c", 101
    lda #100
    sta $22
    lda $20
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $22
    ldx #0
    ldx #0
    sta $20
    stx $21
    lda $26
    ldx $27
    cmp.16 .AX, $20
    beq @if_then252
    bra @if_end254
@if_then252:
    lda #3
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $24
    stx $25
    bra @inline_end243
@if_end254:
    .loc "test_optimizations.c", 102
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $24
    stx $25
@inline_end243:
    .loc "test_optimizations.c", 150
    lda $24
    ldx $25
    cmp.16 .AX, #2
    bne @if_then240
    bra @if_end242
@if_then240:
    lda #24
    ldx #0
    bra @__return
@if_end242:
    .loc "test_optimizations.c", 151
    lda #100
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda $22
    ldx #0
    sta $20
    .loc "test_optimizations.c", 99
    lda #10
    sta $24
    lda $22
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $26
    ldx $27
    cmp.16 .AX, $22
    bcc @if_then261
    bra @if_end263
@if_then261:
    lda #1
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    bra @inline_end260
@if_end263:
    .loc "test_optimizations.c", 100
    lda #200
    sta $22
    lda $20
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $22
    ldx #0
    ldx #0
    sta $28
    stx $29
    lda $26
    ldx $27
    cmp.16 .AX, $28
    beq @if_end267
    bcs @if_then265
    bra @if_end267
@if_then265:
    lda #2
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    bra @inline_end260
@if_end267:
    .loc "test_optimizations.c", 101
    lda #100
    sta $22
    lda $20
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $22
    ldx #0
    ldx #0
    sta $20
    stx $21
    lda $26
    ldx $27
    cmp.16 .AX, $20
    beq @if_then269
    bra @if_end271
@if_then269:
    lda #3
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $24
    stx $25
    bra @inline_end260
@if_end271:
    .loc "test_optimizations.c", 102
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $24
    stx $25
@inline_end260:
    .loc "test_optimizations.c", 151
    lda $24
    ldx $25
    cmp.16 .AX, #3
    bne @if_then257
    bra @if_end259
@if_then257:
    lda #25
    ldx #0
    bra @__return
@if_end259:
    .loc "test_optimizations.c", 152
    lda #50
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda $22
    ldx #0
    sta $20
    .loc "test_optimizations.c", 99
    lda #10
    sta $24
    lda $22
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $26
    ldx $27
    cmp.16 .AX, $22
    bcc @if_then278
    bra @if_end280
@if_then278:
    lda #1
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    bra @inline_end277
@if_end280:
    .loc "test_optimizations.c", 100
    lda #200
    sta $22
    lda $20
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $22
    ldx #0
    ldx #0
    sta $28
    stx $29
    lda $26
    ldx $27
    cmp.16 .AX, $28
    beq @if_end284
    bcs @if_then282
    bra @if_end284
@if_then282:
    lda #2
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    bra @inline_end277
@if_end284:
    .loc "test_optimizations.c", 101
    lda #100
    sta $22
    lda $20
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $22
    ldx #0
    ldx #0
    sta $20
    stx $21
    lda $26
    ldx $27
    cmp.16 .AX, $20
    beq @if_then286
    bra @if_end288
@if_then286:
    lda #3
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $24
    stx $25
    bra @inline_end277
@if_end288:
    .loc "test_optimizations.c", 102
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $24
    stx $25
@inline_end277:
    .loc "test_optimizations.c", 152
    lda $24
    ora $25
    bne @if_then274
    bra @if_end276
@if_then274:
    lda #26
    ldx #0
    bra @__return
@if_end276:
    .loc "test_optimizations.c", 154
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 18
    endproc


__zp_save_buf:

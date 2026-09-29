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

    .global _select_int
    .global _select_char
    .global _select_ptr
    .global _select_default
    .global _main

    .segment "code"

; function _select_int
; SAC zero-alloc leaf: no storage overhead
    proc _select_int
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_generic.c", 2

@entry:
    lda #1
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, zeroalloc, leaf
    .reg_clobbers A, X
    .flag_clobbers N, Z
    .frame_size 0
    endproc

; function _select_char
; SAC zero-alloc leaf: no storage overhead
    proc _select_char
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_generic.c", 3

@entry:
    lda #2
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, zeroalloc, leaf
    .reg_clobbers A, X
    .flag_clobbers N, Z
    .frame_size 0
    endproc

; function _select_ptr
; SAC zero-alloc leaf: no storage overhead
    proc _select_ptr
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_generic.c", 4

@entry:
    lda #3
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, zeroalloc, leaf
    .reg_clobbers A, X
    .flag_clobbers N, Z
    .frame_size 0
    endproc

; function _select_default
; SAC zero-alloc leaf: no storage overhead
    proc _select_default
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_generic.c", 5

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

; function _main
; SAC inline storage: 16 bytes
    _main__local_0: .word 0
    _main__local_2: .word 0
    _main__local_4: .word 0
    _main__local_6: .word 0
    _main__local_8: .word 0
    _main__local_10: .word 0
    _main__local_12: .word 0
    _main__local_14: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_generic.c", 7
    .local @_l_c = 2
    .local @_l_i = 0
    .local @_l_p = 4
    .local @_l_r1 = 10
    .local @_l_r2 = 12
    .local @_l_r3 = 14
    .local @_l_v1 = 6
    .local @_l_v2 = 8
; .debug_var: __main @_l_c offset=2 size=2 type=int8 scope=local
; .debug_var: __main @_l_i offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_p offset=4 size=2 type=ptr scope=local
; .debug_var: __main @_l_r1 offset=10 size=2 type=int16 scope=local
; .debug_var: __main @_l_r2 offset=12 size=2 type=int16 scope=local
; .debug_var: __main @_l_r3 offset=14 size=2 type=int16 scope=local
; .debug_var: __main @_l_v1 offset=6 size=2 type=int16 scope=local
; .debug_var: __main @_l_v2 offset=8 size=2 type=int16 scope=local

@entry:
    .loc "test_generic.c", 8
    lda #10
    sta _main__local_0
    lda #0
    sta _main__local_0+1
    .loc "test_generic.c", 9
    .loc "test_generic.c", 10
    .loc "test_generic.c", 13
    lda #10
    sta _main__local_6
    lda #0
    sta _main__local_6+1
    .loc "test_generic.c", 14
    lda #20
    sta _main__local_8
    lda #0
    sta _main__local_8+1
    .loc "test_generic.c", 17
    lda #1
    sta _main__local_10
    lda #0
    sta _main__local_10+1
    .loc "test_generic.c", 18
    lda #2
    sta _main__local_12
    lda #0
    sta _main__local_12+1
    .loc "test_generic.c", 19
    lda #3
    sta _main__local_14
    lda #0
    sta _main__local_14+1
    .loc "test_generic.c", 21
    lda _main__local_6
    ldx _main__local_6+1
    cmp.16 .AX, #10
    beq @and_rhs6
    bra @if_end2
@and_rhs6:
    lda _main__local_8
    ldx _main__local_8+1
    cmp.16 .AX, #20
    beq @and_rhs5
    bra @if_end2
@and_rhs5:
    lda _main__local_10
    ldx _main__local_10+1
    cmp.16 .AX, #1
    beq @and_rhs4
    bra @if_end2
@and_rhs4:
    lda _main__local_12
    ldx _main__local_12+1
    cmp.16 .AX, #2
    beq @and_rhs3
    bra @if_end2
@and_rhs3:
    lda _main__local_14
    ldx _main__local_14+1
    cmp.16 .AX, #3
    beq @if_then0
    bra @if_end2
@if_then0:
    .loc "test_generic.c", 22
    lda #0
    ldx #0
    bra @__return
@if_end2:
    .loc "test_generic.c", 24
    lda #1
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 16
    endproc


__zp_save_buf:

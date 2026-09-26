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

    .global _v_x
    .global _g_x
    .global _g_y
    .global _get_val
    .global _test_self_assignment
    .global _test_granular_store
    .global _test_granular_load
    .global _main

    .segment "data"
    .byte 0
_v_x:
; .debug_var: @global _v_x offset=0 size=2 type=int16 scope=global
    .word 42
_g_x:
; .debug_var: @global _g_x offset=0 size=2 type=int16 scope=global
    .word 100
_g_y:
; .debug_var: @global _g_y offset=0 size=2 type=int16 scope=global
    .word 200

    .segment "code"

; function _get_val
; SAC zero-alloc leaf: no storage overhead
    proc _get_val
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_opt_load_store.c", 9

@entry:
    lda #52
    ldx #18
@__return:
    rts
    .func_flags stack_call, static_alloc, zeroalloc, leaf
    .reg_clobbers A, X
    .flag_clobbers N, Z
    .frame_size 0
    endproc

; function _test_self_assignment
; SAC inline storage: 2 bytes
    _test_self_assignment__local_0: .word 0
    proc _test_self_assignment
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_opt_load_store.c", 11
    .local @_l_x = 0
; .debug_var: __test_self_assignment @_l_x offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_opt_load_store.c", 12
    lda #42
    sta _test_self_assignment__local_0
    lda #0
    sta _test_self_assignment__local_0+1
    .loc "test_opt_load_store.c", 13
    lda _test_self_assignment__local_0
    ldx _test_self_assignment__local_0+1
    sta _test_self_assignment__local_0
    stx _test_self_assignment__local_0+1
    .loc "test_opt_load_store.c", 14
    lda _test_self_assignment__local_0
    ldx _test_self_assignment__local_0+1
    cmp.16 .AX, #42
    bne @if_then0
    bra @if_end2
@if_then0:
    lda #1
    ldx #0
    bra @__return
@if_end2:
    .loc "test_opt_load_store.c", 16
    lda _g_x
    ldx _g_x+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _g_x
    stx _g_x+1
    .loc "test_opt_load_store.c", 17
    lda _g_x
    ldx _g_x+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #100
    bne @if_then3
    bra @if_end5
@if_then3:
    lda #2
    ldx #0
    bra @__return
@if_end5:
    .loc "test_opt_load_store.c", 19
    lda _v_x
    ldx _v_x+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _v_x
    stx _v_x+1
    .loc "test_opt_load_store.c", 20
    lda _v_x
    ldx _v_x+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #42
    bne @if_then6
    bra @if_end8
@if_then6:
    lda #3
    ldx #0
    bra @__return
@if_end8:
    .loc "test_opt_load_store.c", 22
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _test_granular_store
; SAC zero-alloc leaf: no storage overhead
    proc _test_granular_store
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_opt_load_store.c", 25

@entry:
    .loc "test_opt_load_store.c", 27
    lda #52
    ldx #17
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _g_x
    stx _g_x+1
    .loc "test_opt_load_store.c", 28
    lda #52
    ldx #34
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _g_y
    stx _g_y+1
    .loc "test_opt_load_store.c", 30
    lda _g_x
    ldx _g_x+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #4404
    bne @if_then9
    bra @if_end11
@if_then9:
    lda #4
    ldx #0
    bra @__return
@if_end11:
    .loc "test_opt_load_store.c", 31
    lda _g_y
    ldx _g_y+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #8756
    bne @if_then12
    bra @if_end14
@if_then12:
    lda #5
    ldx #0
    bra @__return
@if_end14:
    .loc "test_opt_load_store.c", 33
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, zeroalloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 0
    endproc

; function _test_granular_load
; SAC inline storage: 4 bytes
    _test_granular_load__local_0: .word 0
    _test_granular_load__local_1: .word 0
    proc _test_granular_load
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_opt_load_store.c", 36
    .local @_l_a = 0
    .local @_l_b = 2
; .debug_var: __test_granular_load @_l_a offset=0 size=2 type=int16 scope=local
; .debug_var: __test_granular_load @_l_b offset=2 size=2 type=int16 scope=local

@entry:
    .loc "test_opt_load_store.c", 39
    lda #85
    ldx #68
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _g_x
    stx _g_x+1
    .loc "test_opt_load_store.c", 40
    lda _g_x
    ldx _g_x+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _test_granular_load__local_0
    stx _test_granular_load__local_0+1
    .loc "test_opt_load_store.c", 41
    lda _g_x
    ldx _g_x+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _test_granular_load__local_1
    stx _test_granular_load__local_1+1
    .loc "test_opt_load_store.c", 43
    lda _test_granular_load__local_0
    ldx _test_granular_load__local_0+1
    cmp.16 .AX, #17493
    bne @if_then15
    bra @if_end17
@if_then15:
    lda #6
    ldx #0
    bra @__return
@if_end17:
    .loc "test_opt_load_store.c", 44
    lda _test_granular_load__local_1
    ldx _test_granular_load__local_1+1
    cmp.16 .AX, #17493
    bne @if_then18
    bra @if_end20
@if_then18:
    lda #7
    ldx #0
    bra @__return
@if_end20:
    .loc "test_opt_load_store.c", 46
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _main
; SAC inline storage: 4 bytes
    _main__local_18: .word 0
    _main__local_19: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_opt_load_store.c", 49
    .local @_l_a = 0
    .local @_l_b = 2
; .debug_var: __main @_l_a offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_b offset=2 size=2 type=int16 scope=local

@entry:
    .loc "test_opt_load_store.c", 50
    jsr _test_self_assignment
    sta $20
    stx $21
    lda $20
    ora $21
    bne @if_then21
    bra @if_end23
@if_then21:
    lda #1
    ldx #0
    bra @__return
@if_end23:
    .loc "test_opt_load_store.c", 27
    lda #52
    ldx #17
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _g_x
    stx _g_x+1
    .loc "test_opt_load_store.c", 28
    lda #52
    ldx #34
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _g_y
    stx _g_y+1
    .loc "test_opt_load_store.c", 30
    lda _g_x
    ldx _g_x+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #4404
    bne @if_then28
    bra @if_end30
@if_then28:
    lda #4
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end27
@if_end30:
    .loc "test_opt_load_store.c", 31
    lda _g_y
    ldx _g_y+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #8756
    bne @if_then32
    bra @if_end34
@if_then32:
    lda #5
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end27
@if_end34:
    .loc "test_opt_load_store.c", 33
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end27:
    .loc "test_opt_load_store.c", 51
    lda $22
    ora $23
    bne @if_then24
    bra @if_end26
@if_then24:
    lda #2
    ldx #0
    bra @__return
@if_end26:
    .loc "test_opt_load_store.c", 39
    lda #85
    ldx #68
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _g_x
    stx _g_x+1
    .loc "test_opt_load_store.c", 40
    lda _g_x
    ldx _g_x+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_18
    stx _main__local_18+1
    .loc "test_opt_load_store.c", 41
    lda _g_x
    ldx _g_x+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_19
    stx _main__local_19+1
    .loc "test_opt_load_store.c", 43
    lda _main__local_18
    ldx _main__local_18+1
    cmp.16 .AX, #17493
    bne @if_then41
    bra @if_end43
@if_then41:
    lda #6
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end40
@if_end43:
    .loc "test_opt_load_store.c", 44
    lda _main__local_19
    ldx _main__local_19+1
    cmp.16 .AX, #17493
    bne @if_then45
    bra @if_end47
@if_then45:
    lda #7
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end40
@if_end47:
    .loc "test_opt_load_store.c", 46
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end40:
    .loc "test_opt_load_store.c", 52
    lda $22
    ora $23
    bne @if_then37
    bra @if_end39
@if_then37:
    lda #3
    ldx #0
    bra @__return
@if_end39:
    .loc "test_opt_load_store.c", 54
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc


__zp_save_buf:

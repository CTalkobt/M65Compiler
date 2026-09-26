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

    .global _flag
    .global _set_flag
    .global _set_flag_zero
    .global _main

    .segment "bss"
_flag:
; .debug_var: @global _flag offset=0 size=2 type=int16 scope=global
    .res 2

    .segment "code"

; function _set_flag
; SAC zero-alloc leaf: no storage overhead
    proc _set_flag
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_short_circuit.c", 7

@entry:
    .loc "test_short_circuit.c", 8
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _flag
    stx _flag+1
    .loc "test_short_circuit.c", 9
    lda #1
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, zeroalloc, leaf
    .reg_clobbers A, X
    .flag_clobbers N, Z
    .frame_size 0
    endproc

; function _set_flag_zero
; SAC zero-alloc leaf: no storage overhead
    proc _set_flag_zero
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_short_circuit.c", 12

@entry:
    .loc "test_short_circuit.c", 13
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _flag
    stx _flag+1
    .loc "test_short_circuit.c", 14
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
; SAC inline storage: 2 bytes
    _main__local_0: .word 0
    _main__local_4: .word 0
    _main__local_5: .word 0
    _main__local_17: .word 0
    _main__local_26: .word 0
    _main__local_27: .word 0
    _main__local_39: .word 0
    _main__local_48: .word 0
    _main__local_49: .word 0
    _main__local_59: .word 0
    _main__local_60: .word 0
    _main__local_72: .word 0
    _main__local_83: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_short_circuit.c", 17
    .local @_l_result = 0
; .debug_var: __main @_l_result offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_short_circuit.c", 23
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta _flag
    stx _flag+1
    .loc "test_short_circuit.c", 24
    lda #0
    sta $20
    sta $21
@sc_short1:
@sc_done4:
    lda $20
    ldx $21
    sta _main__local_0
    stx _main__local_0+1
    .loc "test_short_circuit.c", 25
    lda _main__local_0
    ldx _main__local_0+1
    stx __zp_scratch
    ora __zp_scratch
    bne @if_then5
    bra @if_end7
@if_then5:
    lda #1
    ldx #0
    bra @__return
@if_end7:
    .loc "test_short_circuit.c", 26
    lda _flag
    ldx _flag+1
    sta $20
    stx $21
    lda $20
    ora $21
    bne @if_then8
    bra @if_end10
@if_then8:
    lda #2
    ldx #0
    bra @__return
@if_end10:
    .loc "test_short_circuit.c", 29
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta _flag
    stx _flag+1
    .loc "test_short_circuit.c", 30
    lda #1
    ldx #0
    sta $20
    stx $21
@sc_merge11:
    .loc "test_short_circuit.c", 8
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _flag
    stx _flag+1
    .loc "test_short_circuit.c", 9
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@sc_done15:
    .loc "test_short_circuit.c", 30
    lda $22
    ldx $23
    sta _main__local_0
    stx _main__local_0+1
    .loc "test_short_circuit.c", 31
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, #1
    bne @if_then16
    bra @if_end18
@if_then16:
    lda #3
    ldx #0
    bra @__return
@if_end18:
    .loc "test_short_circuit.c", 32
    lda _flag
    ldx _flag+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #1
    bne @if_then19
    bra @if_end21
@if_then19:
    lda #4
    ldx #0
    bra @__return
@if_end21:
    .loc "test_short_circuit.c", 37
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta _flag
    stx _flag+1
    .loc "test_short_circuit.c", 38
    lda #1
    ldx #0
    sta $20
    stx $21
@sc_short23:
@sc_done26:
    lda $20
    ldx $21
    sta _main__local_0
    stx _main__local_0+1
    .loc "test_short_circuit.c", 39
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, #1
    bne @if_then27
    bra @if_end29
@if_then27:
    lda #5
    ldx #0
    bra @__return
@if_end29:
    .loc "test_short_circuit.c", 40
    lda _flag
    ldx _flag+1
    sta $20
    stx $21
    lda $20
    ora $21
    bne @if_then30
    bra @if_end32
@if_then30:
    lda #6
    ldx #0
    bra @__return
@if_end32:
    .loc "test_short_circuit.c", 43
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta _flag
    stx _flag+1
    .loc "test_short_circuit.c", 44
    lda #0
    sta $20
    sta $21
@sc_merge33:
    .loc "test_short_circuit.c", 8
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _flag
    stx _flag+1
    .loc "test_short_circuit.c", 9
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@sc_done37:
    .loc "test_short_circuit.c", 44
    lda $22
    ldx $23
    sta _main__local_0
    stx _main__local_0+1
    .loc "test_short_circuit.c", 45
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, #1
    bne @if_then38
    bra @if_end40
@if_then38:
    lda #7
    ldx #0
    bra @__return
@if_end40:
    .loc "test_short_circuit.c", 46
    lda _flag
    ldx _flag+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #1
    bne @if_then41
    bra @if_end43
@if_then41:
    lda #8
    ldx #0
    bra @__return
@if_end43:
    .loc "test_short_circuit.c", 50
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta _flag
    stx _flag+1
    .loc "test_short_circuit.c", 51
    lda #0
    sta $20
    sta $21
@sc_short45:
@sc_done48:
    lda $20
    ldx $21
    sta _main__local_0
    stx _main__local_0+1
    .loc "test_short_circuit.c", 52
    lda _main__local_0
    ldx _main__local_0+1
    stx __zp_scratch
    ora __zp_scratch
    bne @if_then49
    bra @if_end51
@if_then49:
    lda #9
    ldx #0
    bra @__return
@if_end51:
    .loc "test_short_circuit.c", 53
    lda _flag
    ldx _flag+1
    sta $20
    stx $21
    lda $20
    ora $21
    bne @if_then52
    bra @if_end54
@if_then52:
    lda #10
    ldx #0
    bra @__return
@if_end54:
    .loc "test_short_circuit.c", 57
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta _flag
    stx _flag+1
    .loc "test_short_circuit.c", 58
    lda #1
    ldx #0
    sta $20
    stx $21
@sc_short56:
@sc_done59:
    lda $20
    ldx $21
    sta _main__local_0
    stx _main__local_0+1
    .loc "test_short_circuit.c", 59
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, #1
    bne @if_then60
    bra @if_end62
@if_then60:
    lda #11
    ldx #0
    bra @__return
@if_end62:
    .loc "test_short_circuit.c", 60
    lda _flag
    ldx _flag+1
    sta $20
    stx $21
    lda $20
    ora $21
    bne @if_then63
    bra @if_end65
@if_then63:
    lda #12
    ldx #0
    bra @__return
@if_end65:
    .loc "test_short_circuit.c", 64
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta _flag
    stx _flag+1
    .loc "test_short_circuit.c", 65
    lda #0
    sta $20
    sta $21
@sc_merge66:
    .loc "test_short_circuit.c", 8
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _flag
    stx _flag+1
    .loc "test_short_circuit.c", 9
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@sc_done70:
    .loc "test_short_circuit.c", 65
    lda $22
    ldx $23
    sta _main__local_0
    stx _main__local_0+1
    .loc "test_short_circuit.c", 66
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, #1
    bne @if_then71
    bra @if_end73
@if_then71:
    lda #13
    ldx #0
    bra @__return
@if_end73:
    .loc "test_short_circuit.c", 67
    lda _flag
    ldx _flag+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #1
    bne @if_then74
    bra @if_end76
@if_then74:
    lda #14
    ldx #0
    bra @__return
@if_end76:
    .loc "test_short_circuit.c", 70
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta _flag
    stx _flag+1
    .loc "test_short_circuit.c", 71
    lda #1
    ldx #0
    sta $20
    stx $21
@sc_merge77:
    .loc "test_short_circuit.c", 13
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _flag
    stx _flag+1
    .loc "test_short_circuit.c", 14
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
@sc_done81:
    .loc "test_short_circuit.c", 71
    lda $22
    ldx $23
    sta _main__local_0
    stx _main__local_0+1
    .loc "test_short_circuit.c", 72
    lda _main__local_0
    ldx _main__local_0+1
    stx __zp_scratch
    ora __zp_scratch
    bne @if_then82
    bra @if_end84
@if_then82:
    lda #15
    ldx #0
    bra @__return
@if_end84:
    .loc "test_short_circuit.c", 73
    lda _flag
    ldx _flag+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #1
    bne @if_then85
    bra @if_end87
@if_then85:
    lda #16
    ldx #0
    bra @__return
@if_end87:
    .loc "test_short_circuit.c", 75
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc


__zp_save_buf:

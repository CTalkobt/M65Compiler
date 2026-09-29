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

    .global _main

    .segment "code"

; function _main
; SAC inline storage: 18 bytes
    _main__local_0: .word 0
    _main__local_2: .word 0
    _main__local_4: .word 0
    _main__local_8: .word 0
    _main__local_12: .word 0
    _main__local_16: .word 0
    _main__local_20: .word 0
    _main__local_22: .word 0
    _main__local_24: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_modulo.c", 1
    .local @_l_a = 0
    .local @_l_b = 2
    .local @_l_c = 4
    .local @_l_d = 6
    .local @_l_e = 8
    .local @_l_f = 10
    .local @_l_x = 12
    .local @_l_y = 14
    .local @_l_z = 16
; .debug_var: __main @_l_a offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_b offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_c offset=4 size=2 type=int16 scope=local
; .debug_var: __main @_l_d offset=6 size=2 type=int16 scope=local
; .debug_var: __main @_l_e offset=8 size=2 type=int16 scope=local
; .debug_var: __main @_l_f offset=10 size=2 type=int16 scope=local
; .debug_var: __main @_l_x offset=12 size=2 type=int16 scope=local
; .debug_var: __main @_l_y offset=14 size=2 type=int16 scope=local
; .debug_var: __main @_l_z offset=16 size=2 type=int16 scope=local

@entry:
    .loc "test_modulo.c", 2
    lda #10
    sta _main__local_0
    lda #0
    sta _main__local_0+1
    .loc "test_modulo.c", 3
    lda #3
    sta _main__local_2
    lda #0
    sta _main__local_2+1
    .loc "test_modulo.c", 4
    lda _main__local_2
    ldx _main__local_2+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_0
    ldx _main__local_0+1
    mod.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_4
    stx _main__local_4+1
    .loc "test_modulo.c", 6
    lda _main__local_4
    ldx _main__local_4+1
    cmp.16 .AX, #1
    bne @if_then0
    bra @if_end2
@if_then0:
    lda #1
    ldx #0
    bra @__return
@if_end2:
    .loc "test_modulo.c", 8
    lda #3
    sta _main__local_8
    lda #0
    sta _main__local_8+1
    .loc "test_modulo.c", 9
    lda _main__local_8
    ldx _main__local_8+1
    cmp.16 .AX, #3
    bne @if_then3
    bra @if_end5
@if_then3:
    lda #2
    ldx #0
    bra @__return
@if_end5:
    .loc "test_modulo.c", 11
    lda #0
    sta _main__local_12
    sta _main__local_12+1
    .loc "test_modulo.c", 12
    lda _main__local_12
    ldx _main__local_12+1
    stx __zp_scratch
    ora __zp_scratch
    bne @if_then6
    bra @if_end8
@if_then6:
    lda #3
    ldx #0
    bra @__return
@if_end8:
    .loc "test_modulo.c", 15
    lda #1
    sta _main__local_16
    lda #0
    sta _main__local_16+1
    .loc "test_modulo.c", 16
    lda _main__local_16
    ldx _main__local_16+1
    cmp.16 .AX, #1
    bne @if_then9
    bra @if_end11
@if_then9:
    lda #4
    ldx #0
    bra @__return
@if_end11:
    .loc "test_modulo.c", 19
    lda #100
    sta _main__local_20
    lda #0
    sta _main__local_20+1
    .loc "test_modulo.c", 20
    lda #7
    sta _main__local_22
    lda #0
    sta _main__local_22+1
    .loc "test_modulo.c", 21
    lda _main__local_22
    ldx _main__local_22+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_20
    ldx _main__local_20+1
    mod.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_24
    stx _main__local_24+1
    .loc "test_modulo.c", 22
    lda _main__local_24
    ldx _main__local_24+1
    cmp.16 .AX, #2
    bne @if_then12
    bra @if_end14
@if_then12:
    lda #5
    ldx #0
    bra @__return
@if_end14:
    .loc "test_modulo.c", 24
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 18
    endproc


__zp_save_buf:

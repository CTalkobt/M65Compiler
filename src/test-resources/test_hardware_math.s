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
; SAC inline storage: 12 bytes
    _main__local_0: .word 0
    _main__local_2: .word 0
    _main__local_4: .word 0
    _main__local_8: .word 0
    _main__local_10: .word 0
    _main__local_12: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdio.h", 3
    .local @_l_a = 0
    .local @_l_b = 2
    .local @_l_c = 4
    .local @_l_x = 6
    .local @_l_y = 8
    .local @_l_z = 10
; .debug_var: __main @_l_a offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_b offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_c offset=4 size=2 type=int16 scope=local
; .debug_var: __main @_l_x offset=6 size=2 type=int16 scope=local
; .debug_var: __main @_l_y offset=8 size=2 type=int16 scope=local
; .debug_var: __main @_l_z offset=10 size=2 type=int16 scope=local

@entry:
    .loc "test_hardware_math.c", 4
    lda #10
    sta _main__local_0
    lda #0
    sta _main__local_0+1
    .loc "test_hardware_math.c", 5
    lda #3
    sta _main__local_2
    lda #0
    sta _main__local_2+1
    .loc "test_hardware_math.c", 6
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
    .loc "test_hardware_math.c", 8
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
    .loc "test_hardware_math.c", 10
    lda #100
    sta _main__local_8
    lda #0
    sta _main__local_8+1
    .loc "test_hardware_math.c", 11
    lda #7
    sta _main__local_10
    lda #0
    sta _main__local_10+1
    .loc "test_hardware_math.c", 12
    lda _main__local_10
    ldx _main__local_10+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_8
    ldx _main__local_8+1
    mod.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_12
    stx _main__local_12+1
    .loc "test_hardware_math.c", 13
    lda _main__local_12
    ldx _main__local_12+1
    cmp.16 .AX, #2
    bne @if_then3
    bra @if_end5
@if_then3:
    lda #2
    ldx #0
    bra @__return
@if_end5:
    .loc "test_hardware_math.c", 15
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 12
    endproc


__zp_save_buf:

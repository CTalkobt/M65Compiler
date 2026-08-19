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

    .global _hw_reg
    .global _main

    .segment "bss"
_hw_reg:
; .debug_var: @global _hw_reg offset=0 size=2 type=int16 scope=global
    .res 2

    .segment "code"

; function _main
; SAC inline storage: 8 bytes
    _main__local_0: .word 0
    _main__local_5: .word 0
    _main__local_8: .word 0
    _main__local_11: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_volatile_dse.c", 6
    .local @_l_counter = 0
    .local @_l_status = 4
    .local @_l_temp = 2
    .local @_l_val = 6
; .debug_var: __main @_l_counter offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_status offset=4 size=2 type=int16 scope=local
; .debug_var: __main @_l_temp offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_val offset=6 size=2 type=int16 scope=local

@entry:
    .loc "test_volatile_dse.c", 8
    lda #0
    sta _main__local_0
    sta _main__local_0+1
    .loc "test_volatile_dse.c", 9
    lda #1
    sta _main__local_0
    lda #0
    sta _main__local_0+1
    .loc "test_volatile_dse.c", 10
    lda #2
    sta _main__local_0
    lda #0
    sta _main__local_0+1
    .loc "test_volatile_dse.c", 11
    lda #3
    sta _main__local_0
    lda #0
    sta _main__local_0+1
    .loc "test_volatile_dse.c", 15
    .loc "test_volatile_dse.c", 16
    .loc "test_volatile_dse.c", 20
    lda #0
    sta _main__local_8
    sta _main__local_8+1
    .loc "test_volatile_dse.c", 21
    lda #42
    sta _main__local_8
    lda #0
    sta _main__local_8+1
    .loc "test_volatile_dse.c", 22
    lda _main__local_8
    ldx _main__local_8+1
    sta _main__local_11
    stx _main__local_11+1
    .loc "test_volatile_dse.c", 23
    lda _main__local_11
    ldx _main__local_11+1
    cmp.16 .AX, #42
    bne @if_then0
    bra @if_end2
@if_then0:
    lda #1
    ldx #0
    bra @__return
@if_end2:
    .loc "test_volatile_dse.c", 25
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 8
    endproc


__zp_save_buf:

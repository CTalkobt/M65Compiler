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
; SAC inline storage: 2 bytes
    _main__local_0: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_enum.c", 8
    .local @_l_c = 0
; .debug_var: __main @_l_c offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_enum.c", 9
    lda #0
    sta _main__local_0
    sta _main__local_0+1
    .loc "test_enum.c", 10
    lda _main__local_0
    ldx _main__local_0+1
    stx __zp_scratch
    ora __zp_scratch
    bne @if_then0
    bra @if_end2
@if_then0:
    lda #1
    ldx #0
    bra @__return
@if_end2:
    .loc "test_enum.c", 12
    lda #1
    sta _main__local_0
    lda #0
    sta _main__local_0+1
    .loc "test_enum.c", 13
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, #1
    bne @if_then3
    bra @if_end5
@if_then3:
    lda #2
    ldx #0
    bra @__return
@if_end5:
    .loc "test_enum.c", 15
    lda #5
    sta _main__local_0
    lda #0
    sta _main__local_0+1
    .loc "test_enum.c", 16
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, #5
    bne @if_then6
    bra @if_end8
@if_then6:
    lda #3
    ldx #0
    bra @__return
@if_end8:
    .loc "test_enum.c", 18
    lda #6
    sta _main__local_0
    lda #0
    sta _main__local_0+1
    .loc "test_enum.c", 19
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, #6
    bne @if_then9
    bra @if_end11
@if_then9:
    lda #4
    ldx #0
    bra @__return
@if_end11:
    .loc "test_enum.c", 30
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

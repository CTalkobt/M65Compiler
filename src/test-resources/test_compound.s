; [Phase 87: Peephole Optimizer Applied]
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
    _main__local_1: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "src/test-resources/test_compound.c", 16
    .local @_l_a = 0
; .debug_var: __main @_l_a offset=0 size=2 type=int16 scope=local

@entry:
    .loc "src/test-resources/test_compound.c", 2
    lda #10
    sta _main__local_1
    lda #0
    sta _main__local_1+1
    .loc "src/test-resources/test_compound.c", 3
    lda _main__local_1
    ldx _main__local_1+1
    add.16 .AX, #5
    sta _main__local_1
    stx _main__local_1+1
    .loc "src/test-resources/test_compound.c", 4
    lda _main__local_1
    ; [peephole-opt]     ldx _main__local_1+1
    sub.16 .AX, #2
    sta _main__local_1
    stx _main__local_1+1
    .loc "src/test-resources/test_compound.c", 5
    lda #3
    ldx #0
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_1
    ; [peephole-opt]     ldx _main__local_1+1
    mul.16 .AX, __zp_scratch2
    sta _main__local_1
    stx _main__local_1+1
    .loc "src/test-resources/test_compound.c", 6
    lda #4
    ldx #0
    lda _main__local_1
    ; [peephole-opt]     ldx _main__local_1+1
    lsr.16 .AX
    lsr.16 .AX
    sta _main__local_1
    stx _main__local_1+1
    .loc "src/test-resources/test_compound.c", 7
    lda #7
    ldx #0
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_1
    ; [peephole-opt]     ldx _main__local_1+1
    mod.16 .AX, __zp_scratch2
    sta _main__local_1
    stx _main__local_1+1
    .loc "src/test-resources/test_compound.c", 8
    lda #3
    ldx #0
    lda _main__local_1
    ; [peephole-opt]     ldx _main__local_1+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    ldy __zp_scratch2
    beq @__shl_done_1
@__shl_loop_0:
    lsl.16 .AX
    dey
    bne @__shl_loop_0
@__shl_done_1:
    sta _main__local_1
    stx _main__local_1+1
    .loc "src/test-resources/test_compound.c", 9
    lda #2
    ldx #0
    lda _main__local_1
    ; [peephole-opt]     ldx _main__local_1+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    ldy __zp_scratch2
    beq @__shr_done_3
@__shr_loop_2:
    lsr.16 .AX
    dey
    bne @__shr_loop_2
@__shr_done_3:
    sta _main__local_1
    stx _main__local_1+1
    .loc "src/test-resources/test_compound.c", 10
    lda #8
    ldx #0
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_1
    ; [peephole-opt]     ldx _main__local_1+1
    ora.16 .AX, __zp_scratch2
    sta _main__local_1
    stx _main__local_1+1
    .loc "src/test-resources/test_compound.c", 11
    lda #13
    ldx #0
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_1
    ; [peephole-opt]     ldx _main__local_1+1
    and.16 .AX, __zp_scratch2
    sta _main__local_1
    stx _main__local_1+1
    .loc "src/test-resources/test_compound.c", 12
    lda #15
    ldx #0
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_1
    ; [peephole-opt]     ldx _main__local_1+1
    eor.16 .AX, __zp_scratch2
    sta _main__local_1
    stx _main__local_1+1
    .loc "src/test-resources/test_compound.c", 13
@inline_end3:
    .loc "src/test-resources/test_compound.c", 17
    lda _main__local_1
    ; [peephole-opt]     ldx _main__local_1+1
    cmp.16 .AX, #3
    beq @if_then0
    bra @if_end2
@if_then0:
    .loc "src/test-resources/test_compound.c", 18
    lda #0
    ldx #0
    bra @__return
@if_end2:
    .loc "src/test-resources/test_compound.c", 20
    lda #1
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc


__zp_save_buf:
; [DEBUG] Phase 87 code reached, optimize=true

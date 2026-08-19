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
; SAC inline storage: 4 bytes
    _main__local_0: .word 0
    _main__local_2: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_for_decl.c", 1
    .local @_l_i = 2
    .local @_l_sum = 0
; .debug_var: __main @_l_i offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_sum offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_for_decl.c", 2
    lda #0
    sta _main__local_0
    sta _main__local_0+1
    .loc "test_for_decl.c", 3
    lda #0
    sta _main__local_2
    sta _main__local_2+1
@for_cond0:
    lda _main__local_2
    ldx _main__local_2+1
    cmp.16 .AX, #10
    bcc @for_body1
    bra @for_end3
@for_body1:
    .loc "test_for_decl.c", 4
    lda _main__local_2
    ldx _main__local_2+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_0
    ldx _main__local_0+1
    add.16 .AX, __zp_scratch2
    sta $24
    stx $25
    sta _main__local_0
    stx _main__local_0+1
@for_inc2:
    .loc "test_for_decl.c", 3
    inc.16f __vr2
    bra @for_cond0
@for_end3:
    .loc "test_for_decl.c", 10
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, #45
    beq @if_then4
    bra @if_end6
@if_then4:
    .loc "test_for_decl.c", 11
    lda #0
    ldx #0
    bra @__return
@if_end6:
    .loc "test_for_decl.c", 13
    lda #1
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc


__zp_save_buf:

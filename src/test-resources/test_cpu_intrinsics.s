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
    _main__local_22: .word 0
    _main__local_30: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_cpu_intrinsics.c", 1
    .local @_l_a = 2
    .local @_l_sum = 0
; .debug_var: __main @_l_a offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_sum offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_cpu_intrinsics.c", 3
    lda #66
    ldx #0
    sta $20
    stx $21
    lda $20
    .loc "test_cpu_intrinsics.c", 4
    sta $20
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $24
    ldx $25
    cmp.16 .AX, #66
    bne @if_then0
    bra @if_end2
@if_then0:
    lda #1
    ldx #0
    bra @__return
@if_end2:
    .loc "test_cpu_intrinsics.c", 6
    lda #16
    ldx #0
    sta $20
    stx $21
    ldx $20
    .loc "test_cpu_intrinsics.c", 7
    txa
    sta $20
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $24
    ldx $25
    cmp.16 .AX, #16
    bne @if_then3
    bra @if_end5
@if_then3:
    lda #2
    ldx #0
    bra @__return
@if_end5:
    .loc "test_cpu_intrinsics.c", 9
    lda #52
    ldx #18
    sta $20
    stx $21
    ldax $20
    .loc "test_cpu_intrinsics.c", 10
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #4660
    bne @if_then6
    bra @if_end8
@if_then6:
    lda #3
    ldx #0
    bra @__return
@if_end8:
    .loc "test_cpu_intrinsics.c", 13
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    lsr a
    .loc "test_cpu_intrinsics.c", 14
    lda #0
    adc #0
    sta $20
    lda $20
    bne @if_end11
@if_then9:
    lda #4
    ldx #0
    bra @__return
@if_end11:
    .loc "test_cpu_intrinsics.c", 16
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    lsr a
    .loc "test_cpu_intrinsics.c", 17
    lda #0
    adc #0
    sta $20
    lda $20
    bne @if_then12
    bra @if_end14
@if_then12:
    lda #5
    ldx #0
    bra @__return
@if_end14:
    .loc "test_cpu_intrinsics.c", 19
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp #0
    .loc "test_cpu_intrinsics.c", 20
    lda #0
    bne *+3
    lda #1
    sta $20
    lda $20
    bne @if_end17
@if_then15:
    lda #6
    ldx #0
    bra @__return
@if_end17:
    .loc "test_cpu_intrinsics.c", 23
    lda #10
    ldx #0
    sta $20
    stx $21
    lda $20
    .loc "test_cpu_intrinsics.c", 24
    lda #20
    ldx #0
    sta $20
    stx $21
    ldx $20
    .loc "test_cpu_intrinsics.c", 25
    sta $20
    txa
    sta $22
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx #0
    ldx #0
    sta $20
    stx $21
    lda $24
    clc
    adc $20
    sta $22
    lda $25
    adc $20+1
    sta $23
    lda $22
    ldx $23
    sta _main__local_22
    stx _main__local_22+1
    .loc "test_cpu_intrinsics.c", 26
    lda _main__local_22
    ldx _main__local_22+1
    cmp.16 .AX, #30
    bne @if_then18
    bra @if_end20
@if_then18:
    lda #7
    ldx #0
    bra @__return
@if_end20:
    .loc "test_cpu_intrinsics.c", 29
    lda #50
    ldx #0
    sta $20
    stx $21
    lda $20
    lda $20
    ldx $21
    sta _main__local_30
    stx _main__local_30+1
    .loc "test_cpu_intrinsics.c", 30
    lda _main__local_30
    ldx _main__local_30+1
    cmp.16 .AX, #50
    bne @if_then21
    bra @if_end23
@if_then21:
    lda #8
    ldx #0
    bra @__return
@if_end23:
    .loc "test_cpu_intrinsics.c", 31
    sta $20
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $24
    ldx $25
    cmp.16 .AX, #50
    bne @if_then24
    bra @if_end26
@if_then24:
    lda #9
    ldx #0
    bra @__return
@if_end26:
    .loc "test_cpu_intrinsics.c", 33
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc


__zp_save_buf:

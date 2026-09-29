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

    .global _factorial
    .global _main

    .segment "code"

; function _factorial
    proc _factorial, W#@_p_n
    .var _fp = 0
    .loc "test_recursion.c", 1
; frame: 2 bytes (frame-allocated vRegs only)
    phw #0
    tsy
    tsx
    inx
    bne @__fp_no_carry_0
    iny
@__fp_no_carry_0:
    stx $FD
    sty $FE
    .frameptr_zp $FD
    .local __vr0 = 0
    .local __vr4 = 2
    .local __vr5 = 0
    .local __vr6 = 0
    .local __vr11 = 4
    .var @_p_n = 4
; .debug_var: __factorial @_p_n offset=4 size=2 type=int16 scope=parameter

    ldax.param @_p_n
    stax.local __vr0
@entry:
    .loc "test_recursion.c", 2
    ldax.local __vr0
    stx __zp_scratch
    ora __zp_scratch
    beq @if_then0
    bra @if_end2
@if_then0:
    lda #1
    ldx #0
    bra @__return
@if_end2:
    .loc "test_recursion.c", 3
    ldax.local __vr0
    sub.16 .AX, #1
    stax.local __vr4
    .loc "test_recursion.c", 2
    ldax.local __vr4
    stx __zp_scratch
    ora __zp_scratch
    beq @if_then4
    bra @if_end6
@if_then4:
    lda #1
    ldx #0
    sta $20
    stx $21
    bra @inline_end3
@if_end6:
    .loc "test_recursion.c", 3
    ldax.local __vr4
    sub.16 .AX, #1
    stax.local __vr11
    ldax.local __vr11
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    jsr _factorial
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    sta $20
    stx $21
    ldax.local __vr4
    mul.16 .AX, $20
    sta $22
    stx $23
@inline_end3:
    ldax.local __vr0
    mul.16 .AX, $22
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    plz
    plz
    rts
    .func_flags stack_call
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _main
; SAC inline storage: 0 bytes
    _main__local_0: .word 0
    _main__local_1: .word 0
    _main__local_2: .word 0
    _main__local_7: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_recursion.c", 6

@entry:
    .loc "test_recursion.c", 7
    lda #5
    ldx #0
    sta _main__local_0
    stx _main__local_0+1
    .loc "test_recursion.c", 2
    lda _main__local_0
    ldx _main__local_0+1
    stx __zp_scratch
    ora __zp_scratch
    beq @if_then10
    bra @if_end12
@if_then10:
    lda #1
    ldx #0
    sta $20
    stx $21
    bra @inline_end9
@if_end12:
    .loc "test_recursion.c", 3
    lda _main__local_0
    ldx _main__local_0+1
    sub.16 .AX, #1
    sta _main__local_7
    stx _main__local_7+1
    lda _main__local_7
    ldx _main__local_7+1
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    jsr _factorial
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    sta $20
    stx $21
    lda _main__local_0
    ldx _main__local_0+1
    mul.16 .AX, $20
    sta $22
    stx $23
@inline_end9:
    .loc "test_recursion.c", 7
    lda $22
    ldx $23
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 0
    endproc


__zp_save_buf:

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

    .global _get_ten
    .global _get_five
    .global _multiply
    .global _main

    .segment "code"

; function _get_ten
; SAC zero-alloc leaf: no storage overhead
    proc _get_ten
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_multi_call.c", 4

@entry:
    .loc "test_multi_call.c", 5
    lda #10
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, zeroalloc, leaf
    .reg_clobbers A, X
    .flag_clobbers N, Z
    .frame_size 0
    endproc

; function _get_five
; SAC zero-alloc leaf: no storage overhead
    proc _get_five
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_multi_call.c", 8

@entry:
    .loc "test_multi_call.c", 9
    lda #5
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, zeroalloc, leaf
    .reg_clobbers A, X
    .flag_clobbers N, Z
    .frame_size 0
    endproc

; function _multiply
; SAC inline storage: 4 bytes
    .global _multiply__param_a
    _multiply__param_a: .word 0
    .global _multiply__param_b
    _multiply__param_b: .word 0
    _multiply__local_0: .word 0
    _multiply__local_1: .word 0
    proc _multiply, W#@_p_a, W#@_p_b
    .sac
    .var _fp = 0
    .loc "test_multi_call.c", 12
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __multiply @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __multiply @_p_b offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_multi_call.c", 13
    lda _multiply__param_b
    ldx _multiply__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _multiply__param_a
    ldx _multiply__param_a+1
    mul.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _main
; SAC inline storage: 14 bytes
    _main__local_0: .word 0
    _main__local_93: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_multi_call.c", 16
    .local @_l_arr = 2
    .local @_l_total = 0
; .debug_var: __main @_l_arr offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_total offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_multi_call.c", 5
    lda #10
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end0:
    .loc "test_multi_call.c", 20
    leax.local 2
    sta $20
    stx $21
    lda #0
    sta $24
    sta $25
    lda $22
    ldx $23
    pha
    phx
    lda $24
    ldx $25
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    .loc "test_multi_call.c", 9
    lda #5
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end2:
    .loc "test_multi_call.c", 21
    leax.local 2
    sta $20
    stx $21
    lda #1
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    pha
    phx
    lda $24
    ldx $25
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    .loc "test_multi_call.c", 5
    lda #10
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end4:
    .loc "test_multi_call.c", 9
    lda #5
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $24
    stx $25
@inline_end6:
    .loc "test_multi_call.c", 22
    lda $22
    ldx $23
    sta $20
    stx $21
    lda $24
    ldx $25
    sta $20
    stx $21
    .loc "test_multi_call.c", 13
    lda $22
    ldx $23
    mul.16 .AX, $24
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end8:
    .loc "test_multi_call.c", 22
    leax.local 2
    sta $20
    stx $21
    lda #2
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    pha
    phx
    lda $24
    ldx $25
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    .loc "test_multi_call.c", 23
    leax.local 2
    sta $20
    stx $21
    lda #0
    sta $22
    sta $23
    lda $22
    ldx $23
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $26
    stx $27
    lda #2
    ldx #0
    sta $20
    stx $21
    lda $26
    ldx $27
    sta $22
    stx $23
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_multi_call.c", 13
    lda $26
    ldx $27
    mul.16 .AX, $20
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end10:
    .loc "test_multi_call.c", 23
    leax.local 2
    sta $22
    stx $23
    lda #3
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx $21
    pha
    phx
    lda $24
    ldx $25
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $22
    ldx $22+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    .loc "test_multi_call.c", 9
    lda #5
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end12:
    lda #5
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $24
    stx $25
@inline_end14:
    .loc "test_multi_call.c", 24
    lda $22
    ldx $23
    sta $20
    stx $21
    lda $24
    ldx $25
    sta $20
    stx $21
    .loc "test_multi_call.c", 13
    lda $22
    ldx $23
    mul.16 .AX, $24
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end16:
    .loc "test_multi_call.c", 24
    leax.local 2
    sta $20
    stx $21
    lda #4
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    pha
    phx
    lda $24
    ldx $25
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    .loc "test_multi_call.c", 25
    leax.local 2
    sta $20
    stx $21
    lda #2
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $26
    stx $27
    leax.local 2
    sta $20
    stx $21
    lda #4
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $28
    stx $29
    lda $26
    clc
    adc $28
    sta $20
    lda $27
    adc $28+1
    sta $21
    leax.local 2
    sta $22
    stx $23
    lda #5
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx $21
    pha
    phx
    lda $24
    ldx $25
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $22
    ldx $22+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    .loc "test_multi_call.c", 27
    leax.local 2
    sta $20
    stx $21
    lda #0
    sta $22
    sta $23
    lda $22
    ldx $23
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $26
    stx $27
    lda $26
    ldx $27
    cmp.16 .AX, #10
    bne @if_then18
    bra @if_end20
@if_then18:
    lda #1
    ldx #0
    bra @__return
@if_end20:
    .loc "test_multi_call.c", 28
    leax.local 2
    sta $20
    stx $21
    lda #1
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $26
    stx $27
    lda $26
    ldx $27
    cmp.16 .AX, #5
    bne @if_then21
    bra @if_end23
@if_then21:
    lda #2
    ldx #0
    bra @__return
@if_end23:
    .loc "test_multi_call.c", 29
    leax.local 2
    sta $20
    stx $21
    lda #2
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $26
    stx $27
    lda $26
    ldx $27
    cmp.16 .AX, #50
    bne @if_then24
    bra @if_end26
@if_then24:
    lda #3
    ldx #0
    bra @__return
@if_end26:
    .loc "test_multi_call.c", 30
    leax.local 2
    sta $20
    stx $21
    lda #3
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $26
    stx $27
    lda $26
    ldx $27
    cmp.16 .AX, #20
    bne @if_then27
    bra @if_end29
@if_then27:
    lda #4
    ldx #0
    bra @__return
@if_end29:
    .loc "test_multi_call.c", 31
    leax.local 2
    sta $20
    stx $21
    lda #4
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $26
    stx $27
    lda $26
    ldx $27
    cmp.16 .AX, #25
    bne @if_then30
    bra @if_end32
@if_then30:
    lda #5
    ldx #0
    bra @__return
@if_end32:
    .loc "test_multi_call.c", 32
    leax.local 2
    sta $20
    stx $21
    lda #5
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $26
    stx $27
    lda $26
    ldx $27
    cmp.16 .AX, #75
    bne @if_then33
    bra @if_end35
@if_then33:
    lda #6
    ldx #0
    bra @__return
@if_end35:
    .loc "test_multi_call.c", 5
    lda #10
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end36:
    .loc "test_multi_call.c", 35
    lda #2
    ldx #0
    sta $20
    stx $21
    lda $22
    ldx $23
    sta $24
    stx $25
    lda $20
    ldx $21
    sta $24
    stx $25
    .loc "test_multi_call.c", 13
    lda $22
    ldx $23
    mul.16 .AX, $20
    sta $24
    stx $25
    lda $24
    ldx $25
    sta $20
    stx $21
@inline_end38:
    .loc "test_multi_call.c", 9
    lda #5
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
@inline_end40:
    .loc "test_multi_call.c", 35
    lda #3
    ldx #0
    sta $22
    stx $23
    lda $24
    ldx $25
    sta $26
    stx $27
    lda $22
    ldx $23
    sta $26
    stx $27
    .loc "test_multi_call.c", 13
    lda $24
    ldx $25
    mul.16 .AX, $22
    sta $26
    stx $27
    lda $26
    ldx $27
    sta $22
    stx $23
@inline_end42:
    .loc "test_multi_call.c", 35
    lda $20
    clc
    adc $22
    sta $24
    lda $21
    adc $22+1
    sta $25
    lda $24
    ldx $25
    sta _main__local_93
    stx _main__local_93+1
    .loc "test_multi_call.c", 36
    lda _main__local_93
    ldx _main__local_93+1
    cmp.16 .AX, #35
    bne @if_then44
    bra @if_end46
@if_then44:
    lda #7
    ldx #0
    bra @__return
@if_end46:
    .loc "test_multi_call.c", 38
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 14
    endproc


__zp_save_buf:

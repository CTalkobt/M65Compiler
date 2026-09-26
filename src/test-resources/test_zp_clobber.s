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

    .global _double_it
    .global _add
    .global _main

    .segment "code"

; function _double_it
; SAC inline storage: 2 bytes
    .global _double_it__param_x
    _double_it__param_x: .word 0
    _double_it__local_0: .word 0
    proc _double_it, W#@_p_x
    .sac
    .var _fp = 0
    .loc "test_zp_clobber.c", 4
    .var @_p_x = 2
; .debug_var: __double_it @_p_x offset=2 size=2 type=int16 scope=parameter

@entry:
    .loc "test_zp_clobber.c", 5
    lda #2
    ldx #0
    sta $20
    stx $21
    lda _double_it__param_x
    ldx _double_it__param_x+1
    lsl.16 .AX
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z
    .frame_size 2
    endproc

; function _add
; SAC inline storage: 4 bytes
    .global _add__param_a
    _add__param_a: .word 0
    .global _add__param_b
    _add__param_b: .word 0
    _add__local_0: .word 0
    _add__local_1: .word 0
    proc _add, W#@_p_a, W#@_p_b
    .sac
    .var _fp = 0
    .loc "test_zp_clobber.c", 8
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __add @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __add @_p_b offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_zp_clobber.c", 9
    lda _add__param_b
    ldx _add__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _add__param_a
    ldx _add__param_a+1
    add.16 .AX, __zp_scratch2
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
; SAC inline storage: 10 bytes
    _main__local_0: .word 0
    _main__local_9: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_zp_clobber.c", 12
    .local @_l_idx = 0
    .local @_l_results = 2
; .debug_var: __main @_l_idx offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_results offset=2 size=2 type=int16 scope=local

@entry:
    .loc "test_zp_clobber.c", 16
    lda #5
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_zp_clobber.c", 5
    lda #2
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    lsl.16 .AX
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end0:
    .loc "test_zp_clobber.c", 16
    leax.local 2
    sta $22
    stx $23
    lda #0
    sta $24
    sta $25
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
    .loc "test_zp_clobber.c", 19
    lda #1
    sta _main__local_9
    lda #0
    sta _main__local_9+1
    .loc "test_zp_clobber.c", 20
    lda #3
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_zp_clobber.c", 5
    lda #2
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    lsl.16 .AX
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end2:
    .loc "test_zp_clobber.c", 20
    leax.local 2
    sta $22
    stx $23
    lda $20
    ldx $21
    pha
    phx
    lda _main__local_9
    ldx _main__local_9+1
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
    .loc "test_zp_clobber.c", 23
    lda #2
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_zp_clobber.c", 5
    lda #2
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    lsl.16 .AX
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end4:
    .loc "test_zp_clobber.c", 23
    lda #1
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    sta $24
    stx $25
    lda $22
    ldx $23
    sta $24
    stx $25
    .loc "test_zp_clobber.c", 9
    lda $20
    clc
    adc #1
    sta $24
    lda $21
    adc #0
    sta $25
    lda $24
    ldx $25
    sta $20
    stx $21
@inline_end6:
    .loc "test_zp_clobber.c", 23
    leax.local 2
    sta $22
    stx $23
    lda #2
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
    .loc "test_zp_clobber.c", 26
    lda #10
    ldx #0
    sta $20
    stx $21
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
    .loc "test_zp_clobber.c", 27
    lda #5
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_zp_clobber.c", 5
    lda #2
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    lsl.16 .AX
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
@inline_end8:
    .loc "test_zp_clobber.c", 27
    leax.local 2
    sta $22
    stx $23
    lda #3
    ldx #0
    sta $24
    stx $25
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
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $28
    stx $29
    lda $28
    clc
    adc $20
    sta $22
    lda $29
    adc $20+1
    sta $23
    leax.local 2
    sta $20
    stx $21
    lda #3
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
    .loc "test_zp_clobber.c", 30
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
    bne @if_then10
    bra @if_end12
@if_then10:
    lda #1
    ldx #0
    bra @__return
@if_end12:
    .loc "test_zp_clobber.c", 31
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
    cmp.16 .AX, #6
    bne @if_then13
    bra @if_end15
@if_then13:
    lda #2
    ldx #0
    bra @__return
@if_end15:
    .loc "test_zp_clobber.c", 32
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
    cmp.16 .AX, #5
    bne @if_then16
    bra @if_end18
@if_then16:
    lda #3
    ldx #0
    bra @__return
@if_end18:
    .loc "test_zp_clobber.c", 33
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
    bne @if_then19
    bra @if_end21
@if_then19:
    lda #4
    ldx #0
    bra @__return
@if_end21:
    .loc "test_zp_clobber.c", 34
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 10
    endproc


__zp_save_buf:

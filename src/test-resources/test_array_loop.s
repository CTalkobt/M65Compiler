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

    .global _result
    .global _scores
    .global _grid
    .global _main

    .segment "data"
    .byte 0
_result:
; .debug_var: @global _result offset=0 size=2 type=ptr scope=global
    .word 16384
_scores:
; .debug_var: @global _scores offset=0 size=2 type=int8 scope=global
    .byte 0
    .res 4
_grid:
; .debug_var: @global _grid offset=0 size=2 type=int16 scope=global
    .word 0
    .res 22

    .segment "code"

; function _main
; SAC inline storage: 4 bytes
    _main__local_0: .word 0
    _main__local_1: .word 0
    _main__local_7: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_array_loop.c", 9
    .local @_l_i = 0
    .local @_l_j = 2
; .debug_var: __main @_l_i offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_j offset=2 size=2 type=int16 scope=local

@entry:
    .loc "test_array_loop.c", 14
    lda #0
    sta _main__local_0
    sta _main__local_0+1
@for_cond0:
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, #5
    bcc @for_body1
    bra @for_end3
@for_body1:
    .loc "test_array_loop.c", 15
    lda _main__local_0
    ldx _main__local_0+1
    clc
    adc #1
    sta $26
    stx $27
    lda $26
    ldx $27
    pha
    lda _main__local_0
    ldx _main__local_0+1
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_scores
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
@for_inc2:
    .loc "test_array_loop.c", 14
    lda _main__local_0
    ldx _main__local_0+1
    sta $2A
    stx $2B
    lda $2A
    clc
    adc #1
    sta $2C
    lda $2B
    adc #0
    sta $2D
    lda $2C
    ldx $2D
    sta _main__local_0
    stx _main__local_0+1
    bra @for_cond0
@for_end3:
    .loc "test_array_loop.c", 19
    lda #0
    sta _main__local_0
    sta _main__local_0+1
@for_cond4:
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, #3
    bcc @for_body5
    bra @for_end7
@for_body5:
    .loc "test_array_loop.c", 20
    lda #0
    sta _main__local_1
    sta _main__local_1+1
@for_cond8_ph:
    .loc "test_array_loop.c", 21
    lda #10
    ldx #0
    sta $26
    stx $27
    lda _main__local_0
    ldx _main__local_0+1
    mul.16 .AX, $26
    sta $28
    stx $29
    lda _main__local_0
    ldx _main__local_0+1
    mul.16 .AX, #8
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_grid
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2A
    stx $2A+1
@for_cond8:
    .loc "test_array_loop.c", 20
    lda _main__local_1
    ldx _main__local_1+1
    cmp.16 .AX, #4
    bcc @for_body9
    bra @for_end11
@for_body9:
    .loc "test_array_loop.c", 21
    lda #10
    ldx #0
    sta $30
    stx $31
    lda _main__local_1
    ldx _main__local_1+1
    add.16 .AX, $28
    sta $32
    stx $33
    lda $32
    ldx $33
    pha
    phx
    lda _main__local_1
    ldx _main__local_1+1
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $2A
    ldx $2A+1
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
@for_inc10:
    .loc "test_array_loop.c", 20
    lda _main__local_1
    ldx _main__local_1+1
    sta $36
    stx $37
    lda $36
    clc
    adc #1
    sta $38
    lda $37
    adc #0
    sta $39
    lda $38
    ldx $39
    sta _main__local_1
    stx _main__local_1+1
    bra @for_cond8
@for_end11:
@for_inc6:
    .loc "test_array_loop.c", 19
    lda _main__local_0
    ldx _main__local_0+1
    sta $3A
    stx $3B
    lda $3A
    clc
    adc #1
    sta $3C
    lda $3B
    adc #0
    sta $3D
    lda $3C
    ldx $3D
    sta _main__local_0
    stx _main__local_0+1
    bra @for_cond4
@for_end7:
    .loc "test_array_loop.c", 26
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_scores
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
    ldx #0
    sta $24
    lda _result
    ldx _result+1
    sta $20
    stx $21
    lda #0
    sta $22
    sta $23
    lda $24
    ldx #0
    pha
    lda $22
    ldx $23
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
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "test_array_loop.c", 29
    lda #4
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_scores
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
    ldx #0
    sta $24
    lda _result
    ldx _result+1
    sta $20
    stx $21
    lda #1
    ldx #0
    sta $22
    stx $23
    lda $24
    ldx #0
    pha
    lda $22
    ldx $23
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
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "test_array_loop.c", 32
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    mul.16 .AX, #8
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_grid
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
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
    sta $26
    stx $27
    lda $26
    ldx $27
    sta $20
    lda _result
    ldx _result+1
    sta $22
    stx $23
    lda #2
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx #0
    pha
    lda $24
    ldx $25
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
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "test_array_loop.c", 35
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    mul.16 .AX, #8
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_grid
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
    lda #2
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
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
    sta $26
    stx $27
    lda $26
    ldx $27
    sta $20
    lda _result
    ldx _result+1
    sta $22
    stx $23
    lda #3
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx #0
    pha
    lda $24
    ldx $25
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
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "test_array_loop.c", 38
    lda #2
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    mul.16 .AX, #8
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_grid
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
    lda #3
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
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
    sta $26
    stx $27
    lda $26
    ldx $27
    sta $20
    lda _result
    ldx _result+1
    sta $22
    stx $23
    lda #4
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx #0
    pha
    lda $24
    ldx $25
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
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "test_array_loop.c", 41
    lda #170
    sta $20
    lda _result
    ldx _result+1
    sta $22
    stx $23
    lda #5
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx #0
    pha
    lda $24
    ldx $25
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
    pla
    ldy #0
    sta (__zp_scratch),y
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc


__zp_save_buf:

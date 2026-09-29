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
; SAC inline storage: 30 bytes
    _main__local_0: .word 0
    _main__local_2: .word 0
    _main__local_4: .word 0
    _main__local_6: .long 0
    _main__local_8: .word 0
    _main__local_11: .word 0
    _main__local_13: .long 0
    _main__local_18: .word 0
    _main__local_28: .word 0
    _main__local_38: .word 0
    _main__local_44: .word 0
    _main__local_46: .word 0
    _main__local_47: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_typedef.c", 10
    .local @_l_a = 0
    .local @_l_b = 2
    .local @_l_c = 6
    .local @_l_cmds = 18
    .local @_l_i = 12
    .local @_l_p = 14
    .local @_l_pCmd = 10
    .local @_l_s = 4
    .local @_l_sum = 8
; .debug_var: __main @_l_a offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_b offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_c offset=6 size=2 type=int16 scope=local
; .debug_var: __main @_l_cmds offset=18 size=4 type=int32 scope=local
; .debug_var: __main @_l_i offset=12 size=2 type=int16 scope=local
; .debug_var: __main @_l_p offset=14 size=4 type=int32 scope=local
; .debug_var: __main @_l_pCmd offset=10 size=2 type=ptr scope=local
; .debug_var: __main @_l_s offset=4 size=2 type=ptr scope=local
; .debug_var: __main @_l_sum offset=8 size=2 type=int16 scope=local

@entry:
    .loc "test_typedef.c", 11
    lda #10
    sta _main__local_0
    lda #0
    sta _main__local_0+1
    .loc "test_typedef.c", 12
    lda #20
    sta _main__local_2
    lda #0
    sta _main__local_2+1
    .loc "test_typedef.c", 13
    .loc "test_typedef.c", 15
    leax.local 14
    sta $20
    stx $21
    lda _main__local_0
    ldx _main__local_0+1
    ldy #0
    sta ($20),y
    txa
    iny
    sta ($20),y
    .loc "test_typedef.c", 16
    leax.local 14
    sta $20
    stx $21
    lda $20
    ldx $21
    add.16 .AX, #2
    sta $22
    stx $23
    lda _main__local_2
    ldx _main__local_2+1
    ldy #0
    sta ($22),y
    txa
    iny
    sta ($22),y
    .loc "test_typedef.c", 19
    lda #30
    sta _main__local_11
    lda #0
    sta _main__local_11+1
    .loc "test_typedef.c", 24
    lda #1
    ldx #0
    sta $20
    stx $21
    leax.local 18
    sta $22
    stx $23
    lda #0
    sta $24
    sta $25
    lda $24
    ldx $25
    mul.16 .AX, #4
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
    sta $26
    stx $26+1
    lda $20
    ldx $21
    ldy #0
    sta ($26),y
    txa
    iny
    sta ($26),y
    lda #10
    ldx #0
    sta $20
    stx $21
    leax.local 18
    sta $22
    stx $23
    lda #0
    sta $24
    sta $25
    lda $24
    ldx $25
    mul.16 .AX, #4
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
    sta $26
    stx $26+1
    lda $26
    ldx $27
    add.16 .AX, #2
    sta $22
    stx $23
    lda $20
    ldx $21
    ldy #0
    sta ($22),y
    txa
    iny
    sta ($22),y
    .loc "test_typedef.c", 25
    lda #2
    ldx #0
    sta $20
    stx $21
    leax.local 18
    sta $22
    stx $23
    lda #1
    ldx #0
    sta $24
    stx $25
    lda $24
    ldx $25
    mul.16 .AX, #4
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
    sta $26
    stx $26+1
    lda $20
    ldx $21
    ldy #0
    sta ($26),y
    txa
    iny
    sta ($26),y
    lda #20
    ldx #0
    sta $20
    stx $21
    leax.local 18
    sta $22
    stx $23
    lda #1
    ldx #0
    sta $24
    stx $25
    lda $24
    ldx $25
    mul.16 .AX, #4
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
    sta $26
    stx $26+1
    lda $26
    ldx $27
    add.16 .AX, #2
    sta $22
    stx $23
    lda $20
    ldx $21
    ldy #0
    sta ($22),y
    txa
    iny
    sta ($22),y
    .loc "test_typedef.c", 26
    lda #3
    ldx #0
    sta $20
    stx $21
    leax.local 18
    sta $22
    stx $23
    lda #2
    ldx #0
    sta $24
    stx $25
    lda $24
    ldx $25
    mul.16 .AX, #4
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
    sta $26
    stx $26+1
    lda $20
    ldx $21
    ldy #0
    sta ($26),y
    txa
    iny
    sta ($26),y
    lda #30
    ldx #0
    sta $20
    stx $21
    leax.local 18
    sta $22
    stx $23
    lda #2
    ldx #0
    sta $24
    stx $25
    lda $24
    ldx $25
    mul.16 .AX, #4
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
    sta $26
    stx $26+1
    lda $26
    ldx $27
    add.16 .AX, #2
    sta $22
    stx $23
    lda $20
    ldx $21
    ldy #0
    sta ($22),y
    txa
    iny
    sta ($22),y
    .loc "test_typedef.c", 28
    lda #0
    sta _main__local_44
    sta _main__local_44+1
    .loc "test_typedef.c", 31
    lda #0
    sta _main__local_47
    sta _main__local_47+1
@for_cond1_ph:
    .loc "test_typedef.c", 32
    leax.local 18
    sta $20
    stx $21
@for_cond1:
    .loc "test_typedef.c", 31
    lda _main__local_47
    ldx _main__local_47+1
    cmp.16 .AX, #3
    bcc @for_body2
    bra @for_end4
@for_body2:
    .loc "test_typedef.c", 32
    lda _main__local_47
    ldx _main__local_47+1
    mul.16 .AX, #4
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
    sta $26
    stx $26+1
    lda $26
    ldx $27
    sta _main__local_46
    stx _main__local_46+1
    .loc "test_typedef.c", 33
    lda _main__local_46
    ldx _main__local_46+1
    add.16 .AX, #2
    sta $28
    stx $29
    ldy #0
    lda ($28),y
    pha
    iny
    lda ($28),y
    tax
    pla
    sta $2A
    stx $2B
    lda _main__local_44
    ldx _main__local_44+1
    add.16 .AX, $2A
    sta $2C
    stx $2D
    sta _main__local_44
    stx _main__local_44+1
@for_inc3:
    .loc "test_typedef.c", 31
    lda _main__local_47
    ldx _main__local_47+1
    sta $2E
    stx $2F
    lda $2E
    clc
    adc #1
    sta $30
    lda $2F
    adc #0
    sta $31
    lda $30
    ldx $31
    sta _main__local_47
    stx _main__local_47+1
    bra @for_cond1
@for_end4:
    .loc "test_typedef.c", 36
    lda _main__local_44
    ldx _main__local_44+1
    sub.16 .AX, #60
    sta $22
    stx $23
    sta _main__local_11
    stx _main__local_11+1
    .loc "test_typedef.c", 38
    lda _main__local_2
    ldx _main__local_2+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_0
    ldx _main__local_0+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda _main__local_11
    ldx _main__local_11+1
    add.16 .AX, $20
    sta $22
    stx $23
    lda $22
    clc
    adc #30
    sta $24
    lda $23
    adc #0
    sta $25
    lda $24
    ldx $25
    cmp.16 .AX, #60
    beq @if_then5
    bra @if_end7
@if_then5:
    .loc "test_typedef.c", 39
    lda #0
    ldx #0
    bra @__return
@if_end7:
    .loc "test_typedef.c", 41
    lda #1
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 30
    endproc


    .segment "data"
__str_0:
    .text "hello"
    .byte 0

__zp_save_buf:

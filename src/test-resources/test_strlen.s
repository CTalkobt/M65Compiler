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

    .global _strlen
    .global _main

    .segment "code"

; function _strlen
; SAC inline storage: 4 bytes
    .global _strlen__param_s
    _strlen__param_s: .word 0
    _strlen__local_0: .word 0
    _strlen__local_1: .word 0
    proc _strlen, W#@_p_s
    .sac
    .var _fp = 0
    .loc "test_strlen.c", 4
    .local @_l_len = 2
; .debug_var: __strlen @_l_len offset=2 size=2 type=int16 scope=local
    .var @_p_s = 2
; .debug_var: __strlen @_p_s offset=2 size=2 type=ptr scope=parameter

@entry:
    .loc "test_strlen.c", 5
    lda #0
    sta _strlen__local_1
    sta _strlen__local_1+1
@while_cond0_ph:
    .loc "test_strlen.c", 6
    lda #0
    sta $20
    lda $20
    ldx #0
    ldx #0
    sta $22
    stx $23
@while_cond0:
    lda _strlen__param_s
    ldx _strlen__param_s+1
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $20
    lda #0
    sta $24
    lda $20
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $26
    ldx $27
    cmp.16 .AX, $22
    bne @while_body1
    bra @while_end2
@while_body1:
    .loc "test_strlen.c", 7
    inc.16f __vr1
    .loc "test_strlen.c", 8
    lda _strlen__param_s
    ldx _strlen__param_s+1
    add.16 .AX, #1
    sta $30
    stx $31
    sta _strlen__param_s
    stx _strlen__param_s+1
    bra @while_cond0
@while_end2:
    .loc "test_strlen.c", 10
    lda _strlen__local_1
    ldx _strlen__local_1+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _main
; SAC inline storage: 8 bytes
    _main__local_51: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_strlen.c", 13
    .local @_l_len = 6
; .debug_var: __main @_l_len offset=6 size=2 type=int16 scope=local

@entry:
    .loc "test_strlen.c", 14
    ldax #__str_6
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_strlen.c", 5
    lda #0
    sta $24
    sta $25
@while_cond8_ph:
    .loc "test_strlen.c", 6
    lda #0
    sta $20
    .loc "test_strlen.c", 8
    lda $20
    ldx #0
    ldx #0
    sta $28
    stx $29
    lda $22
    ldx $23
    add.16 .AX, #1
    sta $20
    stx $21
@while_cond8:
    .loc "test_strlen.c", 6
    ldy #0
    lda ($22),y
    ldx #0
    sta $26
    lda #0
    sta $2A
    lda $26
    ldx #0
    ldx #0
    sta $2C
    stx $2D
    lda $2C
    ldx $2D
    cmp.16 .AX, $28
    bne @while_body9
    bra @while_end10
@while_body9:
    .loc "test_strlen.c", 7
    inc $24
    bne *+4
    inc $25
    .loc "test_strlen.c", 8
    lda #1
    ldx #0
    sta $34
    stx $35
    lda $20
    ldx $21
    ldy #0
    sta ($22),y
    txa
    iny
    sta ($22),y
    bra @while_cond8
@while_end10:
    .loc "test_strlen.c", 10
    lda $24
    ldx $25
    sta $20
    stx $21
@inline_end7:
    .loc "test_strlen.c", 14
    lda $20
    ldx $21
    cmp.16 .AX, #5
    bne @if_then3
    bra @if_end5
@if_then3:
    lda #1
    ldx #0
    bra @__return
@if_end5:
    .loc "test_strlen.c", 15
    ldax #__str_15
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_strlen.c", 5
    lda #0
    sta $24
    sta $25
@while_cond17_ph:
    .loc "test_strlen.c", 6
    lda #0
    sta $20
    .loc "test_strlen.c", 8
    lda $20
    ldx #0
    ldx #0
    sta $28
    stx $29
    lda $22
    ldx $23
    add.16 .AX, #1
    sta $20
    stx $21
@while_cond17:
    .loc "test_strlen.c", 6
    ldy #0
    lda ($22),y
    ldx #0
    sta $26
    lda #0
    sta $2A
    lda $26
    ldx #0
    ldx #0
    sta $2C
    stx $2D
    lda $2C
    ldx $2D
    cmp.16 .AX, $28
    bne @while_body18
    bra @while_end19
@while_body18:
    .loc "test_strlen.c", 7
    inc $24
    bne *+4
    inc $25
    .loc "test_strlen.c", 8
    lda #1
    ldx #0
    sta $34
    stx $35
    lda $20
    ldx $21
    ldy #0
    sta ($22),y
    txa
    iny
    sta ($22),y
    bra @while_cond17
@while_end19:
    .loc "test_strlen.c", 10
    lda $24
    ldx $25
    sta $20
    stx $21
@inline_end16:
    .loc "test_strlen.c", 15
    lda $20
    ora $21
    bne @if_then12
    bra @if_end14
@if_then12:
    lda #2
    ldx #0
    bra @__return
@if_end14:
    .loc "test_strlen.c", 16
    ldax #__str_24
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_strlen.c", 5
    lda #0
    sta $24
    sta $25
@while_cond26_ph:
    .loc "test_strlen.c", 6
    lda #0
    sta $20
    .loc "test_strlen.c", 8
    lda $20
    ldx #0
    ldx #0
    sta $28
    stx $29
    lda $22
    ldx $23
    add.16 .AX, #1
    sta $20
    stx $21
@while_cond26:
    .loc "test_strlen.c", 6
    ldy #0
    lda ($22),y
    ldx #0
    sta $26
    lda #0
    sta $2A
    lda $26
    ldx #0
    ldx #0
    sta $2C
    stx $2D
    lda $2C
    ldx $2D
    cmp.16 .AX, $28
    bne @while_body27
    bra @while_end28
@while_body27:
    .loc "test_strlen.c", 7
    inc $24
    bne *+4
    inc $25
    .loc "test_strlen.c", 8
    lda #1
    ldx #0
    sta $34
    stx $35
    lda $20
    ldx $21
    ldy #0
    sta ($22),y
    txa
    iny
    sta ($22),y
    bra @while_cond26
@while_end28:
    .loc "test_strlen.c", 10
    lda $24
    ldx $25
    sta $20
    stx $21
@inline_end25:
    .loc "test_strlen.c", 16
    lda $20
    ldx $21
    cmp.16 .AX, #1
    bne @if_then21
    bra @if_end23
@if_then21:
    lda #3
    ldx #0
    bra @__return
@if_end23:
    .loc "test_strlen.c", 17
    ldax #__str_33
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_strlen.c", 5
    lda #0
    sta _main__local_51
    sta _main__local_51+1
@while_cond35_ph:
    .loc "test_strlen.c", 6
    lda #0
    sta $20
    .loc "test_strlen.c", 8
    lda $20
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $22
    ldx $23
    add.16 .AX, #1
    sta $20
    stx $21
@while_cond35:
    .loc "test_strlen.c", 6
    ldy #0
    lda ($22),y
    ldx #0
    sta $24
    lda #0
    sta $28
    lda $24
    ldx #0
    ldx #0
    sta $2A
    stx $2B
    lda $2A
    ldx $2B
    cmp.16 .AX, $26
    bne @while_body36
    bra @while_end37
@while_body36:
    .loc "test_strlen.c", 7
    inc.16f __vr51
    .loc "test_strlen.c", 8
    lda #1
    ldx #0
    sta $32
    stx $33
    lda $20
    ldx $21
    ldy #0
    sta ($22),y
    txa
    iny
    sta ($22),y
    bra @while_cond35
@while_end37:
    .loc "test_strlen.c", 10
    lda _main__local_51
    ldx _main__local_51+1
    sta $20
    stx $21
@inline_end34:
    .loc "test_strlen.c", 17
    lda $20
    ldx $21
    cmp.16 .AX, #10
    bne @if_then30
    bra @if_end32
@if_then30:
    lda #4
    ldx #0
    bra @__return
@if_end32:
    .loc "test_strlen.c", 18
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 8
    endproc


    .segment "data"
__str_6:
    .text "hello"
    .byte 0
__str_15:
    .text ""
    .byte 0
__str_24:
    .text "a"
    .byte 0
__str_33:
    .text "abcdefghij"
    .byte 0

__zp_save_buf:

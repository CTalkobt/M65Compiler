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

    .global _strcmp
    .global _main

    .segment "code"

; function _strcmp
; SAC inline storage: 4 bytes
    .global _strcmp__param_s1
    _strcmp__param_s1: .word 0
    .global _strcmp__param_s2
    _strcmp__param_s2: .word 0
    _strcmp__local_0: .word 0
    _strcmp__local_1: .word 0
    proc _strcmp, W#@_p_s1, W#@_p_s2
    .sac
    .var _fp = 0
    .loc "test_strcmp.c", 4
    .var @_p_s1 = 2
    .var @_p_s2 = 4
; .debug_var: __strcmp @_p_s1 offset=2 size=2 type=ptr scope=parameter
; .debug_var: __strcmp @_p_s2 offset=4 size=2 type=ptr scope=parameter

@entry:
@while_cond0_ph:
    .loc "test_strcmp.c", 5
    lda #0
    sta $20
    lda $20
    ldx #0
    ldx #0
    sta $22
    stx $23
@while_cond0:
    lda _strcmp__param_s1
    ldx _strcmp__param_s1+1
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
    .loc "test_strcmp.c", 6
    lda _strcmp__param_s1
    ldx _strcmp__param_s1+1
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $2A
    lda _strcmp__param_s2
    ldx _strcmp__param_s2+1
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $2C
    lda $2A
    ldx #0
    ldx #0
    sta $2E
    stx $2F
    lda $2C
    ldx #0
    ldx #0
    sta $30
    stx $31
    lda $2E
    ldx $2F
    cmp.16 .AX, $30
    bne @if_then3
    bra @if_end5
@if_then3:
    .loc "test_strcmp.c", 7
    lda _strcmp__param_s1
    ldx _strcmp__param_s1+1
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $34
    lda _strcmp__param_s2
    ldx _strcmp__param_s2+1
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $36
    lda $34
    ldx #0
    ldx #0
    sta $38
    stx $39
    lda $36
    ldx #0
    ldx #0
    sta $3A
    stx $3B
    lda $38
    ldx $39
    cmp.16 .AX, $3A
    bcc @if_then6
    bra @if_end8
@if_then6:
    lda #255
    ldx #255
    bra @__return
@if_end8:
    .loc "test_strcmp.c", 8
    lda #1
    ldx #0
    bra @__return
@if_end5:
    .loc "test_strcmp.c", 10
    lda _strcmp__param_s1
    ldx _strcmp__param_s1+1
    add.16 .AX, #1
    sta $40
    stx $41
    sta _strcmp__param_s1
    stx _strcmp__param_s1+1
    .loc "test_strcmp.c", 11
    lda _strcmp__param_s2
    ldx _strcmp__param_s2+1
    add.16 .AX, #1
    sta $44
    stx $45
    sta _strcmp__param_s2
    stx _strcmp__param_s2+1
    bra @while_cond0
@while_end2:
    .loc "test_strcmp.c", 13
    lda _strcmp__param_s2
    ldx _strcmp__param_s2+1
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $20
    lda #0
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
    ldx $25
    cmp.16 .AX, $20
    bne @if_then9
    bra @if_end11
@if_then9:
    lda #255
    ldx #255
    bra @__return
@if_end11:
    .loc "test_strcmp.c", 14
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _main
; SAC inline storage: 0 bytes
    _main__local_0: .word 0
    _main__local_1: .word 0
    _main__local_5: .word 0
    _main__local_6: .word 0
    _main__local_10: .word 0
    _main__local_11: .word 0
    _main__local_15: .word 0
    _main__local_16: .word 0
    _main__local_20: .word 0
    _main__local_21: .word 0
    _main__local_25: .word 0
    _main__local_26: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_strcmp.c", 17

@entry:
    .loc "test_strcmp.c", 18
    ldax #__str_15
    sta _main__local_0
    stx _main__local_0+1
    ldax #__str_16
    sta _main__local_1
    stx _main__local_1+1
    lda _main__local_1
    ldx _main__local_1+1
    sta _strcmp__param_s2
    stx _strcmp__param_s2+1
    lda _main__local_0
    ldx _main__local_0+1
    sta _strcmp__param_s1
    stx _strcmp__param_s1+1
    jsr _strcmp
    sta $20
    stx $21
    lda $20
    ora $21
    bne @if_then12
    bra @if_end14
@if_then12:
    lda #1
    ldx #0
    bra @__return
@if_end14:
    .loc "test_strcmp.c", 19
    ldax #__str_20
    sta _main__local_5
    stx _main__local_5+1
    ldax #__str_21
    sta _main__local_6
    stx _main__local_6+1
    lda _main__local_6
    ldx _main__local_6+1
    sta _strcmp__param_s2
    stx _strcmp__param_s2+1
    lda _main__local_5
    ldx _main__local_5+1
    sta _strcmp__param_s1
    stx _strcmp__param_s1+1
    jsr _strcmp
    sta $20
    stx $21
    lda $20
    ora $21
    bne @if_then17
    bra @if_end19
@if_then17:
    lda #2
    ldx #0
    bra @__return
@if_end19:
    .loc "test_strcmp.c", 20
    ldax #__str_25
    sta _main__local_10
    stx _main__local_10+1
    ldax #__str_26
    sta _main__local_11
    stx _main__local_11+1
    lda _main__local_11
    ldx _main__local_11+1
    sta _strcmp__param_s2
    stx _strcmp__param_s2+1
    lda _main__local_10
    ldx _main__local_10+1
    sta _strcmp__param_s1
    stx _strcmp__param_s1+1
    jsr _strcmp
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #0
    bcs @if_then22
    bra @if_end24
@if_then22:
    lda #3
    ldx #0
    bra @__return
@if_end24:
    .loc "test_strcmp.c", 21
    ldax #__str_30
    sta _main__local_15
    stx _main__local_15+1
    ldax #__str_31
    sta _main__local_16
    stx _main__local_16+1
    lda _main__local_16
    ldx _main__local_16+1
    sta _strcmp__param_s2
    stx _strcmp__param_s2+1
    lda _main__local_15
    ldx _main__local_15+1
    sta _strcmp__param_s1
    stx _strcmp__param_s1+1
    jsr _strcmp
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #0
    bcc @if_then27
    beq @if_then27
    bra @if_end29
@if_then27:
    lda #4
    ldx #0
    bra @__return
@if_end29:
    .loc "test_strcmp.c", 22
    ldax #__str_35
    sta _main__local_20
    stx _main__local_20+1
    ldax #__str_36
    sta _main__local_21
    stx _main__local_21+1
    lda _main__local_21
    ldx _main__local_21+1
    sta _strcmp__param_s2
    stx _strcmp__param_s2+1
    lda _main__local_20
    ldx _main__local_20+1
    sta _strcmp__param_s1
    stx _strcmp__param_s1+1
    jsr _strcmp
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #0
    bcs @if_then32
    bra @if_end34
@if_then32:
    lda #5
    ldx #0
    bra @__return
@if_end34:
    .loc "test_strcmp.c", 23
    ldax #__str_40
    sta _main__local_25
    stx _main__local_25+1
    ldax #__str_41
    sta _main__local_26
    stx _main__local_26+1
    lda _main__local_26
    ldx _main__local_26+1
    sta _strcmp__param_s2
    stx _strcmp__param_s2+1
    lda _main__local_25
    ldx _main__local_25+1
    sta _strcmp__param_s1
    stx _strcmp__param_s1+1
    jsr _strcmp
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #0
    bcc @if_then37
    beq @if_then37
    bra @if_end39
@if_then37:
    lda #6
    ldx #0
    bra @__return
@if_end39:
    .loc "test_strcmp.c", 24
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 0
    endproc


    .segment "data"
__str_15:
    .text "abc"
    .byte 0
__str_16:
    .text "abc"
    .byte 0
__str_20:
    .text ""
    .byte 0
__str_21:
    .text ""
    .byte 0
__str_25:
    .text "abc"
    .byte 0
__str_26:
    .text "abd"
    .byte 0
__str_30:
    .text "abd"
    .byte 0
__str_31:
    .text "abc"
    .byte 0
__str_35:
    .text "ab"
    .byte 0
__str_36:
    .text "abc"
    .byte 0
__str_40:
    .text "abc"
    .byte 0
__str_41:
    .text "ab"
    .byte 0

__zp_save_buf:

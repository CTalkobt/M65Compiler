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

    .global _msg
    .global _strchr
    .global _strrchr
    .global _main

    .segment "bss"
_msg:
; .debug_var: @global _msg offset=0 size=2 type=int8 scope=global
    .res 8

    .segment "code"

; function _strchr
; SAC inline storage: 4 bytes
    .global _strchr__param_s
    _strchr__param_s: .word 0
    .global _strchr__param_c
    _strchr__param_c: .word 0
    _strchr__local_0: .word 0
    _strchr__local_1: .word 0
    proc _strchr, W#@_p_s, W#@_p_c
    .sac
    .var _fp = 0
    .loc "test_strchr.c", 4
    .var @_p_s = 2
    .var @_p_c = 4
; .debug_var: __strchr @_p_s offset=2 size=2 type=ptr scope=parameter
; .debug_var: __strchr @_p_c offset=4 size=2 type=int16 scope=parameter

@entry:
@while_cond0_ph:
    .loc "test_strchr.c", 5
    lda #0
    sta $20
    lda $20
    ldx #0
    ldx #0
    sta $22
    stx $23
    .loc "test_strchr.c", 6
    lda _strchr__param_c
    ldx _strchr__param_c+1
    sta $20
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
@while_cond0:
    .loc "test_strchr.c", 5
    lda _strchr__param_s
    ldx _strchr__param_s+1
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $20
    lda #0
    sta $26
    lda $20
    ldx #0
    ldx #0
    sta $28
    stx $29
    lda $28
    ldx $29
    cmp.16 .AX, $22
    bne @while_body1
    bra @while_end2
@while_body1:
    .loc "test_strchr.c", 6
    lda _strchr__param_s
    ldx _strchr__param_s+1
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $2C
    lda $2C
    ldx #0
    ldx #0
    sta $2E
    stx $2F
    lda $2E
    ldx $2F
    cmp.16 .AX, $24
    beq @if_then3
    bra @if_end5
@if_then3:
    lda _strchr__param_s
    ldx _strchr__param_s+1
    bra @__return
@if_end5:
    .loc "test_strchr.c", 7
    lda _strchr__param_s
    ldx _strchr__param_s+1
    add.16 .AX, #1
    sta $34
    stx $35
    sta _strchr__param_s
    stx _strchr__param_s+1
    bra @while_cond0
@while_end2:
    .loc "test_strchr.c", 9
    lda _strchr__param_c
    ldx _strchr__param_c+1
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
    beq @if_then6
    bra @if_end8
@if_then6:
    lda _strchr__param_s
    ldx _strchr__param_s+1
    bra @__return
@if_end8:
    .loc "test_strchr.c", 10
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _strrchr
; SAC inline storage: 6 bytes
    .global _strrchr__param_s
    _strrchr__param_s: .word 0
    .global _strrchr__param_c
    _strrchr__param_c: .word 0
    _strrchr__local_0: .word 0
    _strrchr__local_1: .word 0
    _strrchr__local_2: .word 0
    proc _strrchr, W#@_p_s, W#@_p_c
    .sac
    .var _fp = 0
    .loc "test_strchr.c", 13
    .local @_l_last = 4
; .debug_var: __strrchr @_l_last offset=4 size=2 type=ptr scope=local
    .var @_p_s = 2
    .var @_p_c = 4
; .debug_var: __strrchr @_p_s offset=2 size=2 type=ptr scope=parameter
; .debug_var: __strrchr @_p_c offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_strchr.c", 14
    lda #0
    sta $20
    sta $21
    sta _strrchr__local_2
    stx _strrchr__local_2+1
@while_cond9_ph:
    .loc "test_strchr.c", 15
    lda #0
    sta $20
    lda $20
    ldx #0
    ldx #0
    sta $22
    stx $23
    .loc "test_strchr.c", 16
    lda _strrchr__param_c
    ldx _strrchr__param_c+1
    sta $20
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
@while_cond9:
    .loc "test_strchr.c", 15
    lda _strrchr__param_s
    ldx _strrchr__param_s+1
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $20
    lda #0
    sta $26
    lda $20
    ldx #0
    ldx #0
    sta $28
    stx $29
    lda $28
    ldx $29
    cmp.16 .AX, $22
    bne @while_body10
    bra @while_end11
@while_body10:
    .loc "test_strchr.c", 16
    lda _strrchr__param_s
    ldx _strrchr__param_s+1
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $2C
    lda $2C
    ldx #0
    ldx #0
    sta $2E
    stx $2F
    lda $2E
    ldx $2F
    cmp.16 .AX, $24
    beq @if_then12
    bra @if_end14
@if_then12:
    lda _strrchr__param_s
    ldx _strrchr__param_s+1
    sta _strrchr__local_2
    stx _strrchr__local_2+1
@if_end14:
    .loc "test_strchr.c", 17
    lda _strrchr__param_s
    ldx _strrchr__param_s+1
    add.16 .AX, #1
    sta $34
    stx $35
    sta _strrchr__param_s
    stx _strrchr__param_s+1
    bra @while_cond9
@while_end11:
    .loc "test_strchr.c", 19
    lda _strrchr__param_c
    ldx _strrchr__param_c+1
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
    beq @if_then15
    bra @if_end17
@if_then15:
    lda _strrchr__param_s
    ldx _strrchr__param_s+1
    bra @__return
@if_end17:
    .loc "test_strchr.c", 20
    lda _strrchr__local_2
    ldx _strrchr__local_2+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _main
; SAC inline storage: 4 bytes
    _main__local_18: .word 0
    _main__local_74: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_strchr.c", 25
    .local @_l_last = 2
    .local @_l_p = 0
; .debug_var: __main @_l_last offset=2 size=2 type=ptr scope=local
; .debug_var: __main @_l_p offset=0 size=2 type=ptr scope=local

@entry:
    .loc "test_strchr.c", 26
    lda #65
    sta $20
    lda #0
    sta $22
    sta $23
    lda $20
    ldx #0
    pha
    lda $22
    ldx $23
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_msg
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
    .loc "test_strchr.c", 27
    lda #66
    sta $20
    lda #1
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    pha
    lda $22
    ldx $23
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_msg
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
    .loc "test_strchr.c", 28
    lda #67
    sta $20
    lda #2
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    pha
    lda $22
    ldx $23
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_msg
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
    .loc "test_strchr.c", 29
    lda #66
    sta $20
    lda #3
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    pha
    lda $22
    ldx $23
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_msg
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
    .loc "test_strchr.c", 30
    lda #68
    sta $20
    lda #4
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    pha
    lda $22
    ldx $23
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_msg
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
    .loc "test_strchr.c", 31
    lda #0
    sta $20
    lda #5
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    pha
    lda $22
    ldx $23
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_msg
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
    .loc "test_strchr.c", 34
    lda #66
    ldx #0
    sta $20
    stx $21
    ldax #_msg
    sta $22
    stx $23
    lda $20
    ldx $21
    sta $24
    stx $25
@while_cond19_ph:
    .loc "test_strchr.c", 5
    lda #0
    sta $20
    .loc "test_strchr.c", 7
    lda $20
    ldx #0
    ldx #0
    sta $28
    stx $29
    .loc "test_strchr.c", 6
    lda $24
    ldx $25
    sta $20
    lda $20
    ldx #0
    ldx #0
    sta $2A
    stx $2B
    .loc "test_strchr.c", 7
    lda $22
    ldx $23
    add.16 .AX, #1
    sta $20
    stx $21
@while_cond19:
    .loc "test_strchr.c", 5
    ldy #0
    lda ($22),y
    ldx #0
    sta $26
    lda #0
    sta $2C
    lda $26
    ldx #0
    ldx #0
    sta $2E
    stx $2F
    lda $2E
    ldx $2F
    cmp.16 .AX, $28
    bne @while_body20
    bra @while_end21
@while_body20:
    .loc "test_strchr.c", 6
    ldy #0
    lda ($22),y
    ldx #0
    sta $32
    lda $32
    ldx #0
    ldx #0
    sta $34
    stx $35
    lda $34
    ldx $35
    cmp.16 .AX, $2A
    beq @if_then22
    bra @if_end24
@if_then22:
    lda $22
    ldx $23
    sta $38
    stx $39
    bra @inline_end18
@if_end24:
    .loc "test_strchr.c", 7
    lda #1
    ldx #0
    sta $3A
    stx $3B
    lda $20
    ldx $21
    ldy #0
    sta ($22),y
    txa
    iny
    sta ($22),y
    bra @while_cond19
@while_end21:
    .loc "test_strchr.c", 9
    lda $24
    ldx $25
    sta $20
    lda #0
    sta $24
    lda $20
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $24
    ldx #0
    ldx #0
    sta $20
    stx $21
    lda $26
    ldx $27
    cmp.16 .AX, $20
    beq @if_then26
    bra @if_end28
@if_then26:
    lda $22
    ldx $23
    sta $38
    stx $39
    bra @inline_end18
@if_end28:
    .loc "test_strchr.c", 10
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $38
    stx $39
@inline_end18:
    .loc "test_strchr.c", 34
    lda $38
    ldx $39
    sta _main__local_18
    stx _main__local_18+1
    .loc "test_strchr.c", 35
    lda _main__local_18
    ldx _main__local_18+1
    stx __zp_scratch
    ora __zp_scratch
    beq @if_then31
    bra @if_end33
@if_then31:
    lda #1
    ldx #0
    bra @__return
@if_end33:
    .loc "test_strchr.c", 36
    ldax #_msg
    add.16 .AX, #1
    sta $22
    stx $23
    lda _main__local_18
    ldx _main__local_18+1
    cmp.16 .AX, $22
    bne @if_then34
    bra @if_end36
@if_then34:
    lda #2
    ldx #0
    bra @__return
@if_end36:
    .loc "test_strchr.c", 39
    lda #90
    ldx #0
    sta $20
    stx $21
    ldax #_msg
    sta $22
    stx $23
    lda $20
    ldx $21
    sta $24
    stx $25
@while_cond41_ph:
    .loc "test_strchr.c", 5
    lda #0
    sta $20
    .loc "test_strchr.c", 7
    lda $20
    ldx #0
    ldx #0
    sta $28
    stx $29
    .loc "test_strchr.c", 6
    lda $24
    ldx $25
    sta $20
    lda $20
    ldx #0
    ldx #0
    sta $2A
    stx $2B
    .loc "test_strchr.c", 7
    lda $22
    ldx $23
    add.16 .AX, #1
    sta $20
    stx $21
@while_cond41:
    .loc "test_strchr.c", 5
    ldy #0
    lda ($22),y
    ldx #0
    sta $26
    lda #0
    sta $2C
    lda $26
    ldx #0
    ldx #0
    sta $2E
    stx $2F
    lda $2E
    ldx $2F
    cmp.16 .AX, $28
    bne @while_body42
    bra @while_end43
@while_body42:
    .loc "test_strchr.c", 6
    ldy #0
    lda ($22),y
    ldx #0
    sta $32
    lda $32
    ldx #0
    ldx #0
    sta $34
    stx $35
    lda $34
    ldx $35
    cmp.16 .AX, $2A
    beq @if_then44
    bra @if_end46
@if_then44:
    lda $22
    ldx $23
    sta $38
    stx $39
    bra @inline_end40
@if_end46:
    .loc "test_strchr.c", 7
    lda #1
    ldx #0
    sta $3A
    stx $3B
    lda $20
    ldx $21
    ldy #0
    sta ($22),y
    txa
    iny
    sta ($22),y
    bra @while_cond41
@while_end43:
    .loc "test_strchr.c", 9
    lda $24
    ldx $25
    sta $20
    lda #0
    sta $24
    lda $20
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $24
    ldx #0
    ldx #0
    sta $20
    stx $21
    lda $26
    ldx $27
    cmp.16 .AX, $20
    beq @if_then48
    bra @if_end50
@if_then48:
    lda $22
    ldx $23
    sta $38
    stx $39
    bra @inline_end40
@if_end50:
    .loc "test_strchr.c", 10
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $38
    stx $39
@inline_end40:
    .loc "test_strchr.c", 39
    lda $38
    ora $39
    bne @if_then37
    bra @if_end39
@if_then37:
    lda #3
    ldx #0
    bra @__return
@if_end39:
    .loc "test_strchr.c", 42
    lda #66
    ldx #0
    sta $20
    stx $21
    ldax #_msg
    sta $22
    stx $23
    lda $20
    ldx $21
    sta $24
    stx $25
    .loc "test_strchr.c", 14
    lda #0
    sta $20
    sta $21
    sta _main__local_74
    stx _main__local_74+1
@while_cond54_ph:
    .loc "test_strchr.c", 15
    lda #0
    sta $20
    .loc "test_strchr.c", 17
    lda $20
    ldx #0
    ldx #0
    sta $28
    stx $29
    .loc "test_strchr.c", 16
    lda $24
    ldx $25
    sta $20
    lda $20
    ldx #0
    ldx #0
    sta $2A
    stx $2B
    .loc "test_strchr.c", 17
    lda $22
    ldx $23
    add.16 .AX, #1
    sta $20
    stx $21
@while_cond54:
    .loc "test_strchr.c", 15
    ldy #0
    lda ($22),y
    ldx #0
    sta $26
    lda #0
    sta $2C
    lda $26
    ldx #0
    ldx #0
    sta $2E
    stx $2F
    lda $2E
    ldx $2F
    cmp.16 .AX, $28
    bne @while_body55
    bra @while_end56
@while_body55:
    .loc "test_strchr.c", 16
    ldy #0
    lda ($22),y
    ldx #0
    sta $32
    lda $32
    ldx #0
    ldx #0
    sta $34
    stx $35
    lda $34
    ldx $35
    cmp.16 .AX, $2A
    beq @if_then57
    bra @if_end59
@if_then57:
    lda $22
    ldx $23
    sta _main__local_74
    stx _main__local_74+1
@if_end59:
    .loc "test_strchr.c", 17
    lda #1
    ldx #0
    sta $38
    stx $39
    lda $20
    ldx $21
    ldy #0
    sta ($22),y
    txa
    iny
    sta ($22),y
    bra @while_cond54
@while_end56:
    .loc "test_strchr.c", 19
    lda $24
    ldx $25
    sta $20
    lda #0
    sta $24
    lda $20
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $24
    ldx #0
    ldx #0
    sta $20
    stx $21
    lda $26
    ldx $27
    cmp.16 .AX, $20
    beq @if_then60
    bra @if_end62
@if_then60:
    lda $22
    ldx $23
    sta $20
    stx $21
    bra @inline_end53
@if_end62:
    .loc "test_strchr.c", 20
    lda _main__local_74
    ldx _main__local_74+1
    sta $20
    stx $21
@inline_end53:
    .loc "test_strchr.c", 42
    lda $20
    ldx $21
    sta _main__local_18
    stx _main__local_18+1
    .loc "test_strchr.c", 43
    lda _main__local_18
    ldx _main__local_18+1
    stx __zp_scratch
    ora __zp_scratch
    beq @if_then65
    bra @if_end67
@if_then65:
    lda #4
    ldx #0
    bra @__return
@if_end67:
    .loc "test_strchr.c", 44
    ldax #_msg
    add.16 .AX, #3
    sta $22
    stx $23
    lda _main__local_18
    ldx _main__local_18+1
    cmp.16 .AX, $22
    bne @if_then68
    bra @if_end70
@if_then68:
    lda #5
    ldx #0
    bra @__return
@if_end70:
    .loc "test_strchr.c", 46
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc


__zp_save_buf:

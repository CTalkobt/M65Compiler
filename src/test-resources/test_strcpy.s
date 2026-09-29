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

    .global _buf
    .global _strcpy
    .global _strlen
    .global _strcmp
    .global _main

    .segment "bss"
_buf:
; .debug_var: @global _buf offset=0 size=2 type=int8 scope=global
    .res 16

    .segment "code"

; function _strcpy
; SAC inline storage: 6 bytes
    .global _strcpy__param_dest
    _strcpy__param_dest: .word 0
    .global _strcpy__param_src
    _strcpy__param_src: .word 0
    _strcpy__local_0: .word 0
    _strcpy__local_1: .word 0
    _strcpy__local_2: .word 0
    proc _strcpy, W#@_p_dest, W#@_p_src
    .sac
    .var _fp = 0
    .loc "test_strcpy.c", 4
    .local @_l_ret = 4
; .debug_var: __strcpy @_l_ret offset=4 size=2 type=ptr scope=local
    .var @_p_dest = 2
    .var @_p_src = 4
; .debug_var: __strcpy @_p_dest offset=2 size=2 type=ptr scope=parameter
; .debug_var: __strcpy @_p_src offset=4 size=2 type=ptr scope=parameter

@entry:
    .loc "test_strcpy.c", 5
    lda _strcpy__param_dest
    ldx _strcpy__param_dest+1
    sta _strcpy__local_2
    stx _strcpy__local_2+1
@while_cond0_ph:
    .loc "test_strcpy.c", 6
    lda #0
    sta $20
    lda $20
    ldx #0
    ldx #0
    sta $22
    stx $23
@while_cond0:
    lda _strcpy__param_src
    ldx _strcpy__param_src+1
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
    .loc "test_strcpy.c", 7
    lda _strcpy__param_src
    ldx _strcpy__param_src+1
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $2A
    lda _strcpy__param_dest
    ldx _strcpy__param_dest+1
    sta $2C
    stx $2D
    lda $2A
    ldy #0
    sta ($2C),y
    .loc "test_strcpy.c", 8
    lda _strcpy__param_dest
    ldx _strcpy__param_dest+1
    add.16 .AX, #1
    sta $30
    stx $31
    sta _strcpy__param_dest
    stx _strcpy__param_dest+1
    .loc "test_strcpy.c", 9
    lda _strcpy__param_src
    ldx _strcpy__param_src+1
    add.16 .AX, #1
    sta $34
    stx $35
    sta _strcpy__param_src
    stx _strcpy__param_src+1
    bra @while_cond0
@while_end2:
    .loc "test_strcpy.c", 11
    lda #0
    sta $20
    lda _strcpy__param_dest
    ldx _strcpy__param_dest+1
    sta $22
    stx $23
    lda $20
    ldy #0
    sta ($22),y
    .loc "test_strcpy.c", 12
    lda _strcpy__local_2
    ldx _strcpy__local_2+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _strlen
; SAC inline storage: 4 bytes
    .global _strlen__param_s
    _strlen__param_s: .word 0
    _strlen__local_0: .word 0
    _strlen__local_1: .word 0
    proc _strlen, W#@_p_s
    .sac
    .var _fp = 0
    .loc "test_strcpy.c", 15
    .local @_l_len = 2
; .debug_var: __strlen @_l_len offset=2 size=2 type=int16 scope=local
    .var @_p_s = 2
; .debug_var: __strlen @_p_s offset=2 size=2 type=ptr scope=parameter

@entry:
    .loc "test_strcpy.c", 16
    lda #0
    sta _strlen__local_1
    sta _strlen__local_1+1
@while_cond3_ph:
    .loc "test_strcpy.c", 17
    lda #0
    sta $20
    lda $20
    ldx #0
    ldx #0
    sta $22
    stx $23
@while_cond3:
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
    bne @while_body4
    bra @while_end5
@while_body4:
    inc.16f __vr1
    lda _strlen__param_s
    ldx _strlen__param_s+1
    add.16 .AX, #1
    sta $30
    stx $31
    sta _strlen__param_s
    stx _strlen__param_s+1
    bra @while_cond3
@while_end5:
    .loc "test_strcpy.c", 18
    lda _strlen__local_1
    ldx _strlen__local_1+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

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
    .loc "test_strcpy.c", 21
    .var @_p_s1 = 2
    .var @_p_s2 = 4
; .debug_var: __strcmp @_p_s1 offset=2 size=2 type=ptr scope=parameter
; .debug_var: __strcmp @_p_s2 offset=4 size=2 type=ptr scope=parameter

@entry:
@while_cond6_ph:
    .loc "test_strcpy.c", 22
    lda #0
    sta $20
    lda $20
    ldx #0
    ldx #0
    sta $22
    stx $23
@while_cond6:
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
    bne @while_body7
    bra @while_end8
@while_body7:
    .loc "test_strcpy.c", 23
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
    bne @if_then9
    bra @if_end11
@if_then9:
    lda #1
    ldx #0
    bra @__return
@if_end11:
    .loc "test_strcpy.c", 24
    lda _strcmp__param_s1
    ldx _strcmp__param_s1+1
    add.16 .AX, #1
    sta $36
    stx $37
    sta _strcmp__param_s1
    stx _strcmp__param_s1+1
    .loc "test_strcpy.c", 25
    lda _strcmp__param_s2
    ldx _strcmp__param_s2+1
    add.16 .AX, #1
    sta $3A
    stx $3B
    sta _strcmp__param_s2
    stx _strcmp__param_s2+1
    bra @while_cond6
@while_end8:
    .loc "test_strcpy.c", 27
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
    bne @if_then12
    bra @if_end14
@if_then12:
    lda #1
    ldx #0
    bra @__return
@if_end14:
    .loc "test_strcpy.c", 28
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
; SAC inline storage: 8 bytes
    _main__local_63: .word 0
    _main__local_77: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_strcpy.c", 33
    .local @_l_len = 6
    .local @_l_ret = 4
; .debug_var: __main @_l_len offset=6 size=2 type=int16 scope=local
; .debug_var: __main @_l_ret offset=4 size=2 type=ptr scope=local

@entry:
    .loc "test_strcpy.c", 34
    ldax #__str_15
    sta $20
    stx $21
    ldax #_buf
    sta $22
    stx $23
    lda $20
    ldx $21
    sta $24
    stx $25
    .loc "test_strcpy.c", 5
@while_cond17_ph:
    .loc "test_strcpy.c", 6
    lda #0
    sta $26
    .loc "test_strcpy.c", 8
    .loc "test_strcpy.c", 9
    lda $26
    ldx #0
    ldx #0
    sta $2C
    stx $2D
    .loc "test_strcpy.c", 8
    lda $22
    ldx $23
    add.16 .AX, #1
    sta $26
    stx $27
    .loc "test_strcpy.c", 9
    lda $24
    ldx $25
    add.16 .AX, #1
    sta $28
    stx $29
@while_cond17:
    .loc "test_strcpy.c", 6
    ldy #0
    lda ($24),y
    ldx #0
    sta $2A
    lda #0
    sta $2E
    lda $2A
    ldx #0
    ldx #0
    sta $30
    stx $31
    lda $30
    ldx $31
    cmp.16 .AX, $2C
    bne @while_body18
    bra @while_end19
@while_body18:
    .loc "test_strcpy.c", 7
    ldy #0
    lda ($24),y
    ldx #0
    sta $34
    lda $34
    ldy #0
    sta ($22),y
    .loc "test_strcpy.c", 8
    lda #1
    ldx #0
    sta $36
    stx $37
    lda $26
    ldx $27
    ldy #0
    sta ($22),y
    txa
    iny
    sta ($22),y
    .loc "test_strcpy.c", 9
    lda #1
    ldx #0
    sta $38
    stx $39
    lda $28
    ldx $29
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    bra @while_cond17
@while_end19:
    .loc "test_strcpy.c", 11
    lda #0
    sta $24
    lda $24
    ldy #0
    sta ($22),y
    .loc "test_strcpy.c", 12
@inline_end16:
    .loc "test_strcpy.c", 35
    ldax #__str_24
    sta $20
    stx $21
    ldax #_buf
    sta $22
    stx $23
    lda $20
    ldx $21
    sta $24
    stx $25
@while_cond26_ph:
    .loc "test_strcpy.c", 22
    lda #0
    sta $20
    .loc "test_strcpy.c", 24
    .loc "test_strcpy.c", 25
    lda $20
    ldx #0
    ldx #0
    sta $2A
    stx $2B
    .loc "test_strcpy.c", 24
    lda $22
    ldx $23
    add.16 .AX, #1
    sta $20
    stx $21
    .loc "test_strcpy.c", 25
    lda $24
    ldx $25
    add.16 .AX, #1
    sta $26
    stx $27
@while_cond26:
    .loc "test_strcpy.c", 22
    ldy #0
    lda ($22),y
    ldx #0
    sta $28
    lda #0
    sta $2C
    lda $28
    ldx #0
    ldx #0
    sta $2E
    stx $2F
    lda $2E
    ldx $2F
    cmp.16 .AX, $2A
    bne @while_body27
    bra @while_end28
@while_body27:
    .loc "test_strcpy.c", 23
    ldy #0
    lda ($22),y
    ldx #0
    sta $32
    ldy #0
    lda ($24),y
    ldx #0
    sta $34
    lda $32
    ldx #0
    ldx #0
    sta $36
    stx $37
    lda $34
    ldx #0
    ldx #0
    sta $38
    stx $39
    lda $36
    ldx $37
    cmp.16 .AX, $38
    bne @if_then29
    bra @if_end31
@if_then29:
    lda #1
    ldx #0
    sta $3C
    stx $3D
    lda $3C
    ldx $3D
    sta $3E
    stx $3F
    bra @inline_end25
@if_end31:
    .loc "test_strcpy.c", 24
    lda #1
    ldx #0
    sta $40
    stx $41
    lda $20
    ldx $21
    ldy #0
    sta ($22),y
    txa
    iny
    sta ($22),y
    .loc "test_strcpy.c", 25
    lda #1
    ldx #0
    sta $42
    stx $43
    lda $26
    ldx $27
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    bra @while_cond26
@while_end28:
    .loc "test_strcpy.c", 27
    ldy #0
    lda ($24),y
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
    bne @if_then33
    bra @if_end35
@if_then33:
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $3E
    stx $3F
    bra @inline_end25
@if_end35:
    .loc "test_strcpy.c", 28
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $3E
    stx $3F
@inline_end25:
    .loc "test_strcpy.c", 35
    lda $3E
    ora $3F
    bne @if_then21
    bra @if_end23
@if_then21:
    lda #1
    ldx #0
    bra @__return
@if_end23:
    .loc "test_strcpy.c", 36
    ldax #_buf
    sta $20
    stx $21
    .loc "test_strcpy.c", 16
    lda #0
    sta $24
    sta $25
@while_cond42_ph:
    .loc "test_strcpy.c", 17
    lda #0
    sta $22
    lda $22
    ldx #0
    ldx #0
    sta $28
    stx $29
    lda $20
    ldx $21
    add.16 .AX, #1
    sta $22
    stx $23
@while_cond42:
    ldy #0
    lda ($20),y
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
    bne @while_body43
    bra @while_end44
@while_body43:
    inc $24
    bne *+4
    inc $25
    lda #1
    ldx #0
    sta $34
    stx $35
    lda $22
    ldx $23
    ldy #0
    sta ($20),y
    txa
    iny
    sta ($20),y
    bra @while_cond42
@while_end44:
    .loc "test_strcpy.c", 18
    lda $24
    ldx $25
    sta $20
    stx $21
@inline_end41:
    .loc "test_strcpy.c", 36
    lda $20
    ldx $21
    cmp.16 .AX, #5
    bne @if_then38
    bra @if_end40
@if_then38:
    lda #2
    ldx #0
    bra @__return
@if_end40:
    .loc "test_strcpy.c", 37
    ldax #__str_46
    sta $20
    stx $21
    ldax #_buf
    sta $22
    stx $23
    lda $20
    ldx $21
    sta $24
    stx $25
    .loc "test_strcpy.c", 5
@while_cond48_ph:
    .loc "test_strcpy.c", 6
    lda #0
    sta $20
    .loc "test_strcpy.c", 8
    .loc "test_strcpy.c", 9
    lda $20
    ldx #0
    ldx #0
    sta $2A
    stx $2B
    .loc "test_strcpy.c", 8
    lda $22
    ldx $23
    add.16 .AX, #1
    sta $20
    stx $21
    .loc "test_strcpy.c", 9
    lda $24
    ldx $25
    add.16 .AX, #1
    sta $26
    stx $27
@while_cond48:
    .loc "test_strcpy.c", 6
    ldy #0
    lda ($24),y
    ldx #0
    sta $28
    lda #0
    sta $2C
    lda $28
    ldx #0
    ldx #0
    sta $2E
    stx $2F
    lda $2E
    ldx $2F
    cmp.16 .AX, $2A
    bne @while_body49
    bra @while_end50
@while_body49:
    .loc "test_strcpy.c", 7
    ldy #0
    lda ($24),y
    ldx #0
    sta $32
    lda $32
    ldy #0
    sta ($22),y
    .loc "test_strcpy.c", 8
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
    .loc "test_strcpy.c", 9
    lda #1
    ldx #0
    sta $36
    stx $37
    lda $26
    ldx $27
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    bra @while_cond48
@while_end50:
    .loc "test_strcpy.c", 11
    lda #0
    sta $20
    lda $20
    ldy #0
    sta ($22),y
    .loc "test_strcpy.c", 12
@inline_end47:
    .loc "test_strcpy.c", 38
    ldax #_buf
    sta $20
    stx $21
    .loc "test_strcpy.c", 16
    lda #0
    sta _main__local_77
    sta _main__local_77+1
@while_cond56_ph:
    .loc "test_strcpy.c", 17
    lda #0
    sta $22
    lda $22
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $20
    ldx $21
    add.16 .AX, #1
    sta $22
    stx $23
@while_cond56:
    ldy #0
    lda ($20),y
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
    bne @while_body57
    bra @while_end58
@while_body57:
    inc.16f __vr77
    lda #1
    ldx #0
    sta $32
    stx $33
    lda $22
    ldx $23
    ldy #0
    sta ($20),y
    txa
    iny
    sta ($20),y
    bra @while_cond56
@while_end58:
    .loc "test_strcpy.c", 18
    lda _main__local_77
    ldx _main__local_77+1
    sta $20
    stx $21
@inline_end55:
    .loc "test_strcpy.c", 38
    lda $20
    ora $21
    bne @if_then52
    bra @if_end54
@if_then52:
    lda #3
    ldx #0
    bra @__return
@if_end54:
    .loc "test_strcpy.c", 39
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
__str_15:
    .text "hello"
    .byte 0
__str_24:
    .text "hello"
    .byte 0
__str_46:
    .text ""
    .byte 0

__zp_save_buf:

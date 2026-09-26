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

    .global _src
    .global _dst
    .global _memset
    .global _memcpy
    .global _memcmp
    .global _main

    .segment "bss"
_src:
; .debug_var: @global _src offset=0 size=2 type=int8 scope=global
    .res 8
_dst:
; .debug_var: @global _dst offset=0 size=2 type=int8 scope=global
    .res 8

    .segment "code"

; function _memset
; SAC inline storage: 8 bytes
    .global _memset__param_s
    _memset__param_s: .word 0
    .global _memset__param_c
    _memset__param_c: .word 0
    .global _memset__param_n
    _memset__param_n: .word 0
    _memset__local_0: .word 0
    _memset__local_1: .word 0
    _memset__local_2: .word 0
    _memset__local_3: .word 0
    proc _memset, W#@_p_s, W#@_p_c, W#@_p_n
    .sac
    .var _fp = 0
    .loc "test_memcpy.c", 4
    .local @_l_p = 6
; .debug_var: __memset @_l_p offset=6 size=2 type=ptr scope=local
    .var @_p_s = 2
    .var @_p_c = 4
    .var @_p_n = 6
; .debug_var: __memset @_p_s offset=2 size=2 type=ptr scope=parameter
; .debug_var: __memset @_p_c offset=4 size=2 type=int16 scope=parameter
; .debug_var: __memset @_p_n offset=6 size=2 type=int16 scope=parameter

@entry:
    .loc "test_memcpy.c", 5
    lda _memset__param_s
    ldx _memset__param_s+1
    sta _memset__local_3
    stx _memset__local_3+1
@while_cond0_ph:
    .loc "test_memcpy.c", 7
    lda _memset__param_c
    ldx _memset__param_c+1
    sta $20
@while_cond0:
    .loc "test_memcpy.c", 6
    lda _memset__param_n
    ldx _memset__param_n+1
    cmp.16 .AX, #0
    beq @while_end2
    bcs @while_body1
    bra @while_end2
@while_body1:
    .loc "test_memcpy.c", 7
    lda _memset__local_3
    ldx _memset__local_3+1
    sta $26
    stx $27
    lda $20
    ldy #0
    sta ($26),y
    .loc "test_memcpy.c", 8
    lda _memset__local_3
    ldx _memset__local_3+1
    add.16 .AX, #1
    sta $2A
    stx $2B
    sta _memset__local_3
    stx _memset__local_3+1
    .loc "test_memcpy.c", 9
    dec.16f __vr2
    bra @while_cond0
@while_end2:
    .loc "test_memcpy.c", 11
    lda _memset__param_s
    ldx _memset__param_s+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 8
    endproc

; function _memcpy
; SAC inline storage: 10 bytes
    .global _memcpy__param_dest
    _memcpy__param_dest: .word 0
    .global _memcpy__param_src
    _memcpy__param_src: .word 0
    .global _memcpy__param_n
    _memcpy__param_n: .word 0
    _memcpy__local_0: .word 0
    _memcpy__local_1: .word 0
    _memcpy__local_2: .word 0
    _memcpy__local_3: .word 0
    _memcpy__local_4: .word 0
    proc _memcpy, W#@_p_dest, W#@_p_src, W#@_p_n
    .sac
    .var _fp = 0
    .loc "test_memcpy.c", 14
    .local @_l_d = 6
    .local @_l_s = 8
; .debug_var: __memcpy @_l_d offset=6 size=2 type=ptr scope=local
; .debug_var: __memcpy @_l_s offset=8 size=2 type=ptr scope=local
    .var @_p_dest = 2
    .var @_p_src = 4
    .var @_p_n = 6
; .debug_var: __memcpy @_p_dest offset=2 size=2 type=ptr scope=parameter
; .debug_var: __memcpy @_p_src offset=4 size=2 type=ptr scope=parameter
; .debug_var: __memcpy @_p_n offset=6 size=2 type=int16 scope=parameter

@entry:
    .loc "test_memcpy.c", 15
    lda _memcpy__param_dest
    ldx _memcpy__param_dest+1
    sta _memcpy__local_3
    stx _memcpy__local_3+1
    .loc "test_memcpy.c", 16
    lda _memcpy__param_src
    ldx _memcpy__param_src+1
    sta _memcpy__local_4
    stx _memcpy__local_4+1
@while_cond3:
    .loc "test_memcpy.c", 17
    lda _memcpy__param_n
    ldx _memcpy__param_n+1
    cmp.16 .AX, #0
    beq @while_end5
    bcs @while_body4
    bra @while_end5
@while_body4:
    .loc "test_memcpy.c", 18
    lda _memcpy__local_4
    ldx _memcpy__local_4+1
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $24
    lda _memcpy__local_3
    ldx _memcpy__local_3+1
    sta $26
    stx $27
    lda $24
    ldy #0
    sta ($26),y
    .loc "test_memcpy.c", 19
    lda _memcpy__local_3
    ldx _memcpy__local_3+1
    add.16 .AX, #1
    sta $2A
    stx $2B
    sta _memcpy__local_3
    stx _memcpy__local_3+1
    .loc "test_memcpy.c", 20
    lda _memcpy__local_4
    ldx _memcpy__local_4+1
    add.16 .AX, #1
    sta $2E
    stx $2F
    sta _memcpy__local_4
    stx _memcpy__local_4+1
    .loc "test_memcpy.c", 21
    dec.16f __vr2
    bra @while_cond3
@while_end5:
    .loc "test_memcpy.c", 23
    lda _memcpy__param_dest
    ldx _memcpy__param_dest+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 10
    endproc

; function _memcmp
; SAC inline storage: 6 bytes
    .global _memcmp__param_s1
    _memcmp__param_s1: .word 0
    .global _memcmp__param_s2
    _memcmp__param_s2: .word 0
    .global _memcmp__param_n
    _memcmp__param_n: .word 0
    _memcmp__local_0: .word 0
    _memcmp__local_1: .word 0
    _memcmp__local_2: .word 0
    proc _memcmp, W#@_p_s1, W#@_p_s2, W#@_p_n
    .sac
    .var _fp = 0
    .loc "test_memcpy.c", 26
    .var @_p_s1 = 2
    .var @_p_s2 = 4
    .var @_p_n = 6
; .debug_var: __memcmp @_p_s1 offset=2 size=2 type=ptr scope=parameter
; .debug_var: __memcmp @_p_s2 offset=4 size=2 type=ptr scope=parameter
; .debug_var: __memcmp @_p_n offset=6 size=2 type=int16 scope=parameter

@entry:
@while_cond6:
    .loc "test_memcpy.c", 27
    lda _memcmp__param_n
    ldx _memcmp__param_n+1
    cmp.16 .AX, #0
    beq @while_end8
    bcs @while_body7
    bra @while_end8
@while_body7:
    .loc "test_memcpy.c", 28
    lda _memcmp__param_s1
    ldx _memcmp__param_s1+1
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $24
    lda _memcmp__param_s2
    ldx _memcmp__param_s2+1
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $26
    lda $24
    ldx #0
    ldx #0
    sta $28
    stx $29
    lda $26
    ldx #0
    ldx #0
    sta $2A
    stx $2B
    lda $28
    ldx $29
    cmp.16 .AX, $2A
    bne @if_then9
    bra @if_end11
@if_then9:
    .loc "test_memcpy.c", 29
    lda _memcmp__param_s1
    ldx _memcmp__param_s1+1
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $2E
    lda _memcmp__param_s2
    ldx _memcmp__param_s2+1
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $30
    lda $2E
    ldx #0
    ldx #0
    sta $32
    stx $33
    lda $30
    ldx #0
    ldx #0
    sta $34
    stx $35
    lda $32
    ldx $33
    cmp.16 .AX, $34
    bcc @if_then12
    bra @if_end14
@if_then12:
    lda #255
    ldx #255
    bra @__return
@if_end14:
    .loc "test_memcpy.c", 30
    lda #1
    ldx #0
    bra @__return
@if_end11:
    .loc "test_memcpy.c", 32
    lda _memcmp__param_s1
    ldx _memcmp__param_s1+1
    add.16 .AX, #1
    sta $3A
    stx $3B
    sta _memcmp__param_s1
    stx _memcmp__param_s1+1
    .loc "test_memcpy.c", 33
    lda _memcmp__param_s2
    ldx _memcmp__param_s2+1
    add.16 .AX, #1
    sta $3E
    stx $3F
    sta _memcmp__param_s2
    stx _memcmp__param_s2+1
    .loc "test_memcpy.c", 34
    dec.16f __vr2
    bra @while_cond6
@while_end8:
    .loc "test_memcpy.c", 36
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _main
; SAC inline storage: 8 bytes
    _main__local_31: .word 0
    _main__local_45: .word 0
    _main__local_46: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_memcpy.c", 42
    .local @_l_d = 4
    .local @_l_p = 2
    .local @_l_s = 6
; .debug_var: __main @_l_d offset=4 size=2 type=ptr scope=local
; .debug_var: __main @_l_p offset=2 size=2 type=ptr scope=local
; .debug_var: __main @_l_s offset=6 size=2 type=ptr scope=local

@entry:
    .loc "test_memcpy.c", 44
    lda #66
    ldx #0
    sta $20
    stx $21
    lda #4
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    lda $20
    ldx $21
    sta $26
    stx $27
    lda $22
    ldx $23
    sta $20
    stx $21
    .loc "test_memcpy.c", 5
    lda $22
    ldx $23
    sta $28
    stx $29
@while_cond16_ph:
    .loc "test_memcpy.c", 6
    .loc "test_memcpy.c", 9
    .loc "test_memcpy.c", 6
    lda $20
    ldx $21
    cmp.16 .AX, #0
    beq @__cmp_zero_0
    bcs @__cmp_set_0
    bra @__cmp_zero_0
@__cmp_set_0:
    lda #1
    ldx #0
    bra @__cmp_done_0
@__cmp_zero_0:
    lda #0
    ldx #0
@__cmp_done_0:
    sta $2E
    .loc "test_memcpy.c", 7
    lda $26
    ldx $27
    sta $2A
    .loc "test_memcpy.c", 9
    lda $20
    sec
    sbc #1
    sta $26
    lda $21
    sbc #0
    sta $27
@while_cond16:
    .loc "test_memcpy.c", 6
    lda #0
    sta $2C
    sta $2D
    lda $2E
    bne @while_body17
    bra @while_end18
@while_body17:
    .loc "test_memcpy.c", 7
    lda $28
    ldx $29
    sta $30
    stx $31
    lda $2A
    ldy #0
    sta ($30),y
    .loc "test_memcpy.c", 8
    lda $28
    ldx $29
    add.16 .AX, #1
    sta $34
    stx $35
    sta $28
    stx $29
    .loc "test_memcpy.c", 9
    lda #1
    ldx #0
    sta $36
    stx $37
    lda $26
    ldx $27
    ldy #0
    sta ($20),y
    txa
    iny
    sta ($20),y
    bra @while_cond16
@while_end18:
    .loc "test_memcpy.c", 11
@inline_end15:
    .loc "test_memcpy.c", 45
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
    cmp.16 .AX, #66
    bne @if_then20
    bra @if_end22
@if_then20:
    lda #1
    ldx #0
    bra @__return
@if_end22:
    .loc "test_memcpy.c", 46
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
    cmp.16 .AX, #66
    bne @if_then23
    bra @if_end25
@if_then23:
    lda #2
    ldx #0
    bra @__return
@if_end25:
    .loc "test_memcpy.c", 49
    lda #0
    sta $20
    sta $21
    lda #8
    ldx #0
    sta $24
    stx $25
    ldax #_dst
    sta $26
    stx $27
    lda $20
    ldx $21
    sta $28
    stx $29
    lda $24
    ldx $25
    sta $20
    stx $21
    .loc "test_memcpy.c", 5
    ldax #_dst
    sta _main__local_31
    stx _main__local_31+1
@while_cond27_ph:
    .loc "test_memcpy.c", 6
    .loc "test_memcpy.c", 9
    .loc "test_memcpy.c", 6
    lda $20
    ldx $21
    cmp.16 .AX, #0
    beq @__cmp_zero_1
    bcs @__cmp_set_1
    bra @__cmp_zero_1
@__cmp_set_1:
    lda #1
    ldx #0
    bra @__cmp_done_1
@__cmp_zero_1:
    lda #0
    ldx #0
@__cmp_done_1:
    sta $2C
    .loc "test_memcpy.c", 7
    lda $28
    ldx $29
    sta $24
    .loc "test_memcpy.c", 9
    lda $20
    sec
    sbc #1
    sta $28
    lda $21
    sbc #0
    sta $29
@while_cond27:
    .loc "test_memcpy.c", 6
    lda #0
    sta $2A
    sta $2B
    lda $2C
    bne @while_body28
    bra @while_end29
@while_body28:
    .loc "test_memcpy.c", 7
    lda _main__local_31
    ldx _main__local_31+1
    sta $2E
    stx $2F
    lda $24
    ldy #0
    sta ($2E),y
    .loc "test_memcpy.c", 8
    lda _main__local_31
    ldx _main__local_31+1
    add.16 .AX, #1
    sta $32
    stx $33
    sta _main__local_31
    stx _main__local_31+1
    .loc "test_memcpy.c", 9
    lda #1
    ldx #0
    sta $34
    stx $35
    lda $28
    ldx $29
    ldy #0
    sta ($20),y
    txa
    iny
    sta ($20),y
    bra @while_cond27
@while_end29:
    .loc "test_memcpy.c", 11
@inline_end26:
    .loc "test_memcpy.c", 50
    lda #4
    ldx #0
    sta $20
    stx $21
    ldax #_dst
    sta $24
    stx $25
    lda $22
    ldx $23
    sta $26
    stx $27
    lda $20
    ldx $21
    sta $26
    stx $27
    .loc "test_memcpy.c", 15
    ldax #_dst
    sta _main__local_45
    stx _main__local_45+1
    .loc "test_memcpy.c", 16
    lda $22
    ldx $23
    sta _main__local_46
    stx _main__local_46+1
@while_cond32_ph:
    .loc "test_memcpy.c", 17
    .loc "test_memcpy.c", 21
    .loc "test_memcpy.c", 17
    lda $26
    ldx $27
    cmp.16 .AX, #0
    beq @__cmp_zero_2
    bcs @__cmp_set_2
    bra @__cmp_zero_2
@__cmp_set_2:
    lda #1
    ldx #0
    bra @__cmp_done_2
@__cmp_zero_2:
    lda #0
    ldx #0
@__cmp_done_2:
    sta $2A
    .loc "test_memcpy.c", 21
    lda $26
    sec
    sbc #1
    sta $20
    lda $27
    sbc #0
    sta $21
@while_cond32:
    .loc "test_memcpy.c", 17
    lda #0
    sta $28
    sta $29
    lda $2A
    bne @while_body33
    bra @while_end34
@while_body33:
    .loc "test_memcpy.c", 18
    lda _main__local_46
    ldx _main__local_46+1
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $2C
    lda _main__local_45
    ldx _main__local_45+1
    sta $2E
    stx $2F
    lda $2C
    ldy #0
    sta ($2E),y
    .loc "test_memcpy.c", 19
    lda _main__local_45
    ldx _main__local_45+1
    add.16 .AX, #1
    sta $32
    stx $33
    sta _main__local_45
    stx _main__local_45+1
    .loc "test_memcpy.c", 20
    lda _main__local_46
    ldx _main__local_46+1
    add.16 .AX, #1
    sta $36
    stx $37
    sta _main__local_46
    stx _main__local_46+1
    .loc "test_memcpy.c", 21
    lda #1
    ldx #0
    sta $38
    stx $39
    lda $20
    ldx $21
    ldy #0
    sta ($26),y
    txa
    iny
    sta ($26),y
    bra @while_cond32
@while_end34:
    .loc "test_memcpy.c", 23
@inline_end31:
    .loc "test_memcpy.c", 51
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_dst
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
    sta $26
    lda #66
    sta $20
    lda $26
    ldx #0
    sxt.8
    sta $24
    stx $25
    lda $20
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $24
    ldx $25
    cmp.16 .AX, $26
    bne @if_then36
    bra @if_end38
@if_then36:
    lda #3
    ldx #0
    bra @__return
@if_end38:
    .loc "test_memcpy.c", 52
    lda #3
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_dst
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
    sta $26
    lda #66
    sta $20
    lda $26
    ldx #0
    sxt.8
    sta $24
    stx $25
    lda $20
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $24
    ldx $25
    cmp.16 .AX, $26
    bne @if_then39
    bra @if_end41
@if_then39:
    lda #4
    ldx #0
    bra @__return
@if_end41:
    .loc "test_memcpy.c", 55
    lda #4
    ldx #0
    sta $20
    stx $21
    lda $22
    ldx $23
    sta $24
    stx $25
    ldax #_dst
    sta $26
    stx $27
    lda $20
    ldx $21
    sta $28
    stx $29
@while_cond46_ph:
    .loc "test_memcpy.c", 27
    .loc "test_memcpy.c", 32
    .loc "test_memcpy.c", 33
    .loc "test_memcpy.c", 34
    .loc "test_memcpy.c", 27
    lda $28
    ldx $29
    cmp.16 .AX, #0
    beq @__cmp_zero_3
    bcs @__cmp_set_3
    bra @__cmp_zero_3
@__cmp_set_3:
    lda #1
    ldx #0
    bra @__cmp_done_3
@__cmp_zero_3:
    lda #0
    ldx #0
@__cmp_done_3:
    sta $30
    .loc "test_memcpy.c", 32
    lda $24
    ldx $25
    add.16 .AX, #1
    sta $20
    stx $21
    .loc "test_memcpy.c", 33
    lda $26
    ldx $27
    add.16 .AX, #1
    sta $2A
    stx $2B
    .loc "test_memcpy.c", 34
    lda $28
    sec
    sbc #1
    sta $2C
    lda $29
    sbc #0
    sta $2D
@while_cond46:
    .loc "test_memcpy.c", 27
    lda #0
    sta $2E
    sta $2F
    lda $30
    bne @while_body47
    bra @while_end48
@while_body47:
    .loc "test_memcpy.c", 28
    ldy #0
    lda ($24),y
    ldx #0
    sta $32
    ldy #0
    lda ($26),y
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
    bne @if_then49
    bra @if_end51
@if_then49:
    .loc "test_memcpy.c", 29
    ldy #0
    lda ($24),y
    ldx #0
    sta $3C
    ldy #0
    lda ($26),y
    ldx #0
    sta $3E
    lda $3C
    ldx #0
    ldx #0
    sta $40
    stx $41
    lda $3E
    ldx #0
    ldx #0
    sta $42
    stx $43
    lda $40
    ldx $41
    cmp.16 .AX, $42
    bcc @if_then52
    bra @if_end54
@if_then52:
    lda #255
    sta $46
    sta $47
    lda $46
    ldx $47
    sta $48
    stx $49
    bra @inline_end45
@if_end54:
    .loc "test_memcpy.c", 30
    lda #1
    ldx #0
    sta $4A
    stx $4B
    lda $4A
    ldx $4B
    sta $48
    stx $49
    bra @inline_end45
@if_end51:
    .loc "test_memcpy.c", 32
    lda #1
    ldx #0
    sta $4C
    stx $4D
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_memcpy.c", 33
    lda #1
    ldx #0
    sta $4E
    stx $4F
    lda $2A
    ldx $2B
    ldy #0
    sta ($26),y
    txa
    iny
    sta ($26),y
    .loc "test_memcpy.c", 34
    lda #1
    ldx #0
    sta $50
    stx $51
    lda $2C
    ldx $2D
    ldy #0
    sta ($28),y
    txa
    iny
    sta ($28),y
    bra @while_cond46
@while_end48:
    .loc "test_memcpy.c", 36
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $48
    stx $49
@inline_end45:
    .loc "test_memcpy.c", 55
    lda $48
    ora $49
    bne @if_then42
    bra @if_end44
@if_then42:
    lda #5
    ldx #0
    bra @__return
@if_end44:
    .loc "test_memcpy.c", 56
    lda #65
    sta $20
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
    ldax #_dst
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
    .loc "test_memcpy.c", 57
    lda #4
    ldx #0
    sta $20
    stx $21
    lda $22
    ldx $23
    sta $24
    stx $25
    ldax #_dst
    sta $22
    stx $23
    lda $20
    ldx $21
    sta $26
    stx $27
@while_cond62_ph:
    .loc "test_memcpy.c", 27
    .loc "test_memcpy.c", 32
    .loc "test_memcpy.c", 33
    .loc "test_memcpy.c", 34
    .loc "test_memcpy.c", 27
    lda $26
    ldx $27
    cmp.16 .AX, #0
    beq @__cmp_zero_4
    bcs @__cmp_set_4
    bra @__cmp_zero_4
@__cmp_set_4:
    lda #1
    ldx #0
    bra @__cmp_done_4
@__cmp_zero_4:
    lda #0
    ldx #0
@__cmp_done_4:
    sta $2E
    .loc "test_memcpy.c", 32
    lda $24
    ldx $25
    add.16 .AX, #1
    sta $20
    stx $21
    .loc "test_memcpy.c", 33
    lda $22
    ldx $23
    add.16 .AX, #1
    sta $28
    stx $29
    .loc "test_memcpy.c", 34
    lda $26
    sec
    sbc #1
    sta $2A
    lda $27
    sbc #0
    sta $2B
@while_cond62:
    .loc "test_memcpy.c", 27
    lda #0
    sta $2C
    sta $2D
    lda $2E
    bne @while_body63
    bra @while_end64
@while_body63:
    .loc "test_memcpy.c", 28
    ldy #0
    lda ($24),y
    ldx #0
    sta $30
    ldy #0
    lda ($22),y
    ldx #0
    sta $32
    lda $30
    ldx #0
    ldx #0
    sta $34
    stx $35
    lda $32
    ldx #0
    ldx #0
    sta $36
    stx $37
    lda $34
    ldx $35
    cmp.16 .AX, $36
    bne @if_then65
    bra @if_end67
@if_then65:
    .loc "test_memcpy.c", 29
    ldy #0
    lda ($24),y
    ldx #0
    sta $3A
    ldy #0
    lda ($22),y
    ldx #0
    sta $3C
    lda $3A
    ldx #0
    ldx #0
    sta $3E
    stx $3F
    lda $3C
    ldx #0
    ldx #0
    sta $40
    stx $41
    lda $3E
    ldx $3F
    cmp.16 .AX, $40
    bcc @if_then68
    bra @if_end70
@if_then68:
    lda #255
    sta $44
    sta $45
    lda $44
    ldx $45
    sta $46
    stx $47
    bra @inline_end61
@if_end70:
    .loc "test_memcpy.c", 30
    lda #1
    ldx #0
    sta $48
    stx $49
    lda $48
    ldx $49
    sta $46
    stx $47
    bra @inline_end61
@if_end67:
    .loc "test_memcpy.c", 32
    lda #1
    ldx #0
    sta $4A
    stx $4B
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_memcpy.c", 33
    lda #1
    ldx #0
    sta $4C
    stx $4D
    lda $28
    ldx $29
    ldy #0
    sta ($22),y
    txa
    iny
    sta ($22),y
    .loc "test_memcpy.c", 34
    lda #1
    ldx #0
    sta $4E
    stx $4F
    lda $2A
    ldx $2B
    ldy #0
    sta ($26),y
    txa
    iny
    sta ($26),y
    bra @while_cond62
@while_end64:
    .loc "test_memcpy.c", 36
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $46
    stx $47
@inline_end61:
    .loc "test_memcpy.c", 57
    lda $46
    ldx $47
    cmp.16 .AX, #0
    bcc @if_then58
    beq @if_then58
    bra @if_end60
@if_then58:
    lda #6
    ldx #0
    bra @__return
@if_end60:
    .loc "test_memcpy.c", 59
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 8
    endproc


__zp_save_buf:

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

    .global _copy
    .global _fill
    .global _swap
    .global _zero
    .global _mixed
    .global _use_local
    .global _main

    .segment "code"

; function _copy
; SAC inline storage: 4 bytes
    .global _copy__param_dst
    _copy__param_dst: .word 0
    .global _copy__param_src
    _copy__param_src: .word 0
    _copy__local_0: .word 0
    _copy__local_1: .word 0
    proc _copy, W#@_p_dst, W#@_p_src
    .sac
    .var _fp = 0
    .loc "test_restrict.c", 5
    .var @_p_dst = 2
    .var @_p_src = 4
; .debug_var: __copy @_p_dst offset=2 size=2 type=ptr scope=parameter
; .debug_var: __copy @_p_src offset=4 size=2 type=ptr scope=parameter

@entry:
    .loc "test_restrict.c", 6
    lda _copy__param_src
    ldx _copy__param_src+1
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $20
    stx $21
    lda _copy__param_dst
    ldx _copy__param_dst+1
    sta $22
    stx $23
    lda $20
    ldx $21
    ldy #0
    sta ($22),y
    txa
    iny
    sta ($22),y
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers N, Z
    .frame_size 4
    endproc

; function _fill
; SAC inline storage: 4 bytes
    .global _fill__param_dst
    _fill__param_dst: .word 0
    .global _fill__param_val
    _fill__param_val: .word 0
    _fill__local_0: .word 0
    _fill__local_1: .word 0
    proc _fill, W#@_p_dst, W#@_p_val
    .sac
    .var _fp = 0
    .loc "test_restrict.c", 9
    .var @_p_dst = 2
    .var @_p_val = 4
; .debug_var: __fill @_p_dst offset=2 size=2 type=ptr scope=parameter
; .debug_var: __fill @_p_val offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_restrict.c", 10
    lda _fill__param_dst
    ldx _fill__param_dst+1
    sta $20
    stx $21
    lda _fill__param_val
    ldx _fill__param_val+1
    ldy #0
    sta ($20),y
    txa
    iny
    sta ($20),y
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers N, Z
    .frame_size 4
    endproc

; function _swap
; SAC inline storage: 6 bytes
    .global _swap__param_a
    _swap__param_a: .word 0
    .global _swap__param_b
    _swap__param_b: .word 0
    _swap__local_0: .word 0
    _swap__local_1: .word 0
    _swap__local_2: .word 0
    proc _swap, W#@_p_a, W#@_p_b
    .sac
    .var _fp = 0
    .loc "test_restrict.c", 13
    .local @_l_tmp = 4
; .debug_var: __swap @_l_tmp offset=4 size=2 type=int16 scope=local
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __swap @_p_a offset=2 size=2 type=ptr scope=parameter
; .debug_var: __swap @_p_b offset=4 size=2 type=ptr scope=parameter

@entry:
    .loc "test_restrict.c", 14
    lda _swap__param_a
    ldx _swap__param_a+1
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _swap__local_2
    stx _swap__local_2+1
    .loc "test_restrict.c", 15
    lda _swap__param_b
    ldx _swap__param_b+1
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $20
    stx $21
    lda _swap__param_a
    ldx _swap__param_a+1
    sta $22
    stx $23
    lda $20
    ldx $21
    ldy #0
    sta ($22),y
    txa
    iny
    sta ($22),y
    .loc "test_restrict.c", 16
    lda _swap__param_b
    ldx _swap__param_b+1
    sta $20
    stx $21
    lda _swap__local_2
    ldx _swap__local_2+1
    ldy #0
    sta ($20),y
    txa
    iny
    sta ($20),y
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers N, Z
    .frame_size 6
    endproc

; function _zero
; SAC inline storage: 2 bytes
    .global _zero__param_p
    _zero__param_p: .word 0
    _zero__local_0: .word 0
    proc _zero, W#@_p_p
    .sac
    .var _fp = 0
    .loc "test_restrict.c", 20
    .var @_p_p = 2
; .debug_var: __zero @_p_p offset=2 size=2 type=ptr scope=parameter

@entry:
    .loc "test_restrict.c", 21
    lda #0
    sta $20
    sta $21
    lda _zero__param_p
    ldx _zero__param_p+1
    sta $22
    stx $23
    lda $20
    ldx $21
    ldy #0
    sta ($22),y
    txa
    iny
    sta ($22),y
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers N, Z
    .frame_size 2
    endproc

; function _mixed
; SAC inline storage: 4 bytes
    .global _mixed__param_p
    _mixed__param_p: .word 0
    _mixed__local_0: .word 0
    _mixed__local_1: .word 0
    proc _mixed, W#@_p_p
    .sac
    .var _fp = 0
    .loc "test_restrict.c", 25
    .local @_l_x = 2
; .debug_var: __mixed @_l_x offset=2 size=2 type=int16 scope=local
    .var @_p_p = 2
; .debug_var: __mixed @_p_p offset=2 size=2 type=ptr scope=parameter

@entry:
    .loc "test_restrict.c", 26
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .frame_size 4
    endproc

; function _use_local
; SAC inline storage: 4 bytes
    _use_local__local_0: .word 0
    _use_local__local_2: .word 0
    proc _use_local
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_restrict.c", 30
    .local @_l_p = 2
    .local @_l_val = 0
; .debug_var: __use_local @_l_p offset=2 size=2 type=ptr scope=local
; .debug_var: __use_local @_l_val offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_restrict.c", 31
    lda #42
    sta _use_local__local_0
    lda #0
    sta _use_local__local_0+1
    .loc "test_restrict.c", 32
    leax.local 0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _use_local__local_2
    stx _use_local__local_2+1
    .loc "test_restrict.c", 33
    lda _use_local__local_2
    ldx _use_local__local_2+1
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z
    .frame_size 4
    endproc

; function _main
; SAC inline storage: 12 bytes
    _main__local_0: .word 0
    _main__local_2: .word 0
    _main__local_21: .word 0
    _main__local_33: .word 0
    _main__local_35: .word 0
    _main__local_37: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_restrict.c", 36
    .local @_l_a = 0
    .local @_l_b = 2
    .local @_l_c = 6
    .local @_l_p = 10
    .local @_l_tmp = 4
    .local @_l_val = 8
; .debug_var: __main @_l_a offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_b offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_c offset=6 size=2 type=int16 scope=local
; .debug_var: __main @_l_p offset=10 size=2 type=ptr scope=local
; .debug_var: __main @_l_tmp offset=4 size=2 type=int16 scope=local
; .debug_var: __main @_l_val offset=8 size=2 type=int16 scope=local

@entry:
    .loc "test_restrict.c", 37
    lda #10
    sta _main__local_0
    lda #0
    sta _main__local_0+1
    .loc "test_restrict.c", 38
    lda #20
    sta _main__local_2
    lda #0
    sta _main__local_2+1
    .loc "test_restrict.c", 40
    leax.local 2
    sta $20
    stx $21
    leax.local 0
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
    .loc "test_restrict.c", 6
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $24
    stx $25
    lda $24
    ldx $25
    ldy #0
    sta ($20),y
    txa
    iny
    sta ($20),y
@inline_end0:
    .loc "test_restrict.c", 41
    lda _main__local_2
    ldx _main__local_2+1
    cmp.16 .AX, #10
    bne @if_then1
    bra @if_end3
@if_then1:
    lda #1
    ldx #0
    bra @__return
@if_end3:
    .loc "test_restrict.c", 43
    leax.local 0
    sta $20
    stx $21
    lda #99
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
    .loc "test_restrict.c", 10
    lda $22
    ldx $23
    ldy #0
    sta ($20),y
    txa
    iny
    sta ($20),y
@inline_end4:
    .loc "test_restrict.c", 44
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, #99
    bne @if_then5
    bra @if_end7
@if_then5:
    lda #2
    ldx #0
    bra @__return
@if_end7:
    .loc "test_restrict.c", 46
    leax.local 0
    sta $20
    stx $21
    leax.local 2
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
    .loc "test_restrict.c", 14
    ldy #0
    lda ($20),y
    pha
    iny
    lda ($20),y
    tax
    pla
    sta $24
    stx $25
    lda $24
    ldx $25
    sta _main__local_21
    stx _main__local_21+1
    .loc "test_restrict.c", 15
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $24
    stx $25
    lda $24
    ldx $25
    ldy #0
    sta ($20),y
    txa
    iny
    sta ($20),y
    .loc "test_restrict.c", 16
    lda _main__local_21
    ldx _main__local_21+1
    ldy #0
    sta ($22),y
    txa
    iny
    sta ($22),y
@inline_end8:
    .loc "test_restrict.c", 47
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, #10
    bne @if_then9
    bra @if_end11
@if_then9:
    lda #3
    ldx #0
    bra @__return
@if_end11:
    .loc "test_restrict.c", 48
    lda _main__local_2
    ldx _main__local_2+1
    cmp.16 .AX, #99
    bne @if_then12
    bra @if_end14
@if_then12:
    lda #4
    ldx #0
    bra @__return
@if_end14:
    .loc "test_restrict.c", 50
    leax.local 0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_restrict.c", 21
    lda #0
    sta $22
    sta $23
    lda $22
    ldx $23
    ldy #0
    sta ($20),y
    txa
    iny
    sta ($20),y
@inline_end15:
    .loc "test_restrict.c", 51
    lda _main__local_0
    ldx _main__local_0+1
    stx __zp_scratch
    ora __zp_scratch
    bne @if_then16
    bra @if_end18
@if_then16:
    lda #5
    ldx #0
    bra @__return
@if_end18:
    .loc "test_restrict.c", 31
    lda #42
    sta _main__local_35
    lda #0
    sta _main__local_35+1
    .loc "test_restrict.c", 32
    leax.local 8
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_37
    stx _main__local_37+1
    .loc "test_restrict.c", 33
    lda _main__local_37
    ldx _main__local_37+1
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end19:
    .loc "test_restrict.c", 53
    lda $22
    ldx $23
    sta _main__local_33
    stx _main__local_33+1
    .loc "test_restrict.c", 54
    lda _main__local_33
    ldx _main__local_33+1
    cmp.16 .AX, #42
    bne @if_then21
    bra @if_end23
@if_then21:
    lda #6
    ldx #0
    bra @__return
@if_end23:
    .loc "test_restrict.c", 56
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 12
    endproc


__zp_save_buf:

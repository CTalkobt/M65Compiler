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
    .global _test_scalars
    .global _test_scalar_assign
    .global _test_scalar_after_call
    .global _helper_func

    .segment "data"
    .byte 0
_result:
; .debug_var: @global _result offset=0 size=2 type=ptr scope=global
    .word 16384

    .segment "code"

; function _test_scalars
; SAC inline storage: 8 bytes
    _test_scalars__local_0: .word 0
    _test_scalars__local_2: .word 0
    _test_scalars__local_4: .word 0
    _test_scalars__local_6: .word 0
    _test_scalars__local_13: .word 0
    _test_scalars__local_25: .word 0
    _test_scalars__local_34: .word 0
    proc _test_scalars
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_scalar_init.c", 8
    .local @_l_a = 0
    .local @_l_b = 2
    .local @_l_c = 4
    .local @_l_d = 6
; .debug_var: __test_scalars @_l_a offset=0 size=2 type=int16 scope=local
; .debug_var: __test_scalars @_l_b offset=2 size=2 type=int8 scope=local
; .debug_var: __test_scalars @_l_c offset=4 size=2 type=int16 scope=local
; .debug_var: __test_scalars @_l_d offset=6 size=2 type=int16 scope=local

@entry:
    .loc "test_scalar_init.c", 10
    lda #10
    sta _test_scalars__local_0
    lda #0
    sta _test_scalars__local_0+1
    .loc "test_scalar_init.c", 11
    lda #20
    sta _test_scalars__local_2
    lda #0
    sta _test_scalars__local_2+1
    .loc "test_scalar_init.c", 12
    lda #30
    sta _test_scalars__local_4
    lda #0
    sta _test_scalars__local_4+1
    .loc "test_scalar_init.c", 13
    lda #40
    sta _test_scalars__local_6
    lda #0
    sta _test_scalars__local_6+1
    lda _test_scalars__local_0
    ldx _test_scalars__local_0+1
    sta $20
    .loc "test_scalar_init.c", 16
    lda _result
    ldx _result+1
    sta $22
    stx $23
    lda #0
    sta $24
    sta $25
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
    .loc "test_scalar_init.c", 17
    lda _test_scalars__local_0
    ldx _test_scalars__local_0+1
    txa
    ldx #0
    sta $20
    stx $21
    lda _result
    ldx _result+1
    sta $22
    stx $23
    lda #1
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx $21
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
    .loc "test_scalar_init.c", 18
    lda _result
    ldx _result+1
    sta $20
    stx $21
    lda #2
    ldx #0
    sta $22
    stx $23
    lda _test_scalars__local_2
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
    lda _test_scalars__local_4
    ldx _test_scalars__local_4+1
    sta $20
    .loc "test_scalar_init.c", 19
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
    .loc "test_scalar_init.c", 20
    lda _test_scalars__local_4
    ldx _test_scalars__local_4+1
    txa
    ldx #0
    sta $20
    stx $21
    lda _result
    ldx _result+1
    sta $22
    stx $23
    lda #4
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx $21
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
    lda _test_scalars__local_6
    ldx _test_scalars__local_6+1
    sta $20
    .loc "test_scalar_init.c", 21
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
    .loc "test_scalar_init.c", 22
    lda _test_scalars__local_6
    ldx _test_scalars__local_6+1
    txa
    ldx #0
    sta $20
    stx $21
    lda _result
    ldx _result+1
    sta $22
    stx $23
    lda #6
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx $21
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
    .loc "test_scalar_init.c", 23
    lda #170
    sta $20
    lda _result
    ldx _result+1
    sta $22
    stx $23
    lda #7
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
    .loc "test_scalar_init.c", 25
    brk
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 8
    endproc

; function _test_scalar_assign
; SAC inline storage: 4 bytes
    _test_scalar_assign__local_0: .word 0
    _test_scalar_assign__local_2: .word 0
    _test_scalar_assign__local_9: .word 0
    proc _test_scalar_assign
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_scalar_init.c", 28
    .local @_l_x = 0
    .local @_l_y = 2
; .debug_var: __test_scalar_assign @_l_x offset=0 size=2 type=int16 scope=local
; .debug_var: __test_scalar_assign @_l_y offset=2 size=2 type=int8 scope=local

@entry:
    .loc "test_scalar_init.c", 30
    lda #20
    sta _test_scalar_assign__local_0
    lda #0
    sta _test_scalar_assign__local_0+1
    .loc "test_scalar_init.c", 31
    lda #21
    sta _test_scalar_assign__local_2
    lda #0
    sta _test_scalar_assign__local_2+1
    lda _test_scalar_assign__local_0
    ldx _test_scalar_assign__local_0+1
    sta $20
    .loc "test_scalar_init.c", 33
    lda _result
    ldx _result+1
    sta $22
    stx $23
    lda #0
    sta $24
    sta $25
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
    .loc "test_scalar_init.c", 34
    lda _test_scalar_assign__local_0
    ldx _test_scalar_assign__local_0+1
    txa
    ldx #0
    sta $20
    stx $21
    lda _result
    ldx _result+1
    sta $22
    stx $23
    lda #1
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx $21
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
    .loc "test_scalar_init.c", 35
    lda _result
    ldx _result+1
    sta $20
    stx $21
    lda #2
    ldx #0
    sta $22
    stx $23
    lda _test_scalar_assign__local_2
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
    .loc "test_scalar_init.c", 36
    lda #187
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
    .loc "test_scalar_init.c", 38
    brk
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _test_scalar_after_call
; SAC inline storage: 8 bytes
    .global _test_scalar_after_call__param_helper
    _test_scalar_after_call__param_helper: .word 0
    _test_scalar_after_call__local_0: .word 0
    _test_scalar_after_call__local_1: .word 0
    _test_scalar_after_call__local_3: .word 0
    _test_scalar_after_call__local_5: .word 0
    _test_scalar_after_call__local_12: .word 0
    _test_scalar_after_call__local_18: .word 0
    _test_scalar_after_call__local_27: .word 0
    proc _test_scalar_after_call, W#@_p_helper
    .sac
    .var _fp = 0
    .loc "test_scalar_init.c", 41
    .local @_l_after = 6
    .local @_l_after_call = 4
    .local @_l_before = 2
; .debug_var: __test_scalar_after_call @_l_after offset=6 size=2 type=int16 scope=local
; .debug_var: __test_scalar_after_call @_l_after_call offset=4 size=2 type=int16 scope=local
; .debug_var: __test_scalar_after_call @_l_before offset=2 size=2 type=int16 scope=local
    .var @_p_helper = 2
; .debug_var: __test_scalar_after_call @_p_helper offset=2 size=2 type=ptr scope=parameter

@entry:
    .loc "test_scalar_init.c", 44
    lda #100
    sta _test_scalar_after_call__local_1
    lda #0
    sta _test_scalar_after_call__local_1+1
    .loc "test_scalar_init.c", 45
    lda _test_scalar_after_call__param_helper
    ldx _test_scalar_after_call__param_helper+1
    sta @__call_site_0+1
    stx @__call_site_0+2
@__call_site_0:
    jsr $0000
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _test_scalar_after_call__local_3
    stx _test_scalar_after_call__local_3+1
    .loc "test_scalar_init.c", 46
    lda #200
    sta _test_scalar_after_call__local_5
    lda #0
    sta _test_scalar_after_call__local_5+1
    lda _test_scalar_after_call__local_1
    ldx _test_scalar_after_call__local_1+1
    sta $20
    .loc "test_scalar_init.c", 48
    lda _result
    ldx _result+1
    sta $22
    stx $23
    lda #0
    sta $24
    sta $25
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
    .loc "test_scalar_init.c", 49
    lda _test_scalar_after_call__local_1
    ldx _test_scalar_after_call__local_1+1
    txa
    ldx #0
    sta $20
    stx $21
    lda _result
    ldx _result+1
    sta $22
    stx $23
    lda #1
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx $21
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
    .loc "test_scalar_init.c", 50
    lda #255
    ldx #0
    sta $20
    stx $21
    lda _test_scalar_after_call__local_3
    ldx _test_scalar_after_call__local_3+1
    and $20
    sta $22
    stx $23
    lda _result
    ldx _result+1
    sta $20
    stx $21
    lda #2
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    pha
    lda $24
    ldx $25
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
    lda _test_scalar_after_call__local_5
    ldx _test_scalar_after_call__local_5+1
    sta $20
    .loc "test_scalar_init.c", 51
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
    .loc "test_scalar_init.c", 52
    lda _test_scalar_after_call__local_5
    ldx _test_scalar_after_call__local_5+1
    txa
    ldx #0
    sta $20
    stx $21
    lda _result
    ldx _result+1
    sta $22
    stx $23
    lda #4
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx $21
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
    .loc "test_scalar_init.c", 53
    lda #204
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
    .loc "test_scalar_init.c", 55
    brk
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 8
    endproc

; function _helper_func
; SAC zero-alloc leaf: no storage overhead
    proc _helper_func
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_scalar_init.c", 58

@entry:
    .loc "test_scalar_init.c", 59
    lda #42
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, zeroalloc, leaf
    .reg_clobbers A, X
    .flag_clobbers N, Z
    .frame_size 0
    endproc


__zp_save_buf:

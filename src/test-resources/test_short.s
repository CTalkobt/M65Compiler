    .o45
    .extern __sp_base
    .global _add_short__ar
    .global _main__ar
    .global _mul_short__ar
    .weak __static_chain
    .weak __zp_scratch
    .weak __zp_scratch2
    .weak __zp_scratch3
    .weak __zp_scratch4
    .global cc45.zeroPageStart
    __static_chain = $06
    __zp_scratch = $08
    __zp_scratch2 = $0A
    __zp_scratch3 = $0C
    __zp_scratch4 = $0E
    cc45.zeroPageStart = $08

    .global _r
    .global _add_short
    .global _mul_short
    .global _main

    .segment "data"
    .byte 0
_r:
; .debug_var: @global _r offset=0 size=2 type=ptr scope=global
    .word 16384

    .segment "code"

; function _add_short
; SAC inline storage: 4 bytes
    _add_short__param_a: .word 0
    _add_short__param_b: .word 0
    _add_short__local_0: .word 0
    _add_short__local_1: .word 0
    proc _add_short, W#@_p_a, W#@_p_b
    .sac
    .var _fp = 0
    .loc "test_short.c", 7
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __add_short @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __add_short @_p_b offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_short.c", 8
    lda _add_short__param_b
    ldx _add_short__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _add_short__param_a
    ldx _add_short__param_a+1
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

; function _mul_short
; SAC inline storage: 4 bytes
    _mul_short__param_a: .word 0
    _mul_short__param_b: .word 0
    _mul_short__local_0: .word 0
    _mul_short__local_1: .word 0
    proc _mul_short, W#@_p_a, W#@_p_b
    .sac
    .var _fp = 0
    .loc "test_short.c", 11
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __mul_short @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __mul_short @_p_b offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_short.c", 12
    lda _mul_short__param_b
    ldx _mul_short__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _mul_short__param_a
    ldx _mul_short__param_a+1
    mul.16 .AX, __zp_scratch2
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
; SAC inline storage: 16 bytes
    _main__local_0: .word 0
    _main__local_2: .word 0
    _main__local_4: .word 0
    _main__local_6: .word 0
    _main__local_8: .word 0
    _main__local_16: .word 0
    _main__local_32: .word 0
    _main__local_33: .word 0
    proc _main
    .sac
    .var _fp = 0
    .loc "test_short.c", 15
    .local @_l_arr = 10
    .local @_l_neg = 6
    .local @_l_p = 8
    .local @_l_x = 0
    .local @_l_y = 2
    .local @_l_z = 4
; .debug_var: __main @_l_arr offset=10 size=2 type=int16 scope=local
; .debug_var: __main @_l_neg offset=6 size=2 type=int16 scope=local
; .debug_var: __main @_l_p offset=8 size=2 type=ptr scope=local
; .debug_var: __main @_l_x offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_y offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_z offset=4 size=2 type=int16 scope=local

@entry:
    .loc "test_short.c", 16
    lda #10
    sta _main__local_0
    lda #0
    sta _main__local_0+1
    .loc "test_short.c", 17
    lda #20
    sta _main__local_2
    lda #0
    sta _main__local_2+1
    .loc "test_short.c", 18
    lda _main__local_2
    ldx _main__local_2+1
    sta _add_short__param_b
    stx _add_short__param_b+1
    lda _main__local_0
    ldx _main__local_0+1
    sta _add_short__param_a
    stx _add_short__param_a+1
    jsr _add_short
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_4
    stx _main__local_4+1
    .loc "test_short.c", 19
    lda #251
    sta _main__local_6
    lda #255
    sta _main__local_6+1
    .loc "test_short.c", 20
    leax.local 10
    sta $20
    stx $21
    lda #100
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    pha
    phx
    struct_elem.16 __zp_scratch, $20, #0
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    lda #200
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    pha
    phx
    struct_elem.16 __zp_scratch, $20, #2
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    lda #44
    ldx #1
    sta $22
    stx $23
    lda $22
    ldx $23
    pha
    phx
    struct_elem.16 __zp_scratch, $20, #4
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    .loc "test_short.c", 21
    leax.local 0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_16
    stx _main__local_16+1
    lda _main__local_4
    ldx _main__local_4+1
    sta $20
    .loc "test_short.c", 23
    lda _r
    ldx _r+1
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
    .loc "test_short.c", 24
    lda _main__local_6
    ldx _main__local_6+1
    clc
    adc #10
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    lda _r
    ldx _r+1
    sta $20
    stx $21
    lda #1
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
    .loc "test_short.c", 25
    lda #2
    sta $20
    lda _r
    ldx _r+1
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
    .loc "test_short.c", 26
    lda _main__local_33
    ldx _main__local_33+1
    sta _mul_short__param_b
    stx _mul_short__param_b+1
    lda _main__local_32
    ldx _main__local_32+1
    sta _mul_short__param_a
    stx _mul_short__param_a+1
    jsr _mul_short
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda _r
    ldx _r+1
    sta $20
    stx $21
    lda #3
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx #0
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
    .loc "test_short.c", 27
    lda _main__local_16
    ldx _main__local_16+1
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
    lda _r
    ldx _r+1
    sta $20
    stx $21
    lda #4
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx #0
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
    .loc "test_short.c", 28
    leax.local 10
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
    sta $20
    lda _r
    ldx _r+1
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
    .loc "test_short.c", 29
    lda #170
    sta $20
    lda _r
    ldx _r+1
    sta $22
    stx $23
    lda #6
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
    .loc "test_short.c", 31
    brk
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 16
    endproc


__zp_save_buf:

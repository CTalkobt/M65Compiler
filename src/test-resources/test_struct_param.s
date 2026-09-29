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

    .global _setPoint
    .global _getX
    .global _getY
    .global _initRect
    .global _area
    .global _main

    .segment "code"

; function _setPoint
; SAC inline storage: 6 bytes
    .global _setPoint__param_p
    _setPoint__param_p: .word 0
    .global _setPoint__param_x
    _setPoint__param_x: .word 0
    .global _setPoint__param_y
    _setPoint__param_y: .word 0
    _setPoint__local_0: .word 0
    _setPoint__local_1: .word 0
    _setPoint__local_2: .word 0
    _setPoint__local_3: .word 0
    proc _setPoint, W#@_p_p, W#@_p_x, W#@_p_y
    .sac
    .var _fp = 0
    .loc "test_struct_param.c", 12
    .var @_p_p = 2
    .var @_p_x = 4
    .var @_p_y = 6
; .debug_var: __setPoint @_p_p offset=2 size=2 type=ptr scope=parameter
; .debug_var: __setPoint @_p_x offset=4 size=2 type=int16 scope=parameter
; .debug_var: __setPoint @_p_y offset=6 size=2 type=int16 scope=parameter

@entry:
    .loc "test_struct_param.c", 13
    lda _setPoint__param_x
    ldx _setPoint__param_x+1
    sta _setPoint__param_p
    stx _setPoint__param_p+1
    .loc "test_struct_param.c", 14
    lda _setPoint__param_p
    ldx _setPoint__param_p+1
    add.16 .AX, #2
    sta $20
    stx $21
    lda _setPoint__param_y
    ldx _setPoint__param_y+1
    ldy #0
    sta ($20),y
    txa
    iny
    sta ($20),y
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _getX
; SAC inline storage: 2 bytes
    .global _getX__param_p
    _getX__param_p: .word 0
    _getX__local_0: .word 0
    _getX__local_1: .word 0
    proc _getX, W#@_p_p
    .sac
    .var _fp = 0
    .loc "test_struct_param.c", 17
    .var @_p_p = 2
; .debug_var: __getX @_p_p offset=2 size=2 type=ptr scope=parameter

@entry:
    .loc "test_struct_param.c", 18
    lda _getX__param_p
    ldx _getX__param_p+1
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
    .flag_clobbers N, Z
    .frame_size 2
    endproc

; function _getY
; SAC inline storage: 2 bytes
    .global _getY__param_p
    _getY__param_p: .word 0
    _getY__local_0: .word 0
    proc _getY, W#@_p_p
    .sac
    .var _fp = 0
    .loc "test_struct_param.c", 21
    .var @_p_p = 2
; .debug_var: __getY @_p_p offset=2 size=2 type=ptr scope=parameter

@entry:
    .loc "test_struct_param.c", 22
    lda _getY__param_p
    ldx _getY__param_p+1
    add.16 .AX, #2
    sta $20
    stx $21
    ldy #0
    lda ($20),y
    pha
    iny
    lda ($20),y
    tax
    pla
    sta $22
    stx $23
    lda $22
    ldx $23
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _initRect
; SAC inline storage: 10 bytes
    .global _initRect__param_r
    _initRect__param_r: .word 0
    .global _initRect__param_x
    _initRect__param_x: .word 0
    .global _initRect__param_y
    _initRect__param_y: .word 0
    .global _initRect__param_w
    _initRect__param_w: .word 0
    .global _initRect__param_h
    _initRect__param_h: .word 0
    _initRect__local_0: .word 0
    _initRect__local_1: .word 0
    _initRect__local_2: .word 0
    _initRect__local_3: .word 0
    _initRect__local_4: .word 0
    proc _initRect, W#@_p_r, W#@_p_x, W#@_p_y, W#@_p_w, W#@_p_h
    .sac
    .var _fp = 0
    .loc "test_struct_param.c", 25
    .var @_p_r = 2
    .var @_p_x = 4
    .var @_p_y = 6
    .var @_p_w = 8
    .var @_p_h = 10
; .debug_var: __initRect @_p_r offset=2 size=2 type=ptr scope=parameter
; .debug_var: __initRect @_p_x offset=4 size=2 type=int16 scope=parameter
; .debug_var: __initRect @_p_y offset=6 size=2 type=int16 scope=parameter
; .debug_var: __initRect @_p_w offset=8 size=2 type=int16 scope=parameter
; .debug_var: __initRect @_p_h offset=10 size=2 type=int16 scope=parameter

@entry:
    .loc "test_struct_param.c", 26
    lda _initRect__param_r
    ldx _initRect__param_r+1
    add.16 .AX, #4
    sta $20
    stx $21
    lda _initRect__param_w
    ldx _initRect__param_w+1
    ldy #0
    sta ($20),y
    txa
    iny
    sta ($20),y
    .loc "test_struct_param.c", 27
    lda _initRect__param_r
    ldx _initRect__param_r+1
    add.16 .AX, #6
    sta $20
    stx $21
    lda _initRect__param_h
    ldx _initRect__param_h+1
    ldy #0
    sta ($20),y
    txa
    iny
    sta ($20),y
    .loc "test_struct_param.c", 28
    lda _initRect__param_r
    ldx _initRect__param_r+1
    sta $20
    stx $21
    lda _initRect__param_r
    ldx _initRect__param_r+1
    sta $20
    stx $21
    lda _initRect__param_x
    ldx _initRect__param_x+1
    sta $20
    stx $21
    lda _initRect__param_y
    ldx _initRect__param_y+1
    sta $20
    stx $21
    .loc "test_struct_param.c", 13
    lda _initRect__param_r
    ldx _initRect__param_r+1
    sta $20
    stx $21
    lda _initRect__param_x
    ldx _initRect__param_x+1
    sta _initRect__param_r
    stx _initRect__param_r+1
    .loc "test_struct_param.c", 14
    lda _initRect__param_r
    ldx _initRect__param_r+1
    add.16 .AX, #2
    sta $20
    stx $21
    lda _initRect__param_y
    ldx _initRect__param_y+1
    ldy #0
    sta ($20),y
    txa
    iny
    sta ($20),y
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 10
    endproc

; function _area
; SAC inline storage: 2 bytes
    .global _area__param_r
    _area__param_r: .word 0
    _area__local_0: .word 0
    proc _area, W#@_p_r
    .sac
    .var _fp = 0
    .loc "test_struct_param.c", 31
    .var @_p_r = 2
; .debug_var: __area @_p_r offset=2 size=2 type=ptr scope=parameter

@entry:
    .loc "test_struct_param.c", 32
    lda _area__param_r
    ldx _area__param_r+1
    add.16 .AX, #4
    sta $20
    stx $21
    ldy #0
    lda ($20),y
    pha
    iny
    lda ($20),y
    tax
    pla
    sta $22
    stx $23
    lda _area__param_r
    ldx _area__param_r+1
    add.16 .AX, #6
    sta $20
    stx $21
    ldy #0
    lda ($20),y
    pha
    iny
    lda ($20),y
    tax
    pla
    sta $24
    stx $25
    lda $22
    ldx $23
    mul.16 .AX, $24
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _main
; SAC inline storage: 18 bytes
    _main__local_0: .long 0
    _main__local_9: .word 0
    _main__local_16: .word 0
    _main__local_23: .word 0
    _main__local_42: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_struct_param.c", 35
    .local @_l_a = 4
    .local @_l_pt = 6
    .local @_l_px = 0
    .local @_l_py = 2
    .local @_l_rc = 10
; .debug_var: __main @_l_a offset=4 size=2 type=int16 scope=local
; .debug_var: __main @_l_pt offset=6 size=4 type=int32 scope=local
; .debug_var: __main @_l_px offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_py offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_rc offset=10 size=2 type=int16 scope=local

@entry:
    .loc "test_struct_param.c", 37
    leax.local 6
    sta $20
    stx $21
    lda #10
    ldx #0
    sta $22
    stx $23
    lda #20
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx $21
    sta $26
    stx $27
    lda $22
    ldx $23
    sta $26
    stx $27
    lda $24
    ldx $25
    sta $26
    stx $27
    .loc "test_struct_param.c", 13
    lda $20
    ldx $21
    sta $26
    stx $27
    lda $22
    ldx $23
    ldy #0
    sta ($20),y
    txa
    iny
    sta ($20),y
    .loc "test_struct_param.c", 14
    lda $20
    ldx $21
    add.16 .AX, #2
    sta $22
    stx $23
    lda $24
    ldx $25
    ldy #0
    sta ($22),y
    txa
    iny
    sta ($22),y
@inline_end1:
    .loc "test_struct_param.c", 38
    .loc "test_struct_param.c", 18
@inline_end2:
    .loc "test_struct_param.c", 38
    .loc "test_struct_param.c", 39
    .loc "test_struct_param.c", 22
@inline_end4:
    .loc "test_struct_param.c", 39
    .loc "test_struct_param.c", 42
    leax.local 10
    sta $20
    stx $21
    lda #5
    ldx #0
    sta $22
    stx $23
    lda #5
    ldx #0
    sta $24
    stx $25
    lda #100
    ldx #0
    sta $26
    stx $27
    lda #50
    ldx #0
    sta $28
    stx $29
    lda $20
    ldx $21
    sta $2A
    stx $2B
    lda $22
    ldx $23
    sta $2A
    stx $2B
    lda $24
    ldx $25
    sta $2A
    stx $2B
    lda $26
    ldx $27
    sta $2A
    stx $2B
    lda $28
    ldx $29
    sta $2A
    stx $2B
    .loc "test_struct_param.c", 26
    lda $20
    ldx $21
    add.16 .AX, #4
    sta $2A
    stx $2B
    lda $26
    ldx $27
    ldy #0
    sta ($2A),y
    txa
    iny
    sta ($2A),y
    .loc "test_struct_param.c", 27
    lda $20
    ldx $21
    add.16 .AX, #6
    sta $26
    stx $27
    lda $28
    ldx $29
    ldy #0
    sta ($26),y
    txa
    iny
    sta ($26),y
    .loc "test_struct_param.c", 28
    lda $20
    ldx $21
    sta $26
    stx $27
    lda $20
    ldx $21
    sta $26
    stx $27
    lda $22
    ldx $23
    sta $26
    stx $27
    lda $24
    ldx $25
    sta $26
    stx $27
    .loc "test_struct_param.c", 13
    lda $20
    ldx $21
    sta $26
    stx $27
    lda $22
    ldx $23
    ldy #0
    sta ($20),y
    txa
    iny
    sta ($20),y
    .loc "test_struct_param.c", 14
    lda $20
    ldx $21
    add.16 .AX, #2
    sta $22
    stx $23
    lda $24
    ldx $25
    ldy #0
    sta ($22),y
    txa
    iny
    sta ($22),y
@inline_end7:
@inline_end6:
    .loc "test_struct_param.c", 43
    .loc "test_struct_param.c", 32
@inline_end8:
    .loc "test_struct_param.c", 43
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 18
    endproc


__zp_save_buf:

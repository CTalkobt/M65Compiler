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

    .global _Shape_vtable
    .global _Circle_vtable
    .global _Shape__area
    .global _Shape__perimeter
    .global _Circle__area
    .global _get_area
    .global _main

    .segment "data"
    .byte 0
_Shape_vtable:
; .debug_var: @global _Shape_vtable offset=0 size=2 type=int16 scope=global
    .word _Shape__area
    .vtable_entry _Shape_vtable, 0, _Shape__area
    .word _Shape__perimeter
    .vtable_entry _Shape_vtable, 1, _Shape__perimeter
_Circle_vtable:
; .debug_var: @global _Circle_vtable offset=0 size=2 type=int16 scope=global
    .word _Circle__area
    .vtable_entry _Circle_vtable, 0, _Circle__area
    .word _Shape__perimeter
    .vtable_entry _Circle_vtable, 1, _Shape__perimeter

    .segment "code"

; function _Shape__area
; SAC inline storage: 2 bytes
    .global _Shape__area__param___this
    _Shape__area__param___this: .word 0
    _Shape__area__local_0: .word 0
    proc _Shape__area, W#@_p___this
    .sac
    .var _fp = 0
    .loc "test_devirtualization.c", 9
    .var @_p___this = 2
; .debug_var: __Shape__area @_p___this offset=2 size=2 type=ptr scope=parameter

@entry:
    .loc "test_devirtualization.c", 10
    lda _width
    ldx _width+1
    sta $20
    stx $21
    lda _height
    ldx _height+1
    sta $22
    stx $23
    lda $20
    ldx $21
    mul.16 .AX, $22
    sta $24
    stx $25
    lda $24
    ldx $25
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _Shape__perimeter
; SAC inline storage: 2 bytes
    .global _Shape__perimeter__param___this
    _Shape__perimeter__param___this: .word 0
    _Shape__perimeter__local_0: .word 0
    proc _Shape__perimeter, W#@_p___this
    .sac
    .var _fp = 0
    .loc "test_devirtualization.c", 14
    .var @_p___this = 2
; .debug_var: __Shape__perimeter @_p___this offset=2 size=2 type=ptr scope=parameter

@entry:
    .loc "test_devirtualization.c", 15
    lda #2
    ldx #0
    sta $20
    stx $21
    lda _width
    ldx _width+1
    sta $22
    stx $23
    lda _height
    ldx _height+1
    sta $24
    stx $25
    lda $22
    clc
    adc $24
    sta $26
    lda $23
    adc $24+1
    sta $27
    lda $20
    ldx $21
    mul.16 .AX, $26
    sta $22
    stx $23
    lda $22
    ldx $23
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _Circle__area
; SAC inline storage: 2 bytes
    .global _Circle__area__param___this
    _Circle__area__param___this: .word 0
    _Circle__area__local_0: .word 0
    proc _Circle__area, W#@_p___this
    .sac
    .var _fp = 0
    .loc "test_devirtualization.c", 22
    .var @_p___this = 2
; .debug_var: __Circle__area @_p___this offset=2 size=2 type=ptr scope=parameter

@entry:
    .loc "test_devirtualization.c", 23
    lda #58
    ldx #1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers N, Z
    .frame_size 2
    endproc

; function _get_area
; SAC inline storage: 2 bytes
    .global _get_area__param_s
    _get_area__param_s: .word 0
    _get_area__local_0: .word 0
    _get_area__local_3: .word 0
    proc _get_area, W#@_p_s
    .sac
    .var _fp = 0
    .loc "test_devirtualization.c", 28
    .var @_p_s = 2
; .debug_var: __get_area @_p_s offset=2 size=2 type=ptr scope=parameter

@entry:
    .loc "test_devirtualization.c", 29
    lda _get_area__param_s
    ldx _get_area__param_s+1
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
    ldy #0
    lda ($20),y
    pha
    iny
    lda ($20),y
    tax
    pla
    sta _get_area__local_3
    stx _get_area__local_3+1
    lda _get_area__param_s
    ldx _get_area__param_s+1
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    lda _get_area__local_3
    ldx _get_area__local_3+1
    sta @__call_site_0+1
    stx @__call_site_0+2
@__call_site_0:
    jsr $0000
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _main
; SAC inline storage: 20 bytes
    _main__local_0: .word 0
    _main__local_9: .word 0
    _main__local_18: .word 0
    _main__local_19: .word 0
    _main__local_22: .word 0
    _main__local_24: .word 0
    _main__local_26: .word 0
    _main__local_29: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_devirtualization.c", 32
    .local @_l_area1 = 0
    .local @_l_area2 = 4
    .local @_l_circle = 12
    .local @_l_rect = 6
    .local @_l_shape_ptr = 2
; .debug_var: __main @_l_area1 offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_area2 offset=4 size=2 type=int16 scope=local
; .debug_var: __main @_l_circle offset=12 size=2 type=int16 scope=local
; .debug_var: __main @_l_rect offset=6 size=2 type=int16 scope=local
; .debug_var: __main @_l_shape_ptr offset=2 size=2 type=ptr scope=local

@entry:
    .loc "test_devirtualization.c", 33
    leax.local 6
    sta $20
    stx $21
    ldax #Shape_vtable
    sta $22
    stx $23
    lda $22
    ldx $23
    ldy #0
    sta ($20),y
    txa
    iny
    sta ($20),y
    .loc "test_devirtualization.c", 34
    lda #10
    ldx #0
    sta $20
    stx $21
    leax.local 6
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #2
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_devirtualization.c", 35
    lda #20
    ldx #0
    sta $20
    stx $21
    leax.local 6
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #4
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_devirtualization.c", 37
    leax.local 12
    sta $20
    stx $21
    ldax #Circle_vtable
    sta $22
    stx $23
    lda $22
    ldx $23
    ldy #0
    sta ($20),y
    txa
    iny
    sta ($20),y
    .loc "test_devirtualization.c", 38
    lda #5
    ldx #0
    sta $20
    stx $21
    leax.local 12
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #2
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_devirtualization.c", 39
    lda #5
    ldx #0
    sta $20
    stx $21
    leax.local 12
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #4
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_devirtualization.c", 43
    leax.local 6
    sta _main__local_19
    stx _main__local_19+1
    lda _main__local_19
    ldx _main__local_19+1
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
    ldy #0
    lda ($20),y
    pha
    iny
    lda ($20),y
    tax
    pla
    sta _main__local_22
    stx _main__local_22+1
    lda _main__local_19
    ldx _main__local_19+1
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    lda _main__local_22
    ldx _main__local_22+1
    sta @__call_site_1+1
    stx @__call_site_1+2
@__call_site_1:
    jsr $0000
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_18
    stx _main__local_18+1
    .loc "test_devirtualization.c", 46
    leax.local 12
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_24
    stx _main__local_24+1
    .loc "test_devirtualization.c", 47
    lda _main__local_24
    ldx _main__local_24+1
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
    ldy #0
    lda ($20),y
    pha
    iny
    lda ($20),y
    tax
    pla
    sta _main__local_29
    stx _main__local_29+1
    lda _main__local_24
    ldx _main__local_24+1
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    lda _main__local_29
    ldx _main__local_29+1
    sta @__call_site_2+1
    stx @__call_site_2+2
@__call_site_2:
    jsr $0000
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_26
    stx _main__local_26+1
    .loc "test_devirtualization.c", 49
    lda _main__local_26
    ldx _main__local_26+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_18
    ldx _main__local_18+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 20
    endproc


__zp_save_buf:

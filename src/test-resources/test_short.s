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
    proc _add_short, W#@_p_a, W#@_p_b
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/src/test-resources/test_short.c", 7
; static activation record (SAC): 4 bytes
    .bss
    _add_short__ar:
    .fill 4
    .code
    .sac
    .var ar = _add_short__ar
    leax.fp 0
    sta $10
    stx $10+1
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __add_short @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __add_short @_p_b offset=4 size=2 type=int16 scope=parameter

    ldax.fp @_p_a
    sta _add_short:ar+0
    stx _add_short:ar+1
    ldax.fp @_p_b
    sta _add_short:ar+2
    stx _add_short:ar+3
@entry:
    .loc "/home/duck/m65/inpg/m65compiler/src/test-resources/test_short.c", 8
    lda _add_short:ar+2
    ldx _add_short:ar+3
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _add_short:ar+0
    ldx _add_short:ar+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _mul_short
    proc _mul_short, W#@_p_a, W#@_p_b
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/src/test-resources/test_short.c", 11
; static activation record (SAC): 4 bytes
    .bss
    _mul_short__ar:
    .fill 4
    .code
    .sac
    .var ar = _mul_short__ar
    leax.fp 0
    sta $12
    stx $12+1
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __mul_short @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __mul_short @_p_b offset=4 size=2 type=int16 scope=parameter

    ldax.fp @_p_a
    sta _mul_short:ar+0
    stx _mul_short:ar+1
    ldax.fp @_p_b
    sta _mul_short:ar+2
    stx _mul_short:ar+3
@entry:
    .loc "/home/duck/m65/inpg/m65compiler/src/test-resources/test_short.c", 12
    lda _mul_short:ar+2
    ldx _mul_short:ar+3
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _mul_short:ar+0
    ldx _mul_short:ar+1
    mul.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _main
    proc _main
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/src/test-resources/test_short.c", 15
; static activation record (SAC): 16 bytes
    .bss
    _main__ar:
    .fill 16
    .code
    .sac
    .var ar = _main__ar
    leax.fp 0
    sta $14
    stx $14+1
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
    .loc "/home/duck/m65/inpg/m65compiler/src/test-resources/test_short.c", 16
    lda #10
    ldz #0
    staz.fp __vr0
    .loc "/home/duck/m65/inpg/m65compiler/src/test-resources/test_short.c", 17
    lda #20
    ldz #0
    staz.fp __vr2
    .loc "/home/duck/m65/inpg/m65compiler/src/test-resources/test_short.c", 18
    lda _main:ar+2
    ldx _main:ar+3
    sta $28
    stx $29
    lda _main:ar+0
    ldx _main:ar+1
    sta $2A
    stx $2B
    lda $28
    ldx $29
    push .ax
    lda $2A
    ldx $2B
    push .ax
    jsr _add_short
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    plz
    plz
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main:ar+4
    stx _main:ar+5
    .loc "/home/duck/m65/inpg/m65compiler/src/test-resources/test_short.c", 19
    lda #251
    ldz #255
    staz.fp __vr6
    .loc "/home/duck/m65/inpg/m65compiler/src/test-resources/test_short.c", 20
    leax.fp 10
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
    .loc "/home/duck/m65/inpg/m65compiler/src/test-resources/test_short.c", 21
    lda $14
    ldx $14+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main:ar+8
    stx _main:ar+9
    lda _main:ar+4
    ldx _main:ar+5
    sta $20
    .loc "/home/duck/m65/inpg/m65compiler/src/test-resources/test_short.c", 23
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
    adc $0C
    pha
    txa
    adc $0D
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "/home/duck/m65/inpg/m65compiler/src/test-resources/test_short.c", 24
    lda _main:ar+6
    ldx _main:ar+7
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
    adc $0C
    pha
    txa
    adc $0D
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "/home/duck/m65/inpg/m65compiler/src/test-resources/test_short.c", 25
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
    adc $0C
    pha
    txa
    adc $0D
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "/home/duck/m65/inpg/m65compiler/src/test-resources/test_short.c", 26
    lda #4
    ldx #0
    push .ax
    lda #3
    ldx #0
    push .ax
    jsr _mul_short
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    plz
    plz
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
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
    adc $0C
    pha
    txa
    adc $0D
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "/home/duck/m65/inpg/m65compiler/src/test-resources/test_short.c", 27
    lda _main:ar+8
    ldx _main:ar+9
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
    adc $0C
    pha
    txa
    adc $0D
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "/home/duck/m65/inpg/m65compiler/src/test-resources/test_short.c", 28
    leax.fp 10
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
    adc $0C
    pha
    txa
    adc $0D
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
    adc $0C
    pha
    txa
    adc $0D
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "/home/duck/m65/inpg/m65compiler/src/test-resources/test_short.c", 29
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
    adc $0C
    pha
    txa
    adc $0D
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "/home/duck/m65/inpg/m65compiler/src/test-resources/test_short.c", 31
    brk
@__return:
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 16
    endproc


__zp_save_buf:

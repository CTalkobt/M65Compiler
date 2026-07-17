* = $2000
    __sp_base = $0101
; Save ZP $08-$FF to BSS buffer
    ldx #0
@__zp_save_loop:
    lda $08,x
    sta __zp_save_buf,x
    inx
    cpx #248
    bne @__zp_save_loop
    jsr _main
    sta $02
    stx $03
; Restore ZP $08-$FF from BSS buffer
    ldx #0
@__zp_restore_loop:
    lda __zp_save_buf,x
    sta $08,x
    inx
    cpx #248
    bne @__zp_restore_loop
    lda $02
    ldx $03
__halt:
    jmp __halt
    .global __static_chain
    .global __zp_scratch
    .global __zp_scratch2
    .global __zp_scratch3
    .global __zp_scratch4
    __static_chain = $06
    __zp_scratch = $08
    __zp_scratch2 = $0A
    __zp_scratch3 = $0C
    __zp_scratch4 = $0E

_r:
    .word 16384


; function _add_short
    proc _add_short, W#@_p_a, W#@_p_b
    .var _fp = 0
    .loc "src/test-resources/test_short.c", 7
; frame: 8 bytes (frame-allocated vRegs only)
    phw #0
    phw #0
    phw #0
    phw #0
    tsx
    txa
    clc
    adc #1
    sta $FD
    lda #$01
    adc #0
    sta $FE
    .frameptr_zp $FD
    leax.fp 0
    sta $08
    stx $08+1
    .local __vr0 = 4
    .local __vr1 = 6
    .var @_p_a = 10
    .var @_p_b = 12
; .debug_var: __add_short @_p_a offset=10 size=2 type=int16 scope=parameter
; .debug_var: __add_short @_p_b offset=12 size=2 type=int16 scope=parameter

    ldax.fp @_p_a
    stax.fp __vr0
    ldax.fp @_p_b
    stax.fp __vr1
@entry:
    .loc "src/test-resources/test_short.c", 8
    ldax.fp 6
    sta __zp_scratch2
    stx __zp_scratch2+1
    ldax.fp 4
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    .func_flags stack_call, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    endproc

; function _mul_short
    proc _mul_short, W#@_p_a, W#@_p_b
    .var _fp = 0
    .loc "src/test-resources/test_short.c", 11
; frame: 8 bytes (frame-allocated vRegs only)
    phw #0
    phw #0
    phw #0
    phw #0
    tsx
    txa
    clc
    adc #1
    sta $FD
    lda #$01
    adc #0
    sta $FE
    .frameptr_zp $FD
    leax.fp 0
    sta $0A
    stx $0A+1
    .local __vr0 = 4
    .local __vr1 = 6
    .var @_p_a = 10
    .var @_p_b = 12
; .debug_var: __mul_short @_p_a offset=10 size=2 type=int16 scope=parameter
; .debug_var: __mul_short @_p_b offset=12 size=2 type=int16 scope=parameter

    ldax.fp @_p_a
    stax.fp __vr0
    ldax.fp @_p_b
    stax.fp __vr1
@entry:
    .loc "src/test-resources/test_short.c", 12
    ldax.fp 6
    sta __zp_scratch2
    stx __zp_scratch2+1
    ldax.fp 4
    mul.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    .func_flags stack_call, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    endproc

; function _main
    proc _main
    .var _fp = 0
    .loc "src/test-resources/test_short.c", 15
; frame: 20 bytes (frame-allocated vRegs only)
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    tsx
    txa
    clc
    adc #1
    sta $FD
    lda #$01
    adc #0
    sta $FE
    .frameptr_zp $FD
    leax.fp 0
    sta $0C
    stx $0C+1
    .local __vr0 = 10
    .local __vr2 = 12
    .local __vr4 = 14
    .local __vr5 = 16
    .local __vr6 = 18
    .local __vr8 = 6
    .local __vr10 = 0
    .local __vr18 = 8
    .local __vr33 = 16
    .local __vr34 = 18
    .local @_l_arr = 0
    .local @_l_neg = 6
    .local @_l_p = 8
    .local @_l_x = 10
    .local @_l_y = 12
    .local @_l_z = 14
; .debug_var: __main @_l_arr offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_neg offset=6 size=2 type=int16 scope=local
; .debug_var: __main @_l_p offset=8 size=2 type=int16 scope=local
; .debug_var: __main @_l_x offset=10 size=2 type=int16 scope=local
; .debug_var: __main @_l_y offset=12 size=2 type=int16 scope=local
; .debug_var: __main @_l_z offset=14 size=2 type=int16 scope=local

@entry:
    .loc "src/test-resources/test_short.c", 16
    lda #10
    ldz #0
    staz.fp __vr0
    .loc "src/test-resources/test_short.c", 17
    .loc "src/test-resources/test_short.c", 18
    lda #20
    ldx #0
    push .ax
    lda #10
    ldx #0
    push .ax
    jsr _add_short
    phx
    pha
    tsx
    txa
    clc
    adc #1
    sta $FD
    lda #$01
    adc #0
    sta $FE
    pla
    plx
    plz
    plz
    plz
    plz
    sta $20
    stx $21
    lda $20
    ldx $21
    stax.fp __vr4
    .loc "src/test-resources/test_short.c", 19
    .loc "src/test-resources/test_short.c", 20
    lda $0C
    ldx $0C+1
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
    .loc "src/test-resources/test_short.c", 21
    leax.fp 10
    sta $20
    stx $21
    lda $20
    ldx $21
    stax.fp __vr18
    ldax.fp 14
    sta $20
    .loc "src/test-resources/test_short.c", 23
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
    add.16 .AX, __zp_scratch3
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "src/test-resources/test_short.c", 24
    lda #5
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda _r
    ldx _r+1
    sta $20
    stx $21
    lda #1
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
    add.16 .AX, __zp_scratch3
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "src/test-resources/test_short.c", 25
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
    add.16 .AX, __zp_scratch3
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "src/test-resources/test_short.c", 26
    lda #4
    ldx #0
    push .ax
    lda #3
    ldx #0
    push .ax
    jsr _mul_short
    phx
    pha
    tsx
    txa
    clc
    adc #1
    sta $FD
    lda #$01
    adc #0
    sta $FE
    pla
    plx
    plz
    plz
    plz
    plz
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
    add.16 .AX, __zp_scratch3
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "src/test-resources/test_short.c", 27
    ldax.fp 8
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
    add.16 .AX, __zp_scratch3
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "src/test-resources/test_short.c", 28
    lda $0C
    ldx $0C+1
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
    add.16 .AX, __zp_scratch3
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
    add.16 .AX, __zp_scratch3
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "src/test-resources/test_short.c", 29
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
    add.16 .AX, __zp_scratch3
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
@__return:
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    .func_flags stack_call
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc


__zp_save_buf:

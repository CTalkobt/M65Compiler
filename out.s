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

_result:
    .word 16384
_scores:
    .byte 0
    .res 4
_grid:
    .word 0
    .res 22


; function _main
    proc _main
    .var _fp = 0
    .loc "src/test-resources/test_array_loop.c", 9
; frame: 4 bytes (frame-allocated vRegs only)
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
    .local __vr0 = 0
    .local __vr1 = 2
    .local @_l_i = 0
    .local @_l_j = 2
; .debug_var: __main @_l_i offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_j offset=2 size=2 type=int16 scope=local

@entry:
    .loc "src/test-resources/test_array_loop.c", 14
    lda #0
    taz
    staz.fp __vr0
@for_cond0:
    ldax.fp __vr0
    cmp.16 .AX, #5
    bcc @for_body1
    bra @for_end3
@for_body1:
    .loc "src/test-resources/test_array_loop.c", 15
    ldax.fp __vr0
    clc
    adc #1
    sta $26
    stx $27
    lda $26
    ldx $27
    pha
    ldax.fp __vr0
    sta __zp_scratch2
    stx __zp_scratch2+1
    addr_elem.16 __zp_scratch, #_scores, __zp_scratch2, #1
    pla
    ldy #0
    sta (__zp_scratch),y
@for_inc2:
    .loc "src/test-resources/test_array_loop.c", 14
    ldax.fp __vr0
    sta $2A
    stx $2B
    lda $2A
    clc
    adc #1
    sta $2C
    lda $2B
    adc #0
    sta $2D
    lda $2C
    ldx $2D
    stax.fp __vr0
    bra @for_cond0
@for_end3:
    .loc "src/test-resources/test_array_loop.c", 19
    lda #0
    taz
    staz.fp __vr0
@for_cond4:
    ldax.fp __vr0
    cmp.16 .AX, #3
    bcc @for_body5
    bra @for_end7
@for_body5:
    .loc "src/test-resources/test_array_loop.c", 20
    lda #0
    taz
    staz.fp __vr1
@for_cond8_ph:
    .loc "src/test-resources/test_array_loop.c", 21
    lda #10
    ldx #0
    sta $26
    stx $27
    ldax.fp __vr0
    mul.16 .AX, $26
    sta $28
    stx $29
    ldax.fp __vr0
    sta __zp_scratch2
    stx __zp_scratch2+1
    addr_elem.16 $2A, #_grid, __zp_scratch2, #8
@for_cond8:
    .loc "src/test-resources/test_array_loop.c", 20
    ldax.fp __vr1
    cmp.16 .AX, #4
    bcc @for_body9
    bra @for_end11
@for_body9:
    .loc "src/test-resources/test_array_loop.c", 21
    lda #10
    ldx #0
    sta $30
    stx $31
    ldax.fp __vr1
    add.16 .AX, $28
    sta $32
    stx $33
    lda $32
    ldx $33
    pha
    phx
    ldax.fp __vr1
    sta __zp_scratch2
    stx __zp_scratch2+1
    addr_elem.16 __zp_scratch, $2A, __zp_scratch2, #2
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
@for_inc10:
    .loc "src/test-resources/test_array_loop.c", 20
    ldax.fp __vr1
    sta $36
    stx $37
    lda $36
    clc
    adc #1
    sta $38
    lda $37
    adc #0
    sta $39
    lda $38
    ldx $39
    stax.fp __vr1
    bra @for_cond8
@for_end11:
@for_inc6:
    .loc "src/test-resources/test_array_loop.c", 19
    ldax.fp __vr0
    sta $3A
    stx $3B
    lda $3A
    clc
    adc #1
    sta $3C
    lda $3B
    adc #0
    sta $3D
    lda $3C
    ldx $3D
    stax.fp __vr0
    bra @for_cond4
@for_end7:
    .loc "src/test-resources/test_array_loop.c", 26
    lda #0
    sta $20
    sta $21
    addr_elem.16 __zp_scratch, #_scores, $20, #1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $24
    lda _result
    ldx _result+1
    sta $20
    stx $21
    lda #0
    sta $22
    sta $23
    lda $24
    ldx #0
    pha
    addr_elem.16 __zp_scratch, $20, $22, #1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "src/test-resources/test_array_loop.c", 29
    lda #4
    ldx #0
    sta $20
    stx $21
    addr_elem.16 __zp_scratch, #_scores, $20, #1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $24
    lda _result
    ldx _result+1
    sta $20
    stx $21
    lda #1
    ldx #0
    sta $22
    stx $23
    lda $24
    ldx #0
    pha
    addr_elem.16 __zp_scratch, $20, $22, #1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "src/test-resources/test_array_loop.c", 32
    lda #0
    sta $20
    sta $21
    addr_elem.16 $22, #_grid, $20, #8
    lda #0
    sta $20
    sta $21
    addr_elem.16 __zp_scratch, $22, $20, #2
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
    lda _result
    ldx _result+1
    sta $22
    stx $23
    lda #2
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx #0
    pha
    addr_elem.16 __zp_scratch, $22, $24, #1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "src/test-resources/test_array_loop.c", 35
    lda #1
    ldx #0
    sta $20
    stx $21
    addr_elem.16 $22, #_grid, $20, #8
    lda #2
    ldx #0
    sta $20
    stx $21
    addr_elem.16 __zp_scratch, $22, $20, #2
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
    addr_elem.16 __zp_scratch, $22, $24, #1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "src/test-resources/test_array_loop.c", 38
    lda #2
    ldx #0
    sta $20
    stx $21
    addr_elem.16 $22, #_grid, $20, #8
    lda #3
    ldx #0
    sta $20
    stx $21
    addr_elem.16 __zp_scratch, $22, $20, #2
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
    lda _result
    ldx _result+1
    sta $22
    stx $23
    lda #4
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx #0
    pha
    addr_elem.16 __zp_scratch, $22, $24, #1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "src/test-resources/test_array_loop.c", 41
    lda #170
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
    addr_elem.16 __zp_scratch, $22, $24, #1
    pla
    ldy #0
    sta (__zp_scratch),y
@__return:
    plz
    plz
    plz
    plz
    .func_flags stack_call, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    endproc


__zp_save_buf:

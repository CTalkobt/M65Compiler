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

_output:
    .word 16384
_test_array:
    .byte 17
    .byte 34
    .byte 51
    .byte 68
    .byte 85
    .byte 102
    .byte 119
    .byte 136


; function _get_array_value
    proc _get_array_value, W#@_p_index
    .var _fp = 0
    .loc "src/test-resources/test_simop_runtime.c", 8
; frame: 2 bytes (frame-allocated vRegs only)
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
    .var @_p_index = 4

    ldax.fp @_p_index
    stax.fp __vr0
@entry:
    .loc "src/test-resources/test_simop_runtime.c", 9
    ldax #_test_array
    sta $1F
    stx $1F+1
    ldax.fp __vr0
    clc
    adc $1F
    pha
    txa
    adc $1F+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $22
    lda $22
    ldx #0
    sxt.8
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    plz
    plz
    .func_flags stack_call, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z
    endproc

; function _main
    proc _main
    .var _fp = 0
    .loc "src/test-resources/test_simop_runtime.c", 12
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
    .local __vr3 = 2

@entry:
    .loc "src/test-resources/test_simop_runtime.c", 17
    lda #0
    taz
    staz.fp __vr0
@for_cond0:
    ldax.fp __vr0
    cmp.16 .AX, #8
    bcc @for_body1
    bra @for_end3
@for_body1:
    .loc "src/test-resources/test_simop_runtime.c", 18
    ldax.fp __vr0
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _get_array_value
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
    .var _fp = _fp - 2
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda _output
    ldx _output+1
    sta $24
    stx $25
    lda $22
    ldx #0
    pha
    lda $24
    ldx $25
    sta $1F
    stx $1F+1
    ldax.fp __vr0
    clc
    adc $1F
    pha
    txa
    adc $1F+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    pla
    sta (__zp_scratch),y
@for_inc2:
    .loc "src/test-resources/test_simop_runtime.c", 17
    ldax.fp __vr0
    sta $28
    stx $29
    lda $28
    clc
    adc #1
    sta $2A
    lda $29
    adc #0
    sta $2B
    lda $2A
    ldx $2B
    stax.fp __vr0
    bra @for_cond0
@for_end3:
    .loc "src/test-resources/test_simop_runtime.c", 22
    lda #170
    sta $20
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #8
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx #0
    pha
    lda $22
    ldx $23
    sta $1F
    stx $1F+1
    lda $24
    ldx $25
    clc
    adc $1F
    pha
    txa
    adc $1F+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    pla
    sta (__zp_scratch),y
@__return:
    plz
    plz
    plz
    plz
    .func_flags stack_call
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc


__zp_save_buf:
    .res 248

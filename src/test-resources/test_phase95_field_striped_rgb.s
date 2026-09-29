; [Phase 87: Peephole Optimizer Applied]
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

    .extern _printf

    .global _image
    .global _test_red_field_access
    .global _test_green_field_access
    .global _test_blue_field_access
    .global _test_mixed_field_access
    .global _test_single_pixel_access
    .global _main

    .segment "data"
    .byte 0
_image:
; .debug_var: @global _image offset=0 size=2 type=int16 scope=global
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .res 176

    .segment "code"

; function _test_red_field_access
; SAC inline storage: 18 bytes
    _test_red_field_access__local_0: .word 0
    _test_red_field_access__local_8: .word 0
    _test_red_field_access__local_19: .word 0
    _test_red_field_access__local_30: .word 0
    _test_red_field_access__local_41: .word 0
    _test_red_field_access__local_52: .word 0
    _test_red_field_access__local_63: .word 0
    _test_red_field_access__local_74: .word 0
    _test_red_field_access__local_79: .word 0
    _test_red_field_access__local_85: .word 0
    _test_red_field_access__local_90: .word 0
    proc _test_red_field_access
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "src/test-resources/test_phase95_field_striped_rgb.c", 16
    .local @_l_red_sum = 0
    .local @_l_x = 16
; .debug_var: __test_red_field_access @_l_red_sum offset=0 size=2 type=int16 scope=local
; .debug_var: __test_red_field_access @_l_x offset=16 size=2 type=int16 scope=local

@entry:
    .loc "src/test-resources/test_phase95_field_striped_rgb.c", 37
    lda #0
    sta _test_red_field_access__local_0
    sta _test_red_field_access__local_0+1
@for_cond0_ph:
    lda #0
    tax
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond0:
    lda $24
    ldx $25
    cmp.16 .AX, #8
    bcc @for_body1
    bra @for_cond4_ph
@for_body1:
    lda #0
    sta $28
    sta $29
    lda $24
    ldx $25
    mul.16 .AX, #3
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
    sta $2A
    stx $2A+1
    ldy #0
    lda ($2A),y
    ldx #0
    sta $2C
    ; [peephole-opt]     lda $2C
    ldx #0
    ldx #0
    sta $2E
    stx $2F
    ; [peephole-opt]     lda $2E
    ; [peephole-opt]     ldx $2F
    sta _test_red_field_access__local_0
    stx _test_red_field_access__local_0+1
@for_inc2:
    lda $24
    ldx $25
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    clc
    adc #1
    sta $24
    lda $31
    adc #0
    sta $25
    bra @for_cond0
@for_cond4_ph:
    lda #1
    ldx #0
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond4:
    lda $24
    ldx $25
    cmp.16 .AX, #8
    bcc @for_body5
    bra @for_cond8_ph
@for_body5:
    lda #1
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #3
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
    sta $2A
    stx $2A+1
    ldy #0
    lda ($2A),y
    ldx #0
    sta $2C
    ; [peephole-opt]     lda $2C
    ldx #0
    ldx #0
    sta $2E
    stx $2F
    ; [peephole-opt]     lda $2E
    ; [peephole-opt]     ldx $2F
    sta _test_red_field_access__local_0
    stx _test_red_field_access__local_0+1
@for_inc6:
    lda $24
    ldx $25
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    clc
    adc #1
    sta $24
    lda $31
    adc #0
    sta $25
    bra @for_cond4
@for_cond8_ph:
    lda #2
    ldx #0
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond8:
    lda $24
    ldx $25
    cmp.16 .AX, #8
    bcc @for_body9
    bra @for_cond12_ph
@for_body9:
    lda #2
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #3
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
    sta $2A
    stx $2A+1
    ldy #0
    lda ($2A),y
    ldx #0
    sta $2C
    ; [peephole-opt]     lda $2C
    ldx #0
    ldx #0
    sta $2E
    stx $2F
    ; [peephole-opt]     lda $2E
    ; [peephole-opt]     ldx $2F
    sta _test_red_field_access__local_0
    stx _test_red_field_access__local_0+1
@for_inc10:
    lda $24
    ldx $25
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    clc
    adc #1
    sta $24
    lda $31
    adc #0
    sta $25
    bra @for_cond8
@for_cond12_ph:
    lda #3
    ldx #0
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond12:
    lda $24
    ldx $25
    cmp.16 .AX, #8
    bcc @for_body13
    bra @for_cond16_ph
@for_body13:
    lda #3
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #3
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
    sta $2A
    stx $2A+1
    ldy #0
    lda ($2A),y
    ldx #0
    sta $2C
    ; [peephole-opt]     lda $2C
    ldx #0
    ldx #0
    sta $2E
    stx $2F
    ; [peephole-opt]     lda $2E
    ; [peephole-opt]     ldx $2F
    sta _test_red_field_access__local_0
    stx _test_red_field_access__local_0+1
@for_inc14:
    lda $24
    ldx $25
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    clc
    adc #1
    sta $24
    lda $31
    adc #0
    sta $25
    bra @for_cond12
@for_cond16_ph:
    lda #4
    ldx #0
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond16:
    lda $24
    ldx $25
    cmp.16 .AX, #8
    bcc @for_body17
    bra @for_cond20_ph
@for_body17:
    lda #4
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #3
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
    sta $2A
    stx $2A+1
    ldy #0
    lda ($2A),y
    ldx #0
    sta $2C
    ; [peephole-opt]     lda $2C
    ldx #0
    ldx #0
    sta $2E
    stx $2F
    ; [peephole-opt]     lda $2E
    ; [peephole-opt]     ldx $2F
    sta _test_red_field_access__local_0
    stx _test_red_field_access__local_0+1
@for_inc18:
    lda $24
    ldx $25
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    clc
    adc #1
    sta $24
    lda $31
    adc #0
    sta $25
    bra @for_cond16
@for_cond20_ph:
    lda #5
    ldx #0
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond20:
    lda $24
    ldx $25
    cmp.16 .AX, #8
    bcc @for_body21
    bra @for_cond24_ph
@for_body21:
    lda #5
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #3
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
    sta $2A
    stx $2A+1
    ldy #0
    lda ($2A),y
    ldx #0
    sta $2C
    ; [peephole-opt]     lda $2C
    ldx #0
    ldx #0
    sta $2E
    stx $2F
    ; [peephole-opt]     lda $2E
    ; [peephole-opt]     ldx $2F
    sta _test_red_field_access__local_0
    stx _test_red_field_access__local_0+1
@for_inc22:
    lda $24
    ldx $25
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    clc
    adc #1
    sta $24
    lda $31
    adc #0
    sta $25
    bra @for_cond20
@for_cond24_ph:
    lda #6
    ldx #0
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond24:
    lda $24
    ldx $25
    cmp.16 .AX, #8
    bcc @for_body25
    bra @for_cond28_ph
@for_body25:
    lda #6
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #3
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
    sta $2A
    stx $2A+1
    ldy #0
    lda ($2A),y
    ldx #0
    sta $2C
    ; [peephole-opt]     lda $2C
    ldx #0
    ldx #0
    sta $2E
    stx $2F
    ; [peephole-opt]     lda $2E
    ; [peephole-opt]     ldx $2F
    sta _test_red_field_access__local_0
    stx _test_red_field_access__local_0+1
@for_inc26:
    lda $24
    ldx $25
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    clc
    adc #1
    sta $24
    lda $31
    adc #0
    sta $25
    bra @for_cond24
@for_cond28_ph:
    lda #7
    ldx #0
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond28:
    lda _test_red_field_access__local_79
    ldx _test_red_field_access__local_79+1
    cmp.16 .AX, #8
    bcc @for_body29
    bra @for_end31
@for_body29:
    lda #7
    ldx #0
    sta $26
    stx $27
    lda _test_red_field_access__local_79
    ldx _test_red_field_access__local_79+1
    mul.16 .AX, #3
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
    sta $28
    stx $28+1
    ldy #0
    lda ($28),y
    ldx #0
    sta $2A
    ; [peephole-opt]     lda $2A
    ldx #0
    ldx #0
    sta $2C
    stx $2D
    ; [peephole-opt]     lda $2C
    ; [peephole-opt]     ldx $2D
    sta _test_red_field_access__local_0
    stx _test_red_field_access__local_0+1
@for_inc30:
    lda _test_red_field_access__local_79
    ldx _test_red_field_access__local_79+1
    sta $2E
    stx $2F
    ; [peephole-opt]     lda $2E
    ; [peephole-opt]     ldx $2F
    add.16 .AX, #1
    sta _test_red_field_access__local_79
    stx _test_red_field_access__local_79+1
    bra @for_cond28
@for_end31:
    .loc "src/test-resources/test_phase95_field_striped_rgb.c", 43
    ldax #__str_32
    sta _test_red_field_access__local_90
    stx _test_red_field_access__local_90+1
    lda _test_red_field_access__local_0
    ldx _test_red_field_access__local_0+1
    sta $28
    stx $29
    lda _test_red_field_access__local_90
    ; [peephole-opt]     ldx _test_red_field_access__local_90+1
    sta $2A
    stx $2B
    ; [peephole-opt]     lda $28
    ; [peephole-opt]     ldx $29
    push .ax
    ; [peephole-opt]     lda $2A
    ; [peephole-opt]     ldx $2B
    push .ax
    jsr _printf
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ; [peephole-opt]     ldx __zp_scratch4+1
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 18
    endproc

; function _test_green_field_access
; SAC inline storage: 18 bytes
    _test_green_field_access__local_0: .word 0
    _test_green_field_access__local_79: .word 0
    _test_green_field_access__local_90: .word 0
    proc _test_green_field_access
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "src/test-resources/test_phase95_field_striped_rgb.c", 27
    .local @_l_green_sum = 0
    .local @_l_x = 16
; .debug_var: __test_green_field_access @_l_green_sum offset=0 size=2 type=int16 scope=local
; .debug_var: __test_green_field_access @_l_x offset=16 size=2 type=int16 scope=local

@entry:
    .loc "src/test-resources/test_phase95_field_striped_rgb.c", 48
    lda #0
    sta _test_green_field_access__local_0
    sta _test_green_field_access__local_0+1
@for_cond33_ph:
    lda #0
    tax
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond33:
    lda $24
    ldx $25
    cmp.16 .AX, #8
    bcc @for_body34
    bra @for_cond37_ph
@for_body34:
    lda #0
    sta $28
    sta $29
    lda $24
    ldx $25
    mul.16 .AX, #3
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
    sta $2A
    stx $2A+1
    lda $2A
    ldx $2B
    add.16 .AX, #1
    sta $2C
    stx $2D
    ldy #0
    lda ($2C),y
    ldx #0
    sta $2E
    ; [peephole-opt]     lda $2E
    ldx #0
    ldx #0
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    sta _test_green_field_access__local_0
    stx _test_green_field_access__local_0+1
@for_inc35:
    lda $24
    ldx $25
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    clc
    adc #1
    sta $24
    lda $33
    adc #0
    sta $25
    bra @for_cond33
@for_cond37_ph:
    lda #1
    ldx #0
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond37:
    lda $24
    ldx $25
    cmp.16 .AX, #8
    bcc @for_body38
    bra @for_cond41_ph
@for_body38:
    lda #1
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #3
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
    sta $2A
    stx $2A+1
    lda $2A
    ldx $2B
    add.16 .AX, #1
    sta $2C
    stx $2D
    ldy #0
    lda ($2C),y
    ldx #0
    sta $2E
    ; [peephole-opt]     lda $2E
    ldx #0
    ldx #0
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    sta _test_green_field_access__local_0
    stx _test_green_field_access__local_0+1
@for_inc39:
    lda $24
    ldx $25
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    clc
    adc #1
    sta $24
    lda $33
    adc #0
    sta $25
    bra @for_cond37
@for_cond41_ph:
    lda #2
    ldx #0
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond41:
    lda $24
    ldx $25
    cmp.16 .AX, #8
    bcc @for_body42
    bra @for_cond45_ph
@for_body42:
    lda #2
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #3
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
    sta $2A
    stx $2A+1
    lda $2A
    ldx $2B
    add.16 .AX, #1
    sta $2C
    stx $2D
    ldy #0
    lda ($2C),y
    ldx #0
    sta $2E
    ; [peephole-opt]     lda $2E
    ldx #0
    ldx #0
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    sta _test_green_field_access__local_0
    stx _test_green_field_access__local_0+1
@for_inc43:
    lda $24
    ldx $25
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    clc
    adc #1
    sta $24
    lda $33
    adc #0
    sta $25
    bra @for_cond41
@for_cond45_ph:
    lda #3
    ldx #0
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond45:
    lda $24
    ldx $25
    cmp.16 .AX, #8
    bcc @for_body46
    bra @for_cond49_ph
@for_body46:
    lda #3
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #3
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
    sta $2A
    stx $2A+1
    lda $2A
    ldx $2B
    add.16 .AX, #1
    sta $2C
    stx $2D
    ldy #0
    lda ($2C),y
    ldx #0
    sta $2E
    ; [peephole-opt]     lda $2E
    ldx #0
    ldx #0
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    sta _test_green_field_access__local_0
    stx _test_green_field_access__local_0+1
@for_inc47:
    lda $24
    ldx $25
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    clc
    adc #1
    sta $24
    lda $33
    adc #0
    sta $25
    bra @for_cond45
@for_cond49_ph:
    lda #4
    ldx #0
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond49:
    lda $24
    ldx $25
    cmp.16 .AX, #8
    bcc @for_body50
    bra @for_cond53_ph
@for_body50:
    lda #4
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #3
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
    sta $2A
    stx $2A+1
    lda $2A
    ldx $2B
    add.16 .AX, #1
    sta $2C
    stx $2D
    ldy #0
    lda ($2C),y
    ldx #0
    sta $2E
    ; [peephole-opt]     lda $2E
    ldx #0
    ldx #0
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    sta _test_green_field_access__local_0
    stx _test_green_field_access__local_0+1
@for_inc51:
    lda $24
    ldx $25
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    clc
    adc #1
    sta $24
    lda $33
    adc #0
    sta $25
    bra @for_cond49
@for_cond53_ph:
    lda #5
    ldx #0
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond53:
    lda $24
    ldx $25
    cmp.16 .AX, #8
    bcc @for_body54
    bra @for_cond57_ph
@for_body54:
    lda #5
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #3
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
    sta $2A
    stx $2A+1
    lda $2A
    ldx $2B
    add.16 .AX, #1
    sta $2C
    stx $2D
    ldy #0
    lda ($2C),y
    ldx #0
    sta $2E
    ; [peephole-opt]     lda $2E
    ldx #0
    ldx #0
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    sta _test_green_field_access__local_0
    stx _test_green_field_access__local_0+1
@for_inc55:
    lda $24
    ldx $25
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    clc
    adc #1
    sta $24
    lda $33
    adc #0
    sta $25
    bra @for_cond53
@for_cond57_ph:
    lda #6
    ldx #0
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond57:
    lda $24
    ldx $25
    cmp.16 .AX, #8
    bcc @for_body58
    bra @for_cond61_ph
@for_body58:
    lda #6
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #3
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
    sta $2A
    stx $2A+1
    lda $2A
    ldx $2B
    add.16 .AX, #1
    sta $2C
    stx $2D
    ldy #0
    lda ($2C),y
    ldx #0
    sta $2E
    ; [peephole-opt]     lda $2E
    ldx #0
    ldx #0
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    sta _test_green_field_access__local_0
    stx _test_green_field_access__local_0+1
@for_inc59:
    lda $24
    ldx $25
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    clc
    adc #1
    sta $24
    lda $33
    adc #0
    sta $25
    bra @for_cond57
@for_cond61_ph:
    lda #7
    ldx #0
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond61:
    lda _test_green_field_access__local_79
    ldx _test_green_field_access__local_79+1
    cmp.16 .AX, #8
    bcc @for_body62
    bra @for_end64
@for_body62:
    lda #7
    ldx #0
    sta $26
    stx $27
    lda _test_green_field_access__local_79
    ldx _test_green_field_access__local_79+1
    mul.16 .AX, #3
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
    sta $28
    stx $28+1
    lda $28
    ldx $29
    add.16 .AX, #1
    sta $2A
    stx $2B
    ldy #0
    lda ($2A),y
    ldx #0
    sta $2C
    ; [peephole-opt]     lda $2C
    ldx #0
    ldx #0
    sta $2E
    stx $2F
    ; [peephole-opt]     lda $2E
    ; [peephole-opt]     ldx $2F
    sta _test_green_field_access__local_0
    stx _test_green_field_access__local_0+1
@for_inc63:
    lda _test_green_field_access__local_79
    ldx _test_green_field_access__local_79+1
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    add.16 .AX, #1
    sta _test_green_field_access__local_79
    stx _test_green_field_access__local_79+1
    bra @for_cond61
@for_end64:
    .loc "src/test-resources/test_phase95_field_striped_rgb.c", 54
    ldax #__str_65
    sta _test_green_field_access__local_90
    stx _test_green_field_access__local_90+1
    lda _test_green_field_access__local_0
    ldx _test_green_field_access__local_0+1
    sta $28
    stx $29
    lda _test_green_field_access__local_90
    ; [peephole-opt]     ldx _test_green_field_access__local_90+1
    sta $2A
    stx $2B
    ; [peephole-opt]     lda $28
    ; [peephole-opt]     ldx $29
    push .ax
    ; [peephole-opt]     lda $2A
    ; [peephole-opt]     ldx $2B
    push .ax
    jsr _printf
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ; [peephole-opt]     ldx __zp_scratch4+1
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 18
    endproc

; function _test_blue_field_access
; SAC inline storage: 18 bytes
    _test_blue_field_access__local_0: .word 0
    _test_blue_field_access__local_79: .word 0
    _test_blue_field_access__local_90: .word 0
    proc _test_blue_field_access
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "src/test-resources/test_phase95_field_striped_rgb.c", 38
    .local @_l_blue_sum = 0
    .local @_l_x = 16
; .debug_var: __test_blue_field_access @_l_blue_sum offset=0 size=2 type=int16 scope=local
; .debug_var: __test_blue_field_access @_l_x offset=16 size=2 type=int16 scope=local

@entry:
    .loc "src/test-resources/test_phase95_field_striped_rgb.c", 59
    lda #0
    sta _test_blue_field_access__local_0
    sta _test_blue_field_access__local_0+1
@for_cond66_ph:
    lda #0
    tax
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond66:
    lda $24
    ldx $25
    cmp.16 .AX, #8
    bcc @for_body67
    bra @for_cond70_ph
@for_body67:
    lda #0
    sta $28
    sta $29
    lda $24
    ldx $25
    mul.16 .AX, #3
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
    sta $2A
    stx $2A+1
    lda $2A
    ldx $2B
    add.16 .AX, #2
    sta $2C
    stx $2D
    ldy #0
    lda ($2C),y
    ldx #0
    sta $2E
    ; [peephole-opt]     lda $2E
    ldx #0
    ldx #0
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    sta _test_blue_field_access__local_0
    stx _test_blue_field_access__local_0+1
@for_inc68:
    lda $24
    ldx $25
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    clc
    adc #1
    sta $24
    lda $33
    adc #0
    sta $25
    bra @for_cond66
@for_cond70_ph:
    lda #1
    ldx #0
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond70:
    lda $24
    ldx $25
    cmp.16 .AX, #8
    bcc @for_body71
    bra @for_cond74_ph
@for_body71:
    lda #1
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #3
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
    sta $2A
    stx $2A+1
    lda $2A
    ldx $2B
    add.16 .AX, #2
    sta $2C
    stx $2D
    ldy #0
    lda ($2C),y
    ldx #0
    sta $2E
    ; [peephole-opt]     lda $2E
    ldx #0
    ldx #0
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    sta _test_blue_field_access__local_0
    stx _test_blue_field_access__local_0+1
@for_inc72:
    lda $24
    ldx $25
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    clc
    adc #1
    sta $24
    lda $33
    adc #0
    sta $25
    bra @for_cond70
@for_cond74_ph:
    lda #2
    ldx #0
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond74:
    lda $24
    ldx $25
    cmp.16 .AX, #8
    bcc @for_body75
    bra @for_cond78_ph
@for_body75:
    lda #2
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #3
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
    sta $2A
    stx $2A+1
    lda $2A
    ldx $2B
    add.16 .AX, #2
    sta $2C
    stx $2D
    ldy #0
    lda ($2C),y
    ldx #0
    sta $2E
    ; [peephole-opt]     lda $2E
    ldx #0
    ldx #0
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    sta _test_blue_field_access__local_0
    stx _test_blue_field_access__local_0+1
@for_inc76:
    lda $24
    ldx $25
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    clc
    adc #1
    sta $24
    lda $33
    adc #0
    sta $25
    bra @for_cond74
@for_cond78_ph:
    lda #3
    ldx #0
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond78:
    lda $24
    ldx $25
    cmp.16 .AX, #8
    bcc @for_body79
    bra @for_cond82_ph
@for_body79:
    lda #3
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #3
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
    sta $2A
    stx $2A+1
    lda $2A
    ldx $2B
    add.16 .AX, #2
    sta $2C
    stx $2D
    ldy #0
    lda ($2C),y
    ldx #0
    sta $2E
    ; [peephole-opt]     lda $2E
    ldx #0
    ldx #0
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    sta _test_blue_field_access__local_0
    stx _test_blue_field_access__local_0+1
@for_inc80:
    lda $24
    ldx $25
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    clc
    adc #1
    sta $24
    lda $33
    adc #0
    sta $25
    bra @for_cond78
@for_cond82_ph:
    lda #4
    ldx #0
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond82:
    lda $24
    ldx $25
    cmp.16 .AX, #8
    bcc @for_body83
    bra @for_cond86_ph
@for_body83:
    lda #4
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #3
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
    sta $2A
    stx $2A+1
    lda $2A
    ldx $2B
    add.16 .AX, #2
    sta $2C
    stx $2D
    ldy #0
    lda ($2C),y
    ldx #0
    sta $2E
    ; [peephole-opt]     lda $2E
    ldx #0
    ldx #0
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    sta _test_blue_field_access__local_0
    stx _test_blue_field_access__local_0+1
@for_inc84:
    lda $24
    ldx $25
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    clc
    adc #1
    sta $24
    lda $33
    adc #0
    sta $25
    bra @for_cond82
@for_cond86_ph:
    lda #5
    ldx #0
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond86:
    lda $24
    ldx $25
    cmp.16 .AX, #8
    bcc @for_body87
    bra @for_cond90_ph
@for_body87:
    lda #5
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #3
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
    sta $2A
    stx $2A+1
    lda $2A
    ldx $2B
    add.16 .AX, #2
    sta $2C
    stx $2D
    ldy #0
    lda ($2C),y
    ldx #0
    sta $2E
    ; [peephole-opt]     lda $2E
    ldx #0
    ldx #0
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    sta _test_blue_field_access__local_0
    stx _test_blue_field_access__local_0+1
@for_inc88:
    lda $24
    ldx $25
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    clc
    adc #1
    sta $24
    lda $33
    adc #0
    sta $25
    bra @for_cond86
@for_cond90_ph:
    lda #6
    ldx #0
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond90:
    lda $24
    ldx $25
    cmp.16 .AX, #8
    bcc @for_body91
    bra @for_cond94_ph
@for_body91:
    lda #6
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #3
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
    sta $2A
    stx $2A+1
    lda $2A
    ldx $2B
    add.16 .AX, #2
    sta $2C
    stx $2D
    ldy #0
    lda ($2C),y
    ldx #0
    sta $2E
    ; [peephole-opt]     lda $2E
    ldx #0
    ldx #0
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    sta _test_blue_field_access__local_0
    stx _test_blue_field_access__local_0+1
@for_inc92:
    lda $24
    ldx $25
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    clc
    adc #1
    sta $24
    lda $33
    adc #0
    sta $25
    bra @for_cond90
@for_cond94_ph:
    lda #7
    ldx #0
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond94:
    lda _test_blue_field_access__local_79
    ldx _test_blue_field_access__local_79+1
    cmp.16 .AX, #8
    bcc @for_body95
    bra @for_end97
@for_body95:
    lda #7
    ldx #0
    sta $26
    stx $27
    lda _test_blue_field_access__local_79
    ldx _test_blue_field_access__local_79+1
    mul.16 .AX, #3
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
    sta $28
    stx $28+1
    lda $28
    ldx $29
    add.16 .AX, #2
    sta $2A
    stx $2B
    ldy #0
    lda ($2A),y
    ldx #0
    sta $2C
    ; [peephole-opt]     lda $2C
    ldx #0
    ldx #0
    sta $2E
    stx $2F
    ; [peephole-opt]     lda $2E
    ; [peephole-opt]     ldx $2F
    sta _test_blue_field_access__local_0
    stx _test_blue_field_access__local_0+1
@for_inc96:
    lda _test_blue_field_access__local_79
    ldx _test_blue_field_access__local_79+1
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    add.16 .AX, #1
    sta _test_blue_field_access__local_79
    stx _test_blue_field_access__local_79+1
    bra @for_cond94
@for_end97:
    .loc "src/test-resources/test_phase95_field_striped_rgb.c", 65
    ldax #__str_98
    sta _test_blue_field_access__local_90
    stx _test_blue_field_access__local_90+1
    lda _test_blue_field_access__local_0
    ldx _test_blue_field_access__local_0+1
    sta $28
    stx $29
    lda _test_blue_field_access__local_90
    ; [peephole-opt]     ldx _test_blue_field_access__local_90+1
    sta $2A
    stx $2B
    ; [peephole-opt]     lda $28
    ; [peephole-opt]     ldx $29
    push .ax
    ; [peephole-opt]     lda $2A
    ; [peephole-opt]     ldx $2B
    push .ax
    jsr _printf
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ; [peephole-opt]     ldx __zp_scratch4+1
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 18
    endproc

; function _test_mixed_field_access
; SAC inline storage: 34 bytes
    _test_mixed_field_access__local_0: .word 0
    _test_mixed_field_access__local_44: .word 0
    _test_mixed_field_access__local_47: .word 0
    _test_mixed_field_access__local_50: .word 0
    proc _test_mixed_field_access
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "src/test-resources/test_phase95_field_striped_rgb.c", 49
    .local @_l_brightness = 32
    .local @_l_brightness_sum = 0
    .local @_l_x = 30
; .debug_var: __test_mixed_field_access @_l_brightness offset=32 size=2 type=int16 scope=local
; .debug_var: __test_mixed_field_access @_l_brightness_sum offset=0 size=2 type=int16 scope=local
; .debug_var: __test_mixed_field_access @_l_x offset=30 size=2 type=int16 scope=local

@entry:
    .loc "src/test-resources/test_phase95_field_striped_rgb.c", 70
    lda #0
    sta _test_mixed_field_access__local_0
    sta _test_mixed_field_access__local_0+1
@for_cond99:
    lda $22
    ldx $23
    cmp.16 .AX, #8
    bcc @for_body100
    bra @for_cond103
@for_body100:
    lda $26
    ldx $27
    sta _test_mixed_field_access__local_0
    stx _test_mixed_field_access__local_0+1
@for_inc101:
    lda $22
    ldx $23
    sta $28
    stx $29
    ; [peephole-opt]     lda $28
    clc
    adc #1
    sta $22
    lda $29
    adc #0
    sta $23
    bra @for_cond99
@for_cond103:
    lda $22
    ldx $23
    cmp.16 .AX, #8
    bcc @for_body104
    bra @for_cond107
@for_body104:
    lda $26
    ldx $27
    sta _test_mixed_field_access__local_0
    stx _test_mixed_field_access__local_0+1
@for_inc105:
    lda $22
    ldx $23
    sta $28
    stx $29
    ; [peephole-opt]     lda $28
    clc
    adc #1
    sta $22
    lda $29
    adc #0
    sta $23
    bra @for_cond103
@for_cond107:
    lda $22
    ldx $23
    cmp.16 .AX, #8
    bcc @for_body108
    bra @for_cond111
@for_body108:
    lda $26
    ldx $27
    sta _test_mixed_field_access__local_0
    stx _test_mixed_field_access__local_0+1
@for_inc109:
    lda $22
    ldx $23
    sta $28
    stx $29
    ; [peephole-opt]     lda $28
    clc
    adc #1
    sta $22
    lda $29
    adc #0
    sta $23
    bra @for_cond107
@for_cond111:
    lda $22
    ldx $23
    cmp.16 .AX, #8
    bcc @for_body112
    bra @for_cond115
@for_body112:
    lda $26
    ldx $27
    sta _test_mixed_field_access__local_0
    stx _test_mixed_field_access__local_0+1
@for_inc113:
    lda $22
    ldx $23
    sta $28
    stx $29
    ; [peephole-opt]     lda $28
    clc
    adc #1
    sta $22
    lda $29
    adc #0
    sta $23
    bra @for_cond111
@for_cond115:
    lda $22
    ldx $23
    cmp.16 .AX, #8
    bcc @for_body116
    bra @for_cond119
@for_body116:
    lda $26
    ldx $27
    sta _test_mixed_field_access__local_0
    stx _test_mixed_field_access__local_0+1
@for_inc117:
    lda $22
    ldx $23
    sta $28
    stx $29
    ; [peephole-opt]     lda $28
    clc
    adc #1
    sta $22
    lda $29
    adc #0
    sta $23
    bra @for_cond115
@for_cond119:
    lda $22
    ldx $23
    cmp.16 .AX, #8
    bcc @for_body120
    bra @for_cond123
@for_body120:
    lda $26
    ldx $27
    sta _test_mixed_field_access__local_0
    stx _test_mixed_field_access__local_0+1
@for_inc121:
    lda $22
    ldx $23
    sta $28
    stx $29
    ; [peephole-opt]     lda $28
    clc
    adc #1
    sta $22
    lda $29
    adc #0
    sta $23
    bra @for_cond119
@for_cond123:
    lda $22
    ldx $23
    cmp.16 .AX, #8
    bcc @for_body124
    bra @for_cond127
@for_body124:
    lda $26
    ldx $27
    sta _test_mixed_field_access__local_0
    stx _test_mixed_field_access__local_0+1
@for_inc125:
    lda $22
    ldx $23
    sta $28
    stx $29
    ; [peephole-opt]     lda $28
    clc
    adc #1
    sta $22
    lda $29
    adc #0
    sta $23
    bra @for_cond123
@for_cond127:
    lda _test_mixed_field_access__local_44
    ldx _test_mixed_field_access__local_44+1
    cmp.16 .AX, #8
    bcc @for_body128
    bra @for_end130
@for_body128:
    lda _test_mixed_field_access__local_47
    ldx _test_mixed_field_access__local_47+1
    sta _test_mixed_field_access__local_0
    stx _test_mixed_field_access__local_0+1
@for_inc129:
    lda _test_mixed_field_access__local_44
    ldx _test_mixed_field_access__local_44+1
    sta $24
    stx $25
    ; [peephole-opt]     lda $24
    ; [peephole-opt]     ldx $25
    add.16 .AX, #1
    sta _test_mixed_field_access__local_44
    stx _test_mixed_field_access__local_44+1
    bra @for_cond127
@for_end130:
    .loc "src/test-resources/test_phase95_field_striped_rgb.c", 77
    ldax #__str_131
    sta _test_mixed_field_access__local_50
    stx _test_mixed_field_access__local_50+1
    lda _test_mixed_field_access__local_0
    ldx _test_mixed_field_access__local_0+1
    sta $28
    stx $29
    lda _test_mixed_field_access__local_50
    ; [peephole-opt]     ldx _test_mixed_field_access__local_50+1
    sta $2A
    stx $2B
    ; [peephole-opt]     lda $28
    ; [peephole-opt]     ldx $29
    push .ax
    ; [peephole-opt]     lda $2A
    ; [peephole-opt]     ldx $2B
    push .ax
    jsr _printf
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ; [peephole-opt]     ldx __zp_scratch4+1
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 34
    endproc

; function _test_single_pixel_access
; SAC inline storage: 6 bytes
    _test_single_pixel_access__local_0: .word 0
    _test_single_pixel_access__local_7: .word 0
    _test_single_pixel_access__local_14: .word 0
    _test_single_pixel_access__local_21: .word 0
    proc _test_single_pixel_access
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "src/test-resources/test_phase95_field_striped_rgb.c", 61
    .local @_l_b = 4
    .local @_l_g = 2
    .local @_l_r = 0
; .debug_var: __test_single_pixel_access @_l_b offset=4 size=2 type=int8 scope=local
; .debug_var: __test_single_pixel_access @_l_g offset=2 size=2 type=int8 scope=local
; .debug_var: __test_single_pixel_access @_l_r offset=0 size=2 type=int8 scope=local

@entry:
    .loc "src/test-resources/test_phase95_field_striped_rgb.c", 82
    lda #3
    ldx #0
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    lda #4
    ldx #0
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda .AX
    ldx .AX+1
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
    sta _test_single_pixel_access__local_0
    .loc "src/test-resources/test_phase95_field_striped_rgb.c", 83
    lda #3
    ldx #0
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    lda #4
    ldx #0
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda .AX
    ldx .AX+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    add.16 .AX, #1
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta _test_single_pixel_access__local_7
    .loc "src/test-resources/test_phase95_field_striped_rgb.c", 84
    lda #3
    ldx #0
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    lda #4
    ldx #0
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda .AX
    ldx .AX+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    add.16 .AX, #2
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta _test_single_pixel_access__local_14
    .loc "src/test-resources/test_phase95_field_striped_rgb.c", 85
    ldax #__str_132
    sta _test_single_pixel_access__local_21
    stx _test_single_pixel_access__local_21+1
    ; [peephole-opt]     lda _test_single_pixel_access__local_14
    sta $28
    stx $29
    lda _test_single_pixel_access__local_7
    sta $2A
    stx $2B
    lda _test_single_pixel_access__local_0
    sta $2C
    stx $2D
    lda _test_single_pixel_access__local_21
    ; [peephole-opt]     ldx _test_single_pixel_access__local_21+1
    sta $2E
    stx $2F
    ; [peephole-opt]     lda $28
    ; [peephole-opt]     ldx $29
    push .ax
    ; [peephole-opt]     lda $2A
    ; [peephole-opt]     ldx $2B
    push .ax
    ; [peephole-opt]     lda $2C
    ; [peephole-opt]     ldx $2D
    push .ax
    ; [peephole-opt]     lda $2E
    ; [peephole-opt]     ldx $2F
    push .ax
    jsr _printf
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    plz
    lda __zp_scratch4
    ; [peephole-opt]     ldx __zp_scratch4+1
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _main
; SAC inline storage: 94 bytes
    _main__local_0: .word 0
    _main__local_2: .word 0
    _main__local_92: .word 0
    _main__local_94: .word 0
    _main__local_184: .word 0
    _main__local_186: .word 0
    _main__local_276: .word 0
    _main__local_278: .word 0
    _main__local_322: .word 0
    _main__local_325: .word 0
    _main__local_328: .word 0
    _main__local_330: .word 0
    _main__local_331: .word 0
    _main__local_332: .word 0
    _main__local_333: .word 0
    _main__local_334: .word 0
    _main__local_335: .word 0
    _main__local_336: .word 0
    _main__local_337: .word 0
    _main__local_338: .word 0
    _main__local_339: .word 0
    _main__local_340: .word 0
    _main__local_341: .word 0
    _main__local_342: .word 0
    _main__local_343: .word 0
    _main__local_344: .word 0
    _main__local_345: .word 0
    _main__local_346: .word 0
    _main__local_347: .word 0
    _main__local_348: .word 0
    _main__local_349: .word 0
    _main__local_350: .word 0
    _main__local_351: .word 0
    _main__local_352: .word 0
    _main__local_353: .word 0
    _main__local_354: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "src/test-resources/test_phase95_field_striped_rgb.c", 69
    .local @_l_b = 92
    .local @_l_blue_sum = 36
    .local @_l_brightness = 86
    .local @_l_brightness_sum = 54
    .local @_l_g = 90
    .local @_l_green_sum = 18
    .local @_l_r = 88
    .local @_l_red_sum = 0
    .local @_l_x = 84
; .debug_var: __main @_l_b offset=92 size=2 type=int8 scope=local
; .debug_var: __main @_l_blue_sum offset=36 size=2 type=int16 scope=local
; .debug_var: __main @_l_brightness offset=86 size=2 type=int16 scope=local
; .debug_var: __main @_l_brightness_sum offset=54 size=2 type=int16 scope=local
; .debug_var: __main @_l_g offset=90 size=2 type=int8 scope=local
; .debug_var: __main @_l_green_sum offset=18 size=2 type=int16 scope=local
; .debug_var: __main @_l_r offset=88 size=2 type=int8 scope=local
; .debug_var: __main @_l_red_sum offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_x offset=84 size=2 type=int16 scope=local

@entry:
    .loc "src/test-resources/test_phase95_field_striped_rgb.c", 89
    ldax #__str_133
    sta _main__local_0
    stx _main__local_0+1
    lda _main__local_0
    ; [peephole-opt]     ldx _main__local_0+1
    sta $28
    stx $29
    ; [peephole-opt]     lda $28
    ; [peephole-opt]     ldx $29
    push .ax
    jsr _printf
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ; [peephole-opt]     ldx __zp_scratch4+1
    .loc "src/test-resources/test_phase95_field_striped_rgb.c", 37
    lda #0
    sta _main__local_2
    sta _main__local_2+1
@for_cond135_ph:
    lda #0
    tax
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond135:
    lda $24
    ldx $25
    cmp.16 .AX, #8
    bcc @for_body136
    bra @for_cond139_ph
@for_body136:
    lda #0
    sta $28
    sta $29
    lda $24
    ldx $25
    mul.16 .AX, #3
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
    sta $2A
    stx $2A+1
    lda $2A
    ldx $2B
    sta $2C
    stx $2D
    ldy #0
    lda ($2A),y
    ldx #0
    sta $2E
    ; [peephole-opt]     lda $2E
    ldx #0
    ldx #0
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    sta _main__local_2
    stx _main__local_2+1
@for_inc137:
    lda $24
    ldx $25
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    clc
    adc #1
    sta $24
    lda $33
    adc #0
    sta $25
    bra @for_cond135
@for_cond139_ph:
    lda #1
    ldx #0
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond139:
    lda $24
    ldx $25
    cmp.16 .AX, #8
    bcc @for_body140
    bra @for_cond143_ph
@for_body140:
    lda #1
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #3
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
    sta $2A
    stx $2A+1
    lda $2A
    ldx $2B
    sta $2C
    stx $2D
    ldy #0
    lda ($2A),y
    ldx #0
    sta $2E
    ; [peephole-opt]     lda $2E
    ldx #0
    ldx #0
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    sta _main__local_2
    stx _main__local_2+1
@for_inc141:
    lda $24
    ldx $25
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    clc
    adc #1
    sta $24
    lda $33
    adc #0
    sta $25
    bra @for_cond139
@for_cond143_ph:
    lda #2
    ldx #0
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond143:
    lda $24
    ldx $25
    cmp.16 .AX, #8
    bcc @for_body144
    bra @for_cond147_ph
@for_body144:
    lda #2
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #3
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
    sta $2A
    stx $2A+1
    lda $2A
    ldx $2B
    sta $2C
    stx $2D
    ldy #0
    lda ($2A),y
    ldx #0
    sta $2E
    ; [peephole-opt]     lda $2E
    ldx #0
    ldx #0
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    sta _main__local_2
    stx _main__local_2+1
@for_inc145:
    lda $24
    ldx $25
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    clc
    adc #1
    sta $24
    lda $33
    adc #0
    sta $25
    bra @for_cond143
@for_cond147_ph:
    lda #3
    ldx #0
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond147:
    lda $24
    ldx $25
    cmp.16 .AX, #8
    bcc @for_body148
    bra @for_cond151_ph
@for_body148:
    lda #3
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #3
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
    sta $2A
    stx $2A+1
    lda $2A
    ldx $2B
    sta $2C
    stx $2D
    ldy #0
    lda ($2A),y
    ldx #0
    sta $2E
    ; [peephole-opt]     lda $2E
    ldx #0
    ldx #0
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    sta _main__local_2
    stx _main__local_2+1
@for_inc149:
    lda $24
    ldx $25
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    clc
    adc #1
    sta $24
    lda $33
    adc #0
    sta $25
    bra @for_cond147
@for_cond151_ph:
    lda #4
    ldx #0
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond151:
    lda $24
    ldx $25
    cmp.16 .AX, #8
    bcc @for_body152
    bra @for_cond155_ph
@for_body152:
    lda #4
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #3
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
    sta $2A
    stx $2A+1
    lda $2A
    ldx $2B
    sta $2C
    stx $2D
    ldy #0
    lda ($2A),y
    ldx #0
    sta $2E
    ; [peephole-opt]     lda $2E
    ldx #0
    ldx #0
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    sta _main__local_2
    stx _main__local_2+1
@for_inc153:
    lda $24
    ldx $25
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    clc
    adc #1
    sta $24
    lda $33
    adc #0
    sta $25
    bra @for_cond151
@for_cond155_ph:
    lda #5
    ldx #0
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond155:
    lda $24
    ldx $25
    cmp.16 .AX, #8
    bcc @for_body156
    bra @for_cond159_ph
@for_body156:
    lda #5
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #3
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
    sta $2A
    stx $2A+1
    lda $2A
    ldx $2B
    sta $2C
    stx $2D
    ldy #0
    lda ($2A),y
    ldx #0
    sta $2E
    ; [peephole-opt]     lda $2E
    ldx #0
    ldx #0
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    sta _main__local_2
    stx _main__local_2+1
@for_inc157:
    lda $24
    ldx $25
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    clc
    adc #1
    sta $24
    lda $33
    adc #0
    sta $25
    bra @for_cond155
@for_cond159_ph:
    lda #6
    ldx #0
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond159:
    lda $24
    ldx $25
    cmp.16 .AX, #8
    bcc @for_body160
    bra @for_cond163_ph
@for_body160:
    lda #6
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #3
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
    sta $2A
    stx $2A+1
    lda $2A
    ldx $2B
    sta $2C
    stx $2D
    ldy #0
    lda ($2A),y
    ldx #0
    sta $2E
    ; [peephole-opt]     lda $2E
    ldx #0
    ldx #0
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    sta _main__local_2
    stx _main__local_2+1
@for_inc161:
    lda $24
    ldx $25
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    clc
    adc #1
    sta $24
    lda $33
    adc #0
    sta $25
    bra @for_cond159
@for_cond163_ph:
    lda #7
    ldx #0
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond163:
    lda $24
    ldx $25
    cmp.16 .AX, #8
    bcc @for_body164
    bra @for_end166
@for_body164:
    lda #7
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #3
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
    sta $2A
    stx $2A+1
    lda $2A
    ldx $2B
    sta $2C
    stx $2D
    ldy #0
    lda ($2A),y
    ldx #0
    sta $2E
    ; [peephole-opt]     lda $2E
    ldx #0
    ldx #0
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    sta _main__local_2
    stx _main__local_2+1
@for_inc165:
    lda $24
    ldx $25
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    clc
    adc #1
    sta $24
    lda $33
    adc #0
    sta $25
    bra @for_cond163
@for_end166:
    .loc "src/test-resources/test_phase95_field_striped_rgb.c", 43
    ldax #__str_167
    sta _main__local_92
    stx _main__local_92+1
    lda _main__local_2
    ldx _main__local_2+1
    sta $28
    stx $29
    lda _main__local_92
    ; [peephole-opt]     ldx _main__local_92+1
    sta $2A
    stx $2B
    ; [peephole-opt]     lda $28
    ; [peephole-opt]     ldx $29
    push .ax
    ; [peephole-opt]     lda $2A
    ; [peephole-opt]     ldx $2B
    push .ax
    jsr _printf
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ; [peephole-opt]     ldx __zp_scratch4+1
@inline_end134:
    .loc "src/test-resources/test_phase95_field_striped_rgb.c", 48
    lda #0
    sta _main__local_94
    sta _main__local_94+1
@for_cond169_ph:
    lda #0
    tax
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond169:
    lda $24
    ldx $25
    cmp.16 .AX, #8
    bcc @for_body170
    bra @for_cond173_ph
@for_body170:
    lda #0
    sta $28
    sta $29
    lda $24
    ldx $25
    mul.16 .AX, #3
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
    sta $2A
    stx $2A+1
    lda $2A
    ldx $2B
    add.16 .AX, #1
    sta $2C
    stx $2D
    ldy #0
    lda ($2C),y
    ldx #0
    sta $2E
    ; [peephole-opt]     lda $2E
    ldx #0
    ldx #0
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    sta _main__local_94
    stx _main__local_94+1
@for_inc171:
    lda $24
    ldx $25
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    clc
    adc #1
    sta $24
    lda $33
    adc #0
    sta $25
    bra @for_cond169
@for_cond173_ph:
    lda #1
    ldx #0
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond173:
    lda $24
    ldx $25
    cmp.16 .AX, #8
    bcc @for_body174
    bra @for_cond177_ph
@for_body174:
    lda #1
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #3
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
    sta $2A
    stx $2A+1
    lda $2A
    ldx $2B
    add.16 .AX, #1
    sta $2C
    stx $2D
    ldy #0
    lda ($2C),y
    ldx #0
    sta $2E
    ; [peephole-opt]     lda $2E
    ldx #0
    ldx #0
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    sta _main__local_94
    stx _main__local_94+1
@for_inc175:
    lda $24
    ldx $25
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    clc
    adc #1
    sta $24
    lda $33
    adc #0
    sta $25
    bra @for_cond173
@for_cond177_ph:
    lda #2
    ldx #0
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond177:
    lda $24
    ldx $25
    cmp.16 .AX, #8
    bcc @for_body178
    bra @for_cond181_ph
@for_body178:
    lda #2
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #3
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
    sta $2A
    stx $2A+1
    lda $2A
    ldx $2B
    add.16 .AX, #1
    sta $2C
    stx $2D
    ldy #0
    lda ($2C),y
    ldx #0
    sta $2E
    ; [peephole-opt]     lda $2E
    ldx #0
    ldx #0
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    sta _main__local_94
    stx _main__local_94+1
@for_inc179:
    lda $24
    ldx $25
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    clc
    adc #1
    sta $24
    lda $33
    adc #0
    sta $25
    bra @for_cond177
@for_cond181_ph:
    lda #3
    ldx #0
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond181:
    lda $24
    ldx $25
    cmp.16 .AX, #8
    bcc @for_body182
    bra @for_cond185_ph
@for_body182:
    lda #3
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #3
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
    sta $2A
    stx $2A+1
    lda $2A
    ldx $2B
    add.16 .AX, #1
    sta $2C
    stx $2D
    ldy #0
    lda ($2C),y
    ldx #0
    sta $2E
    ; [peephole-opt]     lda $2E
    ldx #0
    ldx #0
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    sta _main__local_94
    stx _main__local_94+1
@for_inc183:
    lda $24
    ldx $25
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    clc
    adc #1
    sta $24
    lda $33
    adc #0
    sta $25
    bra @for_cond181
@for_cond185_ph:
    lda #4
    ldx #0
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond185:
    lda $24
    ldx $25
    cmp.16 .AX, #8
    bcc @for_body186
    bra @for_cond189_ph
@for_body186:
    lda #4
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #3
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
    sta $2A
    stx $2A+1
    lda $2A
    ldx $2B
    add.16 .AX, #1
    sta $2C
    stx $2D
    ldy #0
    lda ($2C),y
    ldx #0
    sta $2E
    ; [peephole-opt]     lda $2E
    ldx #0
    ldx #0
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    sta _main__local_94
    stx _main__local_94+1
@for_inc187:
    lda $24
    ldx $25
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    clc
    adc #1
    sta $24
    lda $33
    adc #0
    sta $25
    bra @for_cond185
@for_cond189_ph:
    lda #5
    ldx #0
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond189:
    lda $24
    ldx $25
    cmp.16 .AX, #8
    bcc @for_body190
    bra @for_cond193_ph
@for_body190:
    lda #5
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #3
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
    sta $2A
    stx $2A+1
    lda $2A
    ldx $2B
    add.16 .AX, #1
    sta $2C
    stx $2D
    ldy #0
    lda ($2C),y
    ldx #0
    sta $2E
    ; [peephole-opt]     lda $2E
    ldx #0
    ldx #0
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    sta _main__local_94
    stx _main__local_94+1
@for_inc191:
    lda $24
    ldx $25
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    clc
    adc #1
    sta $24
    lda $33
    adc #0
    sta $25
    bra @for_cond189
@for_cond193_ph:
    lda #6
    ldx #0
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond193:
    lda $24
    ldx $25
    cmp.16 .AX, #8
    bcc @for_body194
    bra @for_cond197_ph
@for_body194:
    lda #6
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #3
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
    sta $2A
    stx $2A+1
    lda $2A
    ldx $2B
    add.16 .AX, #1
    sta $2C
    stx $2D
    ldy #0
    lda ($2C),y
    ldx #0
    sta $2E
    ; [peephole-opt]     lda $2E
    ldx #0
    ldx #0
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    sta _main__local_94
    stx _main__local_94+1
@for_inc195:
    lda $24
    ldx $25
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    clc
    adc #1
    sta $24
    lda $33
    adc #0
    sta $25
    bra @for_cond193
@for_cond197_ph:
    lda #7
    ldx #0
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond197:
    lda $24
    ldx $25
    cmp.16 .AX, #8
    bcc @for_body198
    bra @for_end200
@for_body198:
    lda #7
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #3
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
    sta $2A
    stx $2A+1
    lda $2A
    ldx $2B
    add.16 .AX, #1
    sta $2C
    stx $2D
    ldy #0
    lda ($2C),y
    ldx #0
    sta $2E
    ; [peephole-opt]     lda $2E
    ldx #0
    ldx #0
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    sta _main__local_94
    stx _main__local_94+1
@for_inc199:
    lda $24
    ldx $25
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    clc
    adc #1
    sta $24
    lda $33
    adc #0
    sta $25
    bra @for_cond197
@for_end200:
    .loc "src/test-resources/test_phase95_field_striped_rgb.c", 54
    ldax #__str_201
    sta _main__local_184
    stx _main__local_184+1
    lda _main__local_94
    ldx _main__local_94+1
    sta $28
    stx $29
    lda _main__local_184
    ; [peephole-opt]     ldx _main__local_184+1
    sta $2A
    stx $2B
    ; [peephole-opt]     lda $28
    ; [peephole-opt]     ldx $29
    push .ax
    ; [peephole-opt]     lda $2A
    ; [peephole-opt]     ldx $2B
    push .ax
    jsr _printf
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ; [peephole-opt]     ldx __zp_scratch4+1
@inline_end168:
    .loc "src/test-resources/test_phase95_field_striped_rgb.c", 59
    lda #0
    sta _main__local_186
    sta _main__local_186+1
@for_cond203_ph:
    lda #0
    tax
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond203:
    lda $24
    ldx $25
    cmp.16 .AX, #8
    bcc @for_body204
    bra @for_cond207_ph
@for_body204:
    lda #0
    sta $28
    sta $29
    lda $24
    ldx $25
    mul.16 .AX, #3
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
    sta $2A
    stx $2A+1
    lda $2A
    ldx $2B
    add.16 .AX, #2
    sta $2C
    stx $2D
    ldy #0
    lda ($2C),y
    ldx #0
    sta $2E
    ; [peephole-opt]     lda $2E
    ldx #0
    ldx #0
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    sta _main__local_186
    stx _main__local_186+1
@for_inc205:
    lda $24
    ldx $25
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    clc
    adc #1
    sta $24
    lda $33
    adc #0
    sta $25
    bra @for_cond203
@for_cond207_ph:
    lda #1
    ldx #0
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond207:
    lda $24
    ldx $25
    cmp.16 .AX, #8
    bcc @for_body208
    bra @for_cond211_ph
@for_body208:
    lda #1
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #3
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
    sta $2A
    stx $2A+1
    lda $2A
    ldx $2B
    add.16 .AX, #2
    sta $2C
    stx $2D
    ldy #0
    lda ($2C),y
    ldx #0
    sta $2E
    ; [peephole-opt]     lda $2E
    ldx #0
    ldx #0
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    sta _main__local_186
    stx _main__local_186+1
@for_inc209:
    lda $24
    ldx $25
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    clc
    adc #1
    sta $24
    lda $33
    adc #0
    sta $25
    bra @for_cond207
@for_cond211_ph:
    lda #2
    ldx #0
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond211:
    lda $24
    ldx $25
    cmp.16 .AX, #8
    bcc @for_body212
    bra @for_cond215_ph
@for_body212:
    lda #2
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #3
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
    sta $2A
    stx $2A+1
    lda $2A
    ldx $2B
    add.16 .AX, #2
    sta $2C
    stx $2D
    ldy #0
    lda ($2C),y
    ldx #0
    sta $2E
    ; [peephole-opt]     lda $2E
    ldx #0
    ldx #0
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    sta _main__local_186
    stx _main__local_186+1
@for_inc213:
    lda $24
    ldx $25
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    clc
    adc #1
    sta $24
    lda $33
    adc #0
    sta $25
    bra @for_cond211
@for_cond215_ph:
    lda #3
    ldx #0
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond215:
    lda $24
    ldx $25
    cmp.16 .AX, #8
    bcc @for_body216
    bra @for_cond219_ph
@for_body216:
    lda #3
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #3
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
    sta $2A
    stx $2A+1
    lda $2A
    ldx $2B
    add.16 .AX, #2
    sta $2C
    stx $2D
    ldy #0
    lda ($2C),y
    ldx #0
    sta $2E
    ; [peephole-opt]     lda $2E
    ldx #0
    ldx #0
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    sta _main__local_186
    stx _main__local_186+1
@for_inc217:
    lda $24
    ldx $25
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    clc
    adc #1
    sta $24
    lda $33
    adc #0
    sta $25
    bra @for_cond215
@for_cond219_ph:
    lda #4
    ldx #0
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond219:
    lda $24
    ldx $25
    cmp.16 .AX, #8
    bcc @for_body220
    bra @for_cond223_ph
@for_body220:
    lda #4
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #3
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
    sta $2A
    stx $2A+1
    lda $2A
    ldx $2B
    add.16 .AX, #2
    sta $2C
    stx $2D
    ldy #0
    lda ($2C),y
    ldx #0
    sta $2E
    ; [peephole-opt]     lda $2E
    ldx #0
    ldx #0
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    sta _main__local_186
    stx _main__local_186+1
@for_inc221:
    lda $24
    ldx $25
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    clc
    adc #1
    sta $24
    lda $33
    adc #0
    sta $25
    bra @for_cond219
@for_cond223_ph:
    lda #5
    ldx #0
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond223:
    lda $24
    ldx $25
    cmp.16 .AX, #8
    bcc @for_body224
    bra @for_cond227_ph
@for_body224:
    lda #5
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #3
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
    sta $2A
    stx $2A+1
    lda $2A
    ldx $2B
    add.16 .AX, #2
    sta $2C
    stx $2D
    ldy #0
    lda ($2C),y
    ldx #0
    sta $2E
    ; [peephole-opt]     lda $2E
    ldx #0
    ldx #0
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    sta _main__local_186
    stx _main__local_186+1
@for_inc225:
    lda $24
    ldx $25
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    clc
    adc #1
    sta $24
    lda $33
    adc #0
    sta $25
    bra @for_cond223
@for_cond227_ph:
    lda #6
    ldx #0
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond227:
    lda $24
    ldx $25
    cmp.16 .AX, #8
    bcc @for_body228
    bra @for_cond231_ph
@for_body228:
    lda #6
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #3
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
    sta $2A
    stx $2A+1
    lda $2A
    ldx $2B
    add.16 .AX, #2
    sta $2C
    stx $2D
    ldy #0
    lda ($2C),y
    ldx #0
    sta $2E
    ; [peephole-opt]     lda $2E
    ldx #0
    ldx #0
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    sta _main__local_186
    stx _main__local_186+1
@for_inc229:
    lda $24
    ldx $25
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    clc
    adc #1
    sta $24
    lda $33
    adc #0
    sta $25
    bra @for_cond227
@for_cond231_ph:
    lda #7
    ldx #0
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_image
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond231:
    lda $24
    ldx $25
    cmp.16 .AX, #8
    bcc @for_body232
    bra @for_end234
@for_body232:
    lda #7
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #3
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
    sta $2A
    stx $2A+1
    lda $2A
    ldx $2B
    add.16 .AX, #2
    sta $2C
    stx $2D
    ldy #0
    lda ($2C),y
    ldx #0
    sta $2E
    ; [peephole-opt]     lda $2E
    ldx #0
    ldx #0
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    sta _main__local_186
    stx _main__local_186+1
@for_inc233:
    lda $24
    ldx $25
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    clc
    adc #1
    sta $24
    lda $33
    adc #0
    sta $25
    bra @for_cond231
@for_end234:
    .loc "src/test-resources/test_phase95_field_striped_rgb.c", 65
    ldax #__str_235
    sta _main__local_276
    stx _main__local_276+1
    lda _main__local_186
    ldx _main__local_186+1
    sta $28
    stx $29
    lda _main__local_276
    ; [peephole-opt]     ldx _main__local_276+1
    sta $2A
    stx $2B
    ; [peephole-opt]     lda $28
    ; [peephole-opt]     ldx $29
    push .ax
    ; [peephole-opt]     lda $2A
    ; [peephole-opt]     ldx $2B
    push .ax
    jsr _printf
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ; [peephole-opt]     ldx __zp_scratch4+1
@inline_end202:
    .loc "src/test-resources/test_phase95_field_striped_rgb.c", 70
    lda #0
    sta _main__local_278
    sta _main__local_278+1
@for_cond237:
    lda $22
    ldx $23
    cmp.16 .AX, #8
    bcc @for_body238
    bra @for_cond241
@for_body238:
    lda $26
    ldx $27
    sta _main__local_278
    stx _main__local_278+1
@for_inc239:
    lda $22
    ldx $23
    sta $28
    stx $29
    ; [peephole-opt]     lda $28
    clc
    adc #1
    sta $22
    lda $29
    adc #0
    sta $23
    bra @for_cond237
@for_cond241:
    lda $22
    ldx $23
    cmp.16 .AX, #8
    bcc @for_body242
    bra @for_cond245
@for_body242:
    lda $26
    ldx $27
    sta _main__local_278
    stx _main__local_278+1
@for_inc243:
    lda $22
    ldx $23
    sta $28
    stx $29
    ; [peephole-opt]     lda $28
    clc
    adc #1
    sta $22
    lda $29
    adc #0
    sta $23
    bra @for_cond241
@for_cond245:
    lda $22
    ldx $23
    cmp.16 .AX, #8
    bcc @for_body246
    bra @for_cond249
@for_body246:
    lda $26
    ldx $27
    sta _main__local_278
    stx _main__local_278+1
@for_inc247:
    lda $22
    ldx $23
    sta $28
    stx $29
    ; [peephole-opt]     lda $28
    clc
    adc #1
    sta $22
    lda $29
    adc #0
    sta $23
    bra @for_cond245
@for_cond249:
    lda $22
    ldx $23
    cmp.16 .AX, #8
    bcc @for_body250
    bra @for_cond253
@for_body250:
    lda $26
    ldx $27
    sta _main__local_278
    stx _main__local_278+1
@for_inc251:
    lda $22
    ldx $23
    sta $28
    stx $29
    ; [peephole-opt]     lda $28
    clc
    adc #1
    sta $22
    lda $29
    adc #0
    sta $23
    bra @for_cond249
@for_cond253:
    lda $22
    ldx $23
    cmp.16 .AX, #8
    bcc @for_body254
    bra @for_cond257
@for_body254:
    lda $26
    ldx $27
    sta _main__local_278
    stx _main__local_278+1
@for_inc255:
    lda $22
    ldx $23
    sta $28
    stx $29
    ; [peephole-opt]     lda $28
    clc
    adc #1
    sta $22
    lda $29
    adc #0
    sta $23
    bra @for_cond253
@for_cond257:
    lda $22
    ldx $23
    cmp.16 .AX, #8
    bcc @for_body258
    bra @for_cond261
@for_body258:
    lda $26
    ldx $27
    sta _main__local_278
    stx _main__local_278+1
@for_inc259:
    lda $22
    ldx $23
    sta $28
    stx $29
    ; [peephole-opt]     lda $28
    clc
    adc #1
    sta $22
    lda $29
    adc #0
    sta $23
    bra @for_cond257
@for_cond261:
    lda $22
    ldx $23
    cmp.16 .AX, #8
    bcc @for_body262
    bra @for_cond265
@for_body262:
    lda $26
    ldx $27
    sta _main__local_278
    stx _main__local_278+1
@for_inc263:
    lda $22
    ldx $23
    sta $28
    stx $29
    ; [peephole-opt]     lda $28
    clc
    adc #1
    sta $22
    lda $29
    adc #0
    sta $23
    bra @for_cond261
@for_cond265:
    lda _main__local_322
    ldx _main__local_322+1
    cmp.16 .AX, #8
    bcc @for_body266
    bra @for_end268
@for_body266:
    lda _main__local_325
    ldx _main__local_325+1
    sta _main__local_278
    stx _main__local_278+1
@for_inc267:
    lda _main__local_322
    ldx _main__local_322+1
    sta $24
    stx $25
    ; [peephole-opt]     lda $24
    ; [peephole-opt]     ldx $25
    add.16 .AX, #1
    sta _main__local_322
    stx _main__local_322+1
    bra @for_cond265
@for_end268:
    .loc "src/test-resources/test_phase95_field_striped_rgb.c", 77
    ldax #__str_269
    sta _main__local_328
    stx _main__local_328+1
    lda _main__local_278
    ldx _main__local_278+1
    sta $28
    stx $29
    lda _main__local_328
    ; [peephole-opt]     ldx _main__local_328+1
    sta $2A
    stx $2B
    ; [peephole-opt]     lda $28
    ; [peephole-opt]     ldx $29
    push .ax
    ; [peephole-opt]     lda $2A
    ; [peephole-opt]     ldx $2B
    push .ax
    jsr _printf
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ; [peephole-opt]     ldx __zp_scratch4+1
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 94
    endproc


    .segment "data"
__str_32:
    .text "Red sum: %d
"
    .byte 0
__str_65:
    .text "Green sum: %d
"
    .byte 0
__str_98:
    .text "Blue sum: %d
"
    .byte 0
__str_131:
    .text "Brightness sum: %d
"
    .byte 0
__str_132:
    .text "Pixel [3,4]: R=%d, G=%d, B=%d
"
    .byte 0
__str_133:
    .text "Phase 95.6: Field-striped RGB test
"
    .byte 0
__str_167:
    .text "Red sum: %d
"
    .byte 0
__str_201:
    .text "Green sum: %d
"
    .byte 0
__str_235:
    .text "Blue sum: %d
"
    .byte 0
__str_269:
    .text "Brightness sum: %d
"
    .byte 0
__str_271:
    .text "Pixel [3,4]: R=%d, G=%d, B=%d
"
    .byte 0
__str_272:
    .text "All tests completed
"
    .byte 0

__zp_save_buf:
; [DEBUG] Phase 87 code reached, optimize=true

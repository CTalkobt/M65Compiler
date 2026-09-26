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

    .global _verts
    .global _test_id_field_access
    .global _test_x_field_access
    .global _test_y_field_access
    .global _test_flags_field_access
    .global _test_sequential_field_access
    .global _main

    .segment "data"
    .byte 0
_verts:
; .debug_var: @global _verts offset=0 size=2 type=int16 scope=global
    .word 0
    .word 0
    .word 0
    .word 0
    .res 104

    .segment "code"

; function _test_id_field_access
; SAC inline storage: 4 bytes
    _test_id_field_access__local_0: .word 0
    _test_id_field_access__local_2: .word 0
    _test_id_field_access__local_4: .word 0
    _test_id_field_access__local_5: .word 0
    _test_id_field_access__local_6: .word 0
    _test_id_field_access__local_7: .word 0
    _test_id_field_access__local_8: .word 0
    _test_id_field_access__local_9: .word 0
    _test_id_field_access__local_10: .word 0
    _test_id_field_access__local_11: .word 0
    _test_id_field_access__local_12: .word 0
    _test_id_field_access__local_13: .word 0
    _test_id_field_access__local_14: .word 0
    _test_id_field_access__local_20: .word 0
    proc _test_id_field_access
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 5
    .local @_l_x = 2
    .local @_l_y = 0
; .debug_var: __test_id_field_access @_l_x offset=2 size=2 type=int16 scope=local
; .debug_var: __test_id_field_access @_l_y offset=0 size=2 type=int16 scope=local

@entry:
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 26
    ldax #__str_0
    sta _test_id_field_access__local_0
    stx _test_id_field_access__local_0+1
    lda _test_id_field_access__local_0
    ; [peephole-opt]     ldx _test_id_field_access__local_0+1
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
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 27
    lda #0
    sta _test_id_field_access__local_2
    sta _test_id_field_access__local_2+1
@for_cond1_ph:
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 29
    ldax #__str_9
    sta _test_id_field_access__local_10
    stx _test_id_field_access__local_10+1
@for_cond1:
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 27
    lda _test_id_field_access__local_2
    ldx _test_id_field_access__local_2+1
    cmp.16 .AX, #4
    bcc @for_body2
    bra @for_end4
@for_body2:
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 28
    lda #0
    sta _test_id_field_access__local_6
    sta _test_id_field_access__local_6+1
@for_cond5_ph:
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 29
    lda _test_id_field_access__local_2
    ldx _test_id_field_access__local_2+1
    mul.16 .AX, #28
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_verts
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta _test_id_field_access__local_11
    stx _test_id_field_access__local_11+1
@for_cond5:
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 28
    lda _test_id_field_access__local_6
    ldx _test_id_field_access__local_6+1
    cmp.16 .AX, #4
    bcc @for_body6
    bra @for_end8
@for_body6:
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 29
    lda _test_id_field_access__local_11
    ldx _test_id_field_access__local_11+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _test_id_field_access__local_6
    ldx _test_id_field_access__local_6+1
    mul.16 .AX, #7
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda __zp_scratch2
    ; [peephole-opt]     ldx __zp_scratch2+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta _test_id_field_access__local_12
    stx _test_id_field_access__local_12+1
    lda _test_id_field_access__local_12
    ; [peephole-opt]     ldx _test_id_field_access__local_12+1
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta _test_id_field_access__local_14
    ; [peephole-opt]     lda _test_id_field_access__local_14
    sta $28
    stx $29
    lda _test_id_field_access__local_10
    ldx _test_id_field_access__local_10+1
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
@for_inc7:
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 28
    lda _test_id_field_access__local_6
    ldx _test_id_field_access__local_6+1
    sta $22
    stx $23
    ; [peephole-opt]     lda $22
    clc
    adc #1
    sta $24
    lda $23
    adc #0
    sta $25
    ; [peephole-opt]     lda $24
    ldx $25
    sta _test_id_field_access__local_6
    stx _test_id_field_access__local_6+1
    bra @for_cond5
@for_end8:
@for_inc3:
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 27
    lda _test_id_field_access__local_2
    ldx _test_id_field_access__local_2+1
    sta $26
    stx $27
    ; [peephole-opt]     lda $26
    ; [peephole-opt]     ldx $27
    add.16 .AX, #1
    sta _test_id_field_access__local_2
    stx _test_id_field_access__local_2+1
    bra @for_cond1
@for_end4:
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 32
    ldax #__str_10
    sta _test_id_field_access__local_20
    stx _test_id_field_access__local_20+1
    lda _test_id_field_access__local_20
    ; [peephole-opt]     ldx _test_id_field_access__local_20+1
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
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _test_x_field_access
; SAC inline storage: 10 bytes
    _test_x_field_access__local_0: .word 0
    _test_x_field_access__local_32: .word 0
    _test_x_field_access__local_42: .word 0
    proc _test_x_field_access
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 16
    .local @_l_x = 8
    .local @_l_x_sum = 0
; .debug_var: __test_x_field_access @_l_x offset=8 size=2 type=int16 scope=local
; .debug_var: __test_x_field_access @_l_x_sum offset=0 size=2 type=int16 scope=local

@entry:
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 37
    lda #0
    sta _test_x_field_access__local_0
    sta _test_x_field_access__local_0+1
@for_cond11_ph:
    lda #0
    tax
    mul.16 .AX, #28
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_verts
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond11:
    lda $24
    ldx $25
    cmp.16 .AX, #4
    bcc @for_body12
    bra @for_cond15_ph
@for_body12:
    lda #0
    sta $28
    sta $29
    lda $24
    ldx $25
    mul.16 .AX, #7
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
    pha
    iny
    lda ($2C),y
    tax
    pla
    sta $2E
    stx $2F
    ; [peephole-opt]     lda $2E
    ; [peephole-opt]     ldx $2F
    sta _test_x_field_access__local_0
    stx _test_x_field_access__local_0+1
@for_inc13:
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
    bra @for_cond11
@for_cond15_ph:
    lda #1
    ldx #0
    mul.16 .AX, #28
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_verts
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond15:
    lda $24
    ldx $25
    cmp.16 .AX, #4
    bcc @for_body16
    bra @for_cond19_ph
@for_body16:
    lda #1
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #7
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
    pha
    iny
    lda ($2C),y
    tax
    pla
    sta $2E
    stx $2F
    ; [peephole-opt]     lda $2E
    ; [peephole-opt]     ldx $2F
    sta _test_x_field_access__local_0
    stx _test_x_field_access__local_0+1
@for_inc17:
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
    bra @for_cond15
@for_cond19_ph:
    lda #2
    ldx #0
    mul.16 .AX, #28
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_verts
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond19:
    lda $24
    ldx $25
    cmp.16 .AX, #4
    bcc @for_body20
    bra @for_cond23_ph
@for_body20:
    lda #2
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #7
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
    pha
    iny
    lda ($2C),y
    tax
    pla
    sta $2E
    stx $2F
    ; [peephole-opt]     lda $2E
    ; [peephole-opt]     ldx $2F
    sta _test_x_field_access__local_0
    stx _test_x_field_access__local_0+1
@for_inc21:
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
    bra @for_cond19
@for_cond23_ph:
    lda #3
    ldx #0
    mul.16 .AX, #28
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_verts
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond23:
    lda _test_x_field_access__local_32
    ldx _test_x_field_access__local_32+1
    cmp.16 .AX, #4
    bcc @for_body24
    bra @for_end26
@for_body24:
    lda #3
    ldx #0
    sta $26
    stx $27
    lda _test_x_field_access__local_32
    ldx _test_x_field_access__local_32+1
    mul.16 .AX, #7
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
    pha
    iny
    lda ($2A),y
    tax
    pla
    sta $2C
    stx $2D
    ; [peephole-opt]     lda $2C
    ; [peephole-opt]     ldx $2D
    sta _test_x_field_access__local_0
    stx _test_x_field_access__local_0+1
@for_inc25:
    lda _test_x_field_access__local_32
    ldx _test_x_field_access__local_32+1
    sta $2E
    stx $2F
    ; [peephole-opt]     lda $2E
    ; [peephole-opt]     ldx $2F
    add.16 .AX, #1
    sta _test_x_field_access__local_32
    stx _test_x_field_access__local_32+1
    bra @for_cond23
@for_end26:
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 43
    ldax #__str_27
    sta _test_x_field_access__local_42
    stx _test_x_field_access__local_42+1
    lda _test_x_field_access__local_0
    ldx _test_x_field_access__local_0+1
    sta $28
    stx $29
    lda _test_x_field_access__local_42
    ; [peephole-opt]     ldx _test_x_field_access__local_42+1
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
    .frame_size 10
    endproc

; function _test_y_field_access
; SAC inline storage: 10 bytes
    _test_y_field_access__local_0: .word 0
    _test_y_field_access__local_32: .word 0
    _test_y_field_access__local_42: .word 0
    proc _test_y_field_access
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 27
    .local @_l_x = 8
    .local @_l_y_sum = 0
; .debug_var: __test_y_field_access @_l_x offset=8 size=2 type=int16 scope=local
; .debug_var: __test_y_field_access @_l_y_sum offset=0 size=2 type=int16 scope=local

@entry:
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 48
    lda #0
    sta _test_y_field_access__local_0
    sta _test_y_field_access__local_0+1
@for_cond28_ph:
    lda #0
    tax
    mul.16 .AX, #28
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_verts
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
    lda $24
    ldx $25
    cmp.16 .AX, #4
    bcc @for_body29
    bra @for_cond32_ph
@for_body29:
    lda #0
    sta $28
    sta $29
    lda $24
    ldx $25
    mul.16 .AX, #7
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
    add.16 .AX, #3
    sta $2C
    stx $2D
    ldy #0
    lda ($2C),y
    pha
    iny
    lda ($2C),y
    tax
    pla
    sta $2E
    stx $2F
    ; [peephole-opt]     lda $2E
    ; [peephole-opt]     ldx $2F
    sta _test_y_field_access__local_0
    stx _test_y_field_access__local_0+1
@for_inc30:
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
    bra @for_cond28
@for_cond32_ph:
    lda #1
    ldx #0
    mul.16 .AX, #28
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_verts
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond32:
    lda $24
    ldx $25
    cmp.16 .AX, #4
    bcc @for_body33
    bra @for_cond36_ph
@for_body33:
    lda #1
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #7
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
    add.16 .AX, #3
    sta $2C
    stx $2D
    ldy #0
    lda ($2C),y
    pha
    iny
    lda ($2C),y
    tax
    pla
    sta $2E
    stx $2F
    ; [peephole-opt]     lda $2E
    ; [peephole-opt]     ldx $2F
    sta _test_y_field_access__local_0
    stx _test_y_field_access__local_0+1
@for_inc34:
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
    bra @for_cond32
@for_cond36_ph:
    lda #2
    ldx #0
    mul.16 .AX, #28
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_verts
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond36:
    lda $24
    ldx $25
    cmp.16 .AX, #4
    bcc @for_body37
    bra @for_cond40_ph
@for_body37:
    lda #2
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #7
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
    add.16 .AX, #3
    sta $2C
    stx $2D
    ldy #0
    lda ($2C),y
    pha
    iny
    lda ($2C),y
    tax
    pla
    sta $2E
    stx $2F
    ; [peephole-opt]     lda $2E
    ; [peephole-opt]     ldx $2F
    sta _test_y_field_access__local_0
    stx _test_y_field_access__local_0+1
@for_inc38:
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
    bra @for_cond36
@for_cond40_ph:
    lda #3
    ldx #0
    mul.16 .AX, #28
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_verts
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond40:
    lda _test_y_field_access__local_32
    ldx _test_y_field_access__local_32+1
    cmp.16 .AX, #4
    bcc @for_body41
    bra @for_end43
@for_body41:
    lda #3
    ldx #0
    sta $26
    stx $27
    lda _test_y_field_access__local_32
    ldx _test_y_field_access__local_32+1
    mul.16 .AX, #7
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
    add.16 .AX, #3
    sta $2A
    stx $2B
    ldy #0
    lda ($2A),y
    pha
    iny
    lda ($2A),y
    tax
    pla
    sta $2C
    stx $2D
    ; [peephole-opt]     lda $2C
    ; [peephole-opt]     ldx $2D
    sta _test_y_field_access__local_0
    stx _test_y_field_access__local_0+1
@for_inc42:
    lda _test_y_field_access__local_32
    ldx _test_y_field_access__local_32+1
    sta $2E
    stx $2F
    ; [peephole-opt]     lda $2E
    ; [peephole-opt]     ldx $2F
    add.16 .AX, #1
    sta _test_y_field_access__local_32
    stx _test_y_field_access__local_32+1
    bra @for_cond40
@for_end43:
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 54
    ldax #__str_44
    sta _test_y_field_access__local_42
    stx _test_y_field_access__local_42+1
    lda _test_y_field_access__local_0
    ldx _test_y_field_access__local_0+1
    sta $28
    stx $29
    lda _test_y_field_access__local_42
    ; [peephole-opt]     ldx _test_y_field_access__local_42+1
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
    .frame_size 10
    endproc

; function _test_flags_field_access
; SAC inline storage: 0 bytes
    _test_flags_field_access__local_0: .word 0
    _test_flags_field_access__local_6: .word 0
    _test_flags_field_access__local_8: .word 0
    _test_flags_field_access__local_14: .word 0
    proc _test_flags_field_access
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 38

@entry:
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 59
    ldax #__str_45
    sta _test_flags_field_access__local_0
    stx _test_flags_field_access__local_0+1
    lda #0
    tax
    mul.16 .AX, #28
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_verts
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    lda #0
    tax
    mul.16 .AX, #7
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
    add.16 .AX, #5
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta _test_flags_field_access__local_6
    stx _test_flags_field_access__local_6+1
    lda _test_flags_field_access__local_6
    ; [peephole-opt]     ldx _test_flags_field_access__local_6+1
    sta $28
    stx $29
    lda _test_flags_field_access__local_0
    ldx _test_flags_field_access__local_0+1
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
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 60
    ldax #__str_46
    sta _test_flags_field_access__local_8
    stx _test_flags_field_access__local_8+1
    lda #3
    ldx #0
    mul.16 .AX, #28
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_verts
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    lda #3
    ldx #0
    mul.16 .AX, #7
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
    add.16 .AX, #5
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta _test_flags_field_access__local_14
    stx _test_flags_field_access__local_14+1
    lda _test_flags_field_access__local_14
    ; [peephole-opt]     ldx _test_flags_field_access__local_14+1
    sta $28
    stx $29
    lda _test_flags_field_access__local_8
    ldx _test_flags_field_access__local_8+1
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
    .frame_size 0
    endproc

; function _test_sequential_field_access
; SAC inline storage: 6 bytes
    _test_sequential_field_access__local_0: .word 0
    _test_sequential_field_access__local_2: .word 0
    _test_sequential_field_access__local_3: .word 0
    _test_sequential_field_access__local_4: .word 0
    _test_sequential_field_access__local_5: .word 0
    _test_sequential_field_access__local_6: .word 0
    _test_sequential_field_access__local_7: .word 0
    _test_sequential_field_access__local_8: .word 0
    _test_sequential_field_access__local_9: .word 0
    _test_sequential_field_access__local_10: .word 0
    _test_sequential_field_access__local_11: .word 0
    _test_sequential_field_access__local_12: .word 0
    _test_sequential_field_access__local_13: .word 0
    _test_sequential_field_access__local_14: .word 0
    _test_sequential_field_access__local_15: .word 0
    _test_sequential_field_access__local_16: .word 0
    proc _test_sequential_field_access
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 44
    .local @_l_i = 0
    .local @_l_id = 2
    .local @_l_x = 4
; .debug_var: __test_sequential_field_access @_l_i offset=0 size=2 type=int16 scope=local
; .debug_var: __test_sequential_field_access @_l_id offset=2 size=2 type=int8 scope=local
; .debug_var: __test_sequential_field_access @_l_x offset=4 size=2 type=int16 scope=local

@entry:
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 65
    lda #0
    sta _test_sequential_field_access__local_0
    sta _test_sequential_field_access__local_0+1
@for_cond47_ph:
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 66
    lda #0
    tax
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 67
    lda #0
    tax
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 66
    mul.16 .AX, #28
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_verts
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta _test_sequential_field_access__local_6
    stx _test_sequential_field_access__local_6+1
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 67
    mul.16 .AX, #28
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_verts
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta _test_sequential_field_access__local_12
    stx _test_sequential_field_access__local_12+1
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 68
    ldax #__str_51
    sta _test_sequential_field_access__local_16
    stx _test_sequential_field_access__local_16+1
@for_cond47:
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 65
    lda _test_sequential_field_access__local_0
    ldx _test_sequential_field_access__local_0+1
    cmp.16 .AX, #4
    bcc @for_body48
    bra @for_end50
@for_body48:
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 66
    lda #0
    tax
    sta _test_sequential_field_access__local_5
    stx _test_sequential_field_access__local_5+1
    lda _test_sequential_field_access__local_6
    ldx _test_sequential_field_access__local_6+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _test_sequential_field_access__local_0
    ldx _test_sequential_field_access__local_0+1
    mul.16 .AX, #7
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda __zp_scratch2
    ; [peephole-opt]     ldx __zp_scratch2+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta _test_sequential_field_access__local_7
    stx _test_sequential_field_access__local_7+1
    lda _test_sequential_field_access__local_7
    ; [peephole-opt]     ldx _test_sequential_field_access__local_7+1
    sta _test_sequential_field_access__local_8
    stx _test_sequential_field_access__local_8+1
    lda _test_sequential_field_access__local_7
    ; [peephole-opt]     ldx _test_sequential_field_access__local_7+1
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta _test_sequential_field_access__local_9
    ; [peephole-opt]     lda _test_sequential_field_access__local_9
    sta _test_sequential_field_access__local_4
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 67
    lda #0
    tax
    sta _test_sequential_field_access__local_11
    stx _test_sequential_field_access__local_11+1
    lda _test_sequential_field_access__local_12
    ldx _test_sequential_field_access__local_12+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _test_sequential_field_access__local_0
    ldx _test_sequential_field_access__local_0+1
    mul.16 .AX, #7
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda __zp_scratch2
    ; [peephole-opt]     ldx __zp_scratch2+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta _test_sequential_field_access__local_13
    stx _test_sequential_field_access__local_13+1
    lda _test_sequential_field_access__local_13
    ; [peephole-opt]     ldx _test_sequential_field_access__local_13+1
    add.16 .AX, #1
    sta _test_sequential_field_access__local_14
    stx _test_sequential_field_access__local_14+1
    lda _test_sequential_field_access__local_14
    ; [peephole-opt]     ldx _test_sequential_field_access__local_14+1
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta _test_sequential_field_access__local_15
    stx _test_sequential_field_access__local_15+1
    lda _test_sequential_field_access__local_15
    ; [peephole-opt]     ldx _test_sequential_field_access__local_15+1
    sta _test_sequential_field_access__local_10
    stx _test_sequential_field_access__local_10+1
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 68
    lda _test_sequential_field_access__local_10
    ; [peephole-opt]     ldx _test_sequential_field_access__local_10+1
    sta $28
    stx $29
    lda _test_sequential_field_access__local_4
    sta $2A
    stx $2B
    lda _test_sequential_field_access__local_0
    ldx _test_sequential_field_access__local_0+1
    sta $2C
    stx $2D
    lda _test_sequential_field_access__local_16
    ldx _test_sequential_field_access__local_16+1
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
@for_inc49:
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 65
    lda _test_sequential_field_access__local_0
    ldx _test_sequential_field_access__local_0+1
    sta $22
    stx $23
    ; [peephole-opt]     lda $22
    ; [peephole-opt]     ldx $23
    add.16 .AX, #1
    sta _test_sequential_field_access__local_0
    stx _test_sequential_field_access__local_0+1
    bra @for_cond47
@for_end50:
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _main
; SAC inline storage: 30 bytes
    _main__local_0: .word 0
    _main__local_2: .word 0
    _main__local_4: .word 0
    _main__local_6: .word 0
    _main__local_7: .word 0
    _main__local_8: .word 0
    _main__local_9: .word 0
    _main__local_10: .word 0
    _main__local_11: .word 0
    _main__local_12: .word 0
    _main__local_13: .word 0
    _main__local_14: .word 0
    _main__local_15: .word 0
    _main__local_16: .word 0
    _main__local_22: .word 0
    _main__local_24: .word 0
    _main__local_66: .word 0
    _main__local_68: .word 0
    _main__local_110: .word 0
    _main__local_112: .word 0
    _main__local_118: .word 0
    _main__local_120: .word 0
    _main__local_126: .word 0
    _main__local_128: .word 0
    _main__local_130: .word 0
    _main__local_131: .word 0
    _main__local_132: .word 0
    _main__local_133: .word 0
    _main__local_134: .word 0
    _main__local_135: .word 0
    _main__local_136: .word 0
    _main__local_137: .word 0
    _main__local_138: .word 0
    _main__local_139: .word 0
    _main__local_140: .word 0
    _main__local_141: .word 0
    _main__local_142: .word 0
    _main__local_143: .word 0
    _main__local_144: .word 0
    _main__local_148: .word 0
    _main__local_149: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 53
    .local @_l_i = 24
    .local @_l_id = 26
    .local @_l_x = 28
    .local @_l_x_sum = 4
    .local @_l_y = 0
    .local @_l_y_sum = 14
; .debug_var: __main @_l_i offset=24 size=2 type=int16 scope=local
; .debug_var: __main @_l_id offset=26 size=2 type=int8 scope=local
; .debug_var: __main @_l_x offset=28 size=2 type=int16 scope=local
; .debug_var: __main @_l_x_sum offset=4 size=2 type=int16 scope=local
; .debug_var: __main @_l_y offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_y_sum offset=14 size=2 type=int16 scope=local

@entry:
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 73
    ldax #__str_52
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
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 26
    ldax #__str_54
    sta _main__local_2
    stx _main__local_2+1
    lda _main__local_2
    ; [peephole-opt]     ldx _main__local_2+1
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
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 27
    lda #0
    sta _main__local_4
    sta _main__local_4+1
@for_cond55_ph:
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 29
    ldax #__str_63
    sta _main__local_12
    stx _main__local_12+1
@for_cond55:
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 27
    lda _main__local_4
    ldx _main__local_4+1
    cmp.16 .AX, #4
    bcc @for_body56
    bra @for_end58
@for_body56:
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 28
    lda #0
    sta _main__local_8
    sta _main__local_8+1
@for_cond59_ph:
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 29
    lda _main__local_4
    ldx _main__local_4+1
    mul.16 .AX, #28
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_verts
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta _main__local_13
    stx _main__local_13+1
@for_cond59:
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 28
    lda _main__local_8
    ldx _main__local_8+1
    cmp.16 .AX, #4
    bcc @for_body60
    bra @for_end62
@for_body60:
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 29
    lda _main__local_13
    ldx _main__local_13+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_8
    ldx _main__local_8+1
    mul.16 .AX, #7
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda __zp_scratch2
    ; [peephole-opt]     ldx __zp_scratch2+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta _main__local_14
    stx _main__local_14+1
    lda _main__local_14
    ; [peephole-opt]     ldx _main__local_14+1
    sta _main__local_15
    stx _main__local_15+1
    lda _main__local_14
    ; [peephole-opt]     ldx _main__local_14+1
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta _main__local_16
    ; [peephole-opt]     lda _main__local_16
    sta $28
    stx $29
    lda _main__local_12
    ldx _main__local_12+1
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
@for_inc61:
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 28
    lda _main__local_8
    ldx _main__local_8+1
    sta $22
    stx $23
    ; [peephole-opt]     lda $22
    clc
    adc #1
    sta $24
    lda $23
    adc #0
    sta $25
    ; [peephole-opt]     lda $24
    ldx $25
    sta _main__local_8
    stx _main__local_8+1
    bra @for_cond59
@for_end62:
@for_inc57:
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 27
    lda _main__local_4
    ldx _main__local_4+1
    sta $26
    stx $27
    ; [peephole-opt]     lda $26
    ; [peephole-opt]     ldx $27
    add.16 .AX, #1
    sta _main__local_4
    stx _main__local_4+1
    bra @for_cond55
@for_end58:
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 32
    ldax #__str_64
    sta _main__local_22
    stx _main__local_22+1
    lda _main__local_22
    ; [peephole-opt]     ldx _main__local_22+1
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
@inline_end53:
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 37
    lda #0
    sta _main__local_24
    sta _main__local_24+1
@for_cond66_ph:
    lda #0
    tax
    mul.16 .AX, #28
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_verts
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
    cmp.16 .AX, #4
    bcc @for_body67
    bra @for_cond70_ph
@for_body67:
    lda #0
    sta $28
    sta $29
    lda $24
    ldx $25
    mul.16 .AX, #7
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
    pha
    iny
    lda ($2C),y
    tax
    pla
    sta $2E
    stx $2F
    ; [peephole-opt]     lda $2E
    ; [peephole-opt]     ldx $2F
    sta _main__local_24
    stx _main__local_24+1
@for_inc68:
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
    bra @for_cond66
@for_cond70_ph:
    lda #1
    ldx #0
    mul.16 .AX, #28
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_verts
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
    cmp.16 .AX, #4
    bcc @for_body71
    bra @for_cond74_ph
@for_body71:
    lda #1
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #7
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
    pha
    iny
    lda ($2C),y
    tax
    pla
    sta $2E
    stx $2F
    ; [peephole-opt]     lda $2E
    ; [peephole-opt]     ldx $2F
    sta _main__local_24
    stx _main__local_24+1
@for_inc72:
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
    bra @for_cond70
@for_cond74_ph:
    lda #2
    ldx #0
    mul.16 .AX, #28
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_verts
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
    cmp.16 .AX, #4
    bcc @for_body75
    bra @for_cond78_ph
@for_body75:
    lda #2
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #7
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
    pha
    iny
    lda ($2C),y
    tax
    pla
    sta $2E
    stx $2F
    ; [peephole-opt]     lda $2E
    ; [peephole-opt]     ldx $2F
    sta _main__local_24
    stx _main__local_24+1
@for_inc76:
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
    bra @for_cond74
@for_cond78_ph:
    lda #3
    ldx #0
    mul.16 .AX, #28
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_verts
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
    cmp.16 .AX, #4
    bcc @for_body79
    bra @for_end81
@for_body79:
    lda #3
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #7
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
    pha
    iny
    lda ($2C),y
    tax
    pla
    sta $2E
    stx $2F
    ; [peephole-opt]     lda $2E
    ; [peephole-opt]     ldx $2F
    sta _main__local_24
    stx _main__local_24+1
@for_inc80:
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
    bra @for_cond78
@for_end81:
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 43
    ldax #__str_82
    sta _main__local_66
    stx _main__local_66+1
    lda _main__local_24
    ldx _main__local_24+1
    sta $28
    stx $29
    lda _main__local_66
    ; [peephole-opt]     ldx _main__local_66+1
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
@inline_end65:
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 48
    lda #0
    sta _main__local_68
    sta _main__local_68+1
@for_cond84_ph:
    lda #0
    tax
    mul.16 .AX, #28
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_verts
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond84:
    lda $24
    ldx $25
    cmp.16 .AX, #4
    bcc @for_body85
    bra @for_cond88_ph
@for_body85:
    lda #0
    sta $28
    sta $29
    lda $24
    ldx $25
    mul.16 .AX, #7
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
    add.16 .AX, #3
    sta $2C
    stx $2D
    ldy #0
    lda ($2C),y
    pha
    iny
    lda ($2C),y
    tax
    pla
    sta $2E
    stx $2F
    ; [peephole-opt]     lda $2E
    ; [peephole-opt]     ldx $2F
    sta _main__local_68
    stx _main__local_68+1
@for_inc86:
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
    bra @for_cond84
@for_cond88_ph:
    lda #1
    ldx #0
    mul.16 .AX, #28
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_verts
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond88:
    lda $24
    ldx $25
    cmp.16 .AX, #4
    bcc @for_body89
    bra @for_cond92_ph
@for_body89:
    lda #1
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #7
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
    add.16 .AX, #3
    sta $2C
    stx $2D
    ldy #0
    lda ($2C),y
    pha
    iny
    lda ($2C),y
    tax
    pla
    sta $2E
    stx $2F
    ; [peephole-opt]     lda $2E
    ; [peephole-opt]     ldx $2F
    sta _main__local_68
    stx _main__local_68+1
@for_inc90:
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
    bra @for_cond88
@for_cond92_ph:
    lda #2
    ldx #0
    mul.16 .AX, #28
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_verts
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond92:
    lda $24
    ldx $25
    cmp.16 .AX, #4
    bcc @for_body93
    bra @for_cond96_ph
@for_body93:
    lda #2
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #7
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
    add.16 .AX, #3
    sta $2C
    stx $2D
    ldy #0
    lda ($2C),y
    pha
    iny
    lda ($2C),y
    tax
    pla
    sta $2E
    stx $2F
    ; [peephole-opt]     lda $2E
    ; [peephole-opt]     ldx $2F
    sta _main__local_68
    stx _main__local_68+1
@for_inc94:
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
    bra @for_cond92
@for_cond96_ph:
    lda #3
    ldx #0
    mul.16 .AX, #28
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_verts
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
@for_cond96:
    lda $24
    ldx $25
    cmp.16 .AX, #4
    bcc @for_body97
    bra @for_end99
@for_body97:
    lda #3
    ldx #0
    sta $28
    stx $29
    lda $24
    ldx $25
    mul.16 .AX, #7
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
    add.16 .AX, #3
    sta $2C
    stx $2D
    ldy #0
    lda ($2C),y
    pha
    iny
    lda ($2C),y
    tax
    pla
    sta $2E
    stx $2F
    ; [peephole-opt]     lda $2E
    ; [peephole-opt]     ldx $2F
    sta _main__local_68
    stx _main__local_68+1
@for_inc98:
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
    bra @for_cond96
@for_end99:
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 54
    ldax #__str_100
    sta _main__local_110
    stx _main__local_110+1
    lda _main__local_68
    ldx _main__local_68+1
    sta $28
    stx $29
    lda _main__local_110
    ; [peephole-opt]     ldx _main__local_110+1
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
@inline_end83:
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 59
    ldax #__str_102
    sta _main__local_112
    stx _main__local_112+1
    lda #0
    tax
    mul.16 .AX, #28
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_verts
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    lda #0
    tax
    mul.16 .AX, #7
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
    add.16 .AX, #5
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta _main__local_118
    stx _main__local_118+1
    lda _main__local_118
    ; [peephole-opt]     ldx _main__local_118+1
    sta $28
    stx $29
    lda _main__local_112
    ldx _main__local_112+1
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
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 60
    ldax #__str_103
    sta _main__local_120
    stx _main__local_120+1
    lda #3
    ldx #0
    mul.16 .AX, #28
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_verts
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    lda #3
    ldx #0
    mul.16 .AX, #7
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
    add.16 .AX, #5
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta _main__local_126
    stx _main__local_126+1
    lda _main__local_126
    ; [peephole-opt]     ldx _main__local_126+1
    sta $28
    stx $29
    lda _main__local_120
    ldx _main__local_120+1
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
@inline_end101:
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 65
    lda #0
    sta _main__local_128
    sta _main__local_128+1
@for_cond105_ph:
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 66
    lda #0
    tax
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 67
    lda #0
    tax
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 66
    mul.16 .AX, #28
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_verts
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta _main__local_134
    stx _main__local_134+1
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 67
    mul.16 .AX, #28
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_verts
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta _main__local_140
    stx _main__local_140+1
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 68
    ldax #__str_109
    sta _main__local_144
    stx _main__local_144+1
@for_cond105:
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 65
    lda _main__local_128
    ldx _main__local_128+1
    cmp.16 .AX, #4
    bcc @for_body106
    bra @for_end108
@for_body106:
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 66
    lda #0
    tax
    sta _main__local_133
    stx _main__local_133+1
    lda _main__local_134
    ldx _main__local_134+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_128
    ldx _main__local_128+1
    mul.16 .AX, #7
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda __zp_scratch2
    ; [peephole-opt]     ldx __zp_scratch2+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta _main__local_135
    stx _main__local_135+1
    lda _main__local_135
    ; [peephole-opt]     ldx _main__local_135+1
    sta _main__local_136
    stx _main__local_136+1
    lda _main__local_135
    ; [peephole-opt]     ldx _main__local_135+1
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta _main__local_137
    ; [peephole-opt]     lda _main__local_137
    sta _main__local_132
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 67
    lda #0
    tax
    sta _main__local_139
    stx _main__local_139+1
    lda _main__local_140
    ldx _main__local_140+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_128
    ldx _main__local_128+1
    mul.16 .AX, #7
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda __zp_scratch2
    ; [peephole-opt]     ldx __zp_scratch2+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta _main__local_141
    stx _main__local_141+1
    lda _main__local_141
    ; [peephole-opt]     ldx _main__local_141+1
    add.16 .AX, #1
    sta _main__local_142
    stx _main__local_142+1
    lda _main__local_142
    ; [peephole-opt]     ldx _main__local_142+1
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta _main__local_143
    stx _main__local_143+1
    lda _main__local_143
    ; [peephole-opt]     ldx _main__local_143+1
    sta _main__local_138
    stx _main__local_138+1
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 68
    lda _main__local_138
    ; [peephole-opt]     ldx _main__local_138+1
    sta $28
    stx $29
    lda _main__local_132
    sta $2A
    stx $2B
    lda _main__local_128
    ldx _main__local_128+1
    sta $2C
    stx $2D
    lda _main__local_144
    ldx _main__local_144+1
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
@for_inc107:
    .loc "src/test-resources/test_phase95_mixed_field_sizes.c", 65
    lda _main__local_128
    ldx _main__local_128+1
    sta $22
    stx $23
    ; [peephole-opt]     lda $22
    ; [peephole-opt]     ldx $23
    add.16 .AX, #1
    sta _main__local_128
    stx _main__local_128+1
    bra @for_cond105
@for_end108:
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 30
    endproc


    .segment "data"
__str_0:
    .text "IDs: "
    .byte 0
__str_9:
    .text "%d "
    .byte 0
__str_10:
    .text "
"
    .byte 0
__str_27:
    .text "X sum: %d
"
    .byte 0
__str_44:
    .text "Y sum: %d
"
    .byte 0
__str_45:
    .text "First vertex flags: $%08X
"
    .byte 0
__str_46:
    .text "Last vertex flags: $%08X
"
    .byte 0
__str_51:
    .text "Vert[0,%d]: id=%d, x=%d
"
    .byte 0
__str_52:
    .text "Phase 95.6: Mixed field sizes test
"
    .byte 0
__str_54:
    .text "IDs: "
    .byte 0
__str_63:
    .text "%d "
    .byte 0
__str_64:
    .text "
"
    .byte 0
__str_82:
    .text "X sum: %d
"
    .byte 0
__str_100:
    .text "Y sum: %d
"
    .byte 0
__str_102:
    .text "First vertex flags: $%08X
"
    .byte 0
__str_103:
    .text "Last vertex flags: $%08X
"
    .byte 0
__str_109:
    .text "Vert[0,%d]: id=%d, x=%d
"
    .byte 0
__str_110:
    .text "All tests completed
"
    .byte 0

__zp_save_buf:
; [DEBUG] Phase 87 code reached, optimize=true

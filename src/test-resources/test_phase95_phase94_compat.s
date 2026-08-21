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

    .global _phase94_pixels
    .global _regular_pixels
    .global _test_phase94_array
    .global _test_regular_array
    .global _verify_data_integrity
    .global _main

    .segment "data"
    .byte 0
_phase94_pixels:
; .debug_var: @global _phase94_pixels offset=0 size=2 type=int16 scope=global
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .word 0
    .res 176
_regular_pixels:
; .debug_var: @global _regular_pixels offset=0 size=2 type=int16 scope=global
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

; function _test_phase94_array
; SAC inline storage: 22 bytes
    _test_phase94_array__local_0: .word 0
    _test_phase94_array__local_2: .word 0
    _test_phase94_array__local_4: .word 0
    _test_phase94_array__local_6: .word 0
    _test_phase94_array__local_14: .word 0
    _test_phase94_array__local_37: .word 0
    _test_phase94_array__local_60: .word 0
    _test_phase94_array__local_83: .word 0
    _test_phase94_array__local_106: .word 0
    _test_phase94_array__local_129: .word 0
    _test_phase94_array__local_152: .word 0
    _test_phase94_array__local_169: .word 0
    _test_phase94_array__local_175: .word 0
    _test_phase94_array__local_192: .word 0
    proc _test_phase94_array
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "src/test-resources/test_phase95_phase94_compat.c", 33
    .local @_l_b_sum = 4
    .local @_l_g_sum = 2
    .local @_l_r_sum = 0
    .local @_l_x = 20
; .debug_var: __test_phase94_array @_l_b_sum offset=4 size=2 type=int16 scope=local
; .debug_var: __test_phase94_array @_l_g_sum offset=2 size=2 type=int16 scope=local
; .debug_var: __test_phase94_array @_l_r_sum offset=0 size=2 type=int16 scope=local
; .debug_var: __test_phase94_array @_l_x offset=20 size=2 type=int16 scope=local

@entry:
    .loc "src/test-resources/test_phase95_phase94_compat.c", 54
    ldax #__str_0
    sta _test_phase94_array__local_0
    stx _test_phase94_array__local_0+1
    lda _test_phase94_array__local_0
    ; [peephole-opt]     ldx _test_phase94_array__local_0+1
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
    .loc "src/test-resources/test_phase95_phase94_compat.c", 55
    lda #0
    sta _test_phase94_array__local_2
    sta _test_phase94_array__local_2+1
    lda #0
    sta _test_phase94_array__local_4
    sta _test_phase94_array__local_4+1
    lda #0
    sta _test_phase94_array__local_6
    sta _test_phase94_array__local_6+1
@for_cond1_ph:
    lda #0
    sta $20
    sta $21
    lda #0
    sta $22
    sta $23
    lda #0
    sta $24
    sta $25
    ; [peephole-opt]     lda $20
    ldx $21
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $26
    stx $26+1
    ; [peephole-opt]     lda $22
    ldx $23
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
    lda $24
    ldx $25
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
@for_cond1:
    lda $28
    ldx $29
    cmp.16 .AX, #8
    bcc @for_body2
    bra @for_cond5_ph
@for_body2:
    lda #0
    sta $2C
    sta $2D
    lda $28
    ldx $29
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $26
    ldx $26+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2E
    stx $2E+1
    ldy #0
    lda ($2E),y
    ldx #0
    sta $30
    ; [peephole-opt]     lda $30
    ldx #0
    ldx #0
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    ; [peephole-opt]     ldx $33
    sta _test_phase94_array__local_2
    stx _test_phase94_array__local_2+1
    lda #0
    sta $34
    sta $35
    lda $28
    ldx $29
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
    sta $36
    stx $36+1
    lda $36
    ldx $37
    add.16 .AX, #1
    sta $38
    stx $39
    ldy #0
    lda ($38),y
    ldx #0
    sta $3A
    ; [peephole-opt]     lda $3A
    ldx #0
    ldx #0
    sta $3C
    stx $3D
    ; [peephole-opt]     lda $3C
    ; [peephole-opt]     ldx $3D
    sta _test_phase94_array__local_4
    stx _test_phase94_array__local_4+1
    lda #0
    sta $3E
    sta $3F
    lda $28
    ldx $29
    mul.16 .AX, #3
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
    sta $40
    stx $40+1
    lda $40
    ldx $41
    add.16 .AX, #2
    sta $42
    stx $43
    ldy #0
    lda ($42),y
    ldx #0
    sta $44
    ; [peephole-opt]     lda $44
    ldx #0
    ldx #0
    sta $46
    stx $47
    ; [peephole-opt]     lda $46
    ; [peephole-opt]     ldx $47
    sta _test_phase94_array__local_6
    stx _test_phase94_array__local_6+1
@for_inc3:
    lda $28
    ldx $29
    sta $48
    stx $49
    ; [peephole-opt]     lda $48
    clc
    adc #1
    sta $28
    lda $49
    adc #0
    sta $29
    bra @for_cond1
@for_cond5_ph:
    lda #1
    ldx #0
    sta $20
    stx $21
    lda #1
    ldx #0
    sta $22
    stx $23
    lda #1
    ldx #0
    sta $24
    stx $25
    ; [peephole-opt]     lda $20
    ; [peephole-opt]     ldx $21
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $26
    stx $26+1
    ; [peephole-opt]     lda $22
    ; [peephole-opt]     ldx $23
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
    lda $24
    ldx $25
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
@for_cond5:
    lda $28
    ldx $29
    cmp.16 .AX, #8
    bcc @for_body6
    bra @for_cond9_ph
@for_body6:
    lda #1
    ldx #0
    sta $2C
    stx $2D
    lda $28
    ldx $29
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $26
    ldx $26+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2E
    stx $2E+1
    ldy #0
    lda ($2E),y
    ldx #0
    sta $30
    ; [peephole-opt]     lda $30
    ldx #0
    ldx #0
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    ; [peephole-opt]     ldx $33
    sta _test_phase94_array__local_2
    stx _test_phase94_array__local_2+1
    lda #1
    ldx #0
    sta $34
    stx $35
    lda $28
    ldx $29
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
    sta $36
    stx $36+1
    lda $36
    ldx $37
    add.16 .AX, #1
    sta $38
    stx $39
    ldy #0
    lda ($38),y
    ldx #0
    sta $3A
    ; [peephole-opt]     lda $3A
    ldx #0
    ldx #0
    sta $3C
    stx $3D
    ; [peephole-opt]     lda $3C
    ; [peephole-opt]     ldx $3D
    sta _test_phase94_array__local_4
    stx _test_phase94_array__local_4+1
    lda #1
    ldx #0
    sta $3E
    stx $3F
    lda $28
    ldx $29
    mul.16 .AX, #3
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
    sta $40
    stx $40+1
    lda $40
    ldx $41
    add.16 .AX, #2
    sta $42
    stx $43
    ldy #0
    lda ($42),y
    ldx #0
    sta $44
    ; [peephole-opt]     lda $44
    ldx #0
    ldx #0
    sta $46
    stx $47
    ; [peephole-opt]     lda $46
    ; [peephole-opt]     ldx $47
    sta _test_phase94_array__local_6
    stx _test_phase94_array__local_6+1
@for_inc7:
    lda $28
    ldx $29
    sta $48
    stx $49
    ; [peephole-opt]     lda $48
    clc
    adc #1
    sta $28
    lda $49
    adc #0
    sta $29
    bra @for_cond5
@for_cond9_ph:
    lda #2
    ldx #0
    sta $20
    stx $21
    lda #2
    ldx #0
    sta $22
    stx $23
    lda #2
    ldx #0
    sta $24
    stx $25
    ; [peephole-opt]     lda $20
    ; [peephole-opt]     ldx $21
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $26
    stx $26+1
    ; [peephole-opt]     lda $22
    ; [peephole-opt]     ldx $23
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
    lda $24
    ldx $25
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
@for_cond9:
    lda $28
    ldx $29
    cmp.16 .AX, #8
    bcc @for_body10
    bra @for_cond13_ph
@for_body10:
    lda #2
    ldx #0
    sta $2C
    stx $2D
    lda $28
    ldx $29
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $26
    ldx $26+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2E
    stx $2E+1
    ldy #0
    lda ($2E),y
    ldx #0
    sta $30
    ; [peephole-opt]     lda $30
    ldx #0
    ldx #0
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    ; [peephole-opt]     ldx $33
    sta _test_phase94_array__local_2
    stx _test_phase94_array__local_2+1
    lda #2
    ldx #0
    sta $34
    stx $35
    lda $28
    ldx $29
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
    sta $36
    stx $36+1
    lda $36
    ldx $37
    add.16 .AX, #1
    sta $38
    stx $39
    ldy #0
    lda ($38),y
    ldx #0
    sta $3A
    ; [peephole-opt]     lda $3A
    ldx #0
    ldx #0
    sta $3C
    stx $3D
    ; [peephole-opt]     lda $3C
    ; [peephole-opt]     ldx $3D
    sta _test_phase94_array__local_4
    stx _test_phase94_array__local_4+1
    lda #2
    ldx #0
    sta $3E
    stx $3F
    lda $28
    ldx $29
    mul.16 .AX, #3
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
    sta $40
    stx $40+1
    lda $40
    ldx $41
    add.16 .AX, #2
    sta $42
    stx $43
    ldy #0
    lda ($42),y
    ldx #0
    sta $44
    ; [peephole-opt]     lda $44
    ldx #0
    ldx #0
    sta $46
    stx $47
    ; [peephole-opt]     lda $46
    ; [peephole-opt]     ldx $47
    sta _test_phase94_array__local_6
    stx _test_phase94_array__local_6+1
@for_inc11:
    lda $28
    ldx $29
    sta $48
    stx $49
    ; [peephole-opt]     lda $48
    clc
    adc #1
    sta $28
    lda $49
    adc #0
    sta $29
    bra @for_cond9
@for_cond13_ph:
    lda #3
    ldx #0
    sta $20
    stx $21
    lda #3
    ldx #0
    sta $22
    stx $23
    lda #3
    ldx #0
    sta $24
    stx $25
    ; [peephole-opt]     lda $20
    ; [peephole-opt]     ldx $21
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $26
    stx $26+1
    ; [peephole-opt]     lda $22
    ; [peephole-opt]     ldx $23
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
    lda $24
    ldx $25
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
@for_cond13:
    lda $28
    ldx $29
    cmp.16 .AX, #8
    bcc @for_body14
    bra @for_cond17_ph
@for_body14:
    lda #3
    ldx #0
    sta $2C
    stx $2D
    lda $28
    ldx $29
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $26
    ldx $26+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2E
    stx $2E+1
    ldy #0
    lda ($2E),y
    ldx #0
    sta $30
    ; [peephole-opt]     lda $30
    ldx #0
    ldx #0
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    ; [peephole-opt]     ldx $33
    sta _test_phase94_array__local_2
    stx _test_phase94_array__local_2+1
    lda #3
    ldx #0
    sta $34
    stx $35
    lda $28
    ldx $29
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
    sta $36
    stx $36+1
    lda $36
    ldx $37
    add.16 .AX, #1
    sta $38
    stx $39
    ldy #0
    lda ($38),y
    ldx #0
    sta $3A
    ; [peephole-opt]     lda $3A
    ldx #0
    ldx #0
    sta $3C
    stx $3D
    ; [peephole-opt]     lda $3C
    ; [peephole-opt]     ldx $3D
    sta _test_phase94_array__local_4
    stx _test_phase94_array__local_4+1
    lda #3
    ldx #0
    sta $3E
    stx $3F
    lda $28
    ldx $29
    mul.16 .AX, #3
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
    sta $40
    stx $40+1
    lda $40
    ldx $41
    add.16 .AX, #2
    sta $42
    stx $43
    ldy #0
    lda ($42),y
    ldx #0
    sta $44
    ; [peephole-opt]     lda $44
    ldx #0
    ldx #0
    sta $46
    stx $47
    ; [peephole-opt]     lda $46
    ; [peephole-opt]     ldx $47
    sta _test_phase94_array__local_6
    stx _test_phase94_array__local_6+1
@for_inc15:
    lda $28
    ldx $29
    sta $48
    stx $49
    ; [peephole-opt]     lda $48
    clc
    adc #1
    sta $28
    lda $49
    adc #0
    sta $29
    bra @for_cond13
@for_cond17_ph:
    lda #4
    ldx #0
    sta $20
    stx $21
    lda #4
    ldx #0
    sta $22
    stx $23
    lda #4
    ldx #0
    sta $24
    stx $25
    ; [peephole-opt]     lda $20
    ; [peephole-opt]     ldx $21
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $26
    stx $26+1
    ; [peephole-opt]     lda $22
    ; [peephole-opt]     ldx $23
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
    lda $24
    ldx $25
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
@for_cond17:
    lda $28
    ldx $29
    cmp.16 .AX, #8
    bcc @for_body18
    bra @for_cond21_ph
@for_body18:
    lda #4
    ldx #0
    sta $2C
    stx $2D
    lda $28
    ldx $29
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $26
    ldx $26+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2E
    stx $2E+1
    ldy #0
    lda ($2E),y
    ldx #0
    sta $30
    ; [peephole-opt]     lda $30
    ldx #0
    ldx #0
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    ; [peephole-opt]     ldx $33
    sta _test_phase94_array__local_2
    stx _test_phase94_array__local_2+1
    lda #4
    ldx #0
    sta $34
    stx $35
    lda $28
    ldx $29
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
    sta $36
    stx $36+1
    lda $36
    ldx $37
    add.16 .AX, #1
    sta $38
    stx $39
    ldy #0
    lda ($38),y
    ldx #0
    sta $3A
    ; [peephole-opt]     lda $3A
    ldx #0
    ldx #0
    sta $3C
    stx $3D
    ; [peephole-opt]     lda $3C
    ; [peephole-opt]     ldx $3D
    sta _test_phase94_array__local_4
    stx _test_phase94_array__local_4+1
    lda #4
    ldx #0
    sta $3E
    stx $3F
    lda $28
    ldx $29
    mul.16 .AX, #3
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
    sta $40
    stx $40+1
    lda $40
    ldx $41
    add.16 .AX, #2
    sta $42
    stx $43
    ldy #0
    lda ($42),y
    ldx #0
    sta $44
    ; [peephole-opt]     lda $44
    ldx #0
    ldx #0
    sta $46
    stx $47
    ; [peephole-opt]     lda $46
    ; [peephole-opt]     ldx $47
    sta _test_phase94_array__local_6
    stx _test_phase94_array__local_6+1
@for_inc19:
    lda $28
    ldx $29
    sta $48
    stx $49
    ; [peephole-opt]     lda $48
    clc
    adc #1
    sta $28
    lda $49
    adc #0
    sta $29
    bra @for_cond17
@for_cond21_ph:
    lda #5
    ldx #0
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
    ; [peephole-opt]     lda $20
    ; [peephole-opt]     ldx $21
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $26
    stx $26+1
    ; [peephole-opt]     lda $22
    ; [peephole-opt]     ldx $23
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
    lda $24
    ldx $25
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
@for_cond21:
    lda $28
    ldx $29
    cmp.16 .AX, #8
    bcc @for_body22
    bra @for_cond25_ph
@for_body22:
    lda #5
    ldx #0
    sta $2C
    stx $2D
    lda $28
    ldx $29
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $26
    ldx $26+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2E
    stx $2E+1
    ldy #0
    lda ($2E),y
    ldx #0
    sta $30
    ; [peephole-opt]     lda $30
    ldx #0
    ldx #0
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    ; [peephole-opt]     ldx $33
    sta _test_phase94_array__local_2
    stx _test_phase94_array__local_2+1
    lda #5
    ldx #0
    sta $34
    stx $35
    lda $28
    ldx $29
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
    sta $36
    stx $36+1
    lda $36
    ldx $37
    add.16 .AX, #1
    sta $38
    stx $39
    ldy #0
    lda ($38),y
    ldx #0
    sta $3A
    ; [peephole-opt]     lda $3A
    ldx #0
    ldx #0
    sta $3C
    stx $3D
    ; [peephole-opt]     lda $3C
    ; [peephole-opt]     ldx $3D
    sta _test_phase94_array__local_4
    stx _test_phase94_array__local_4+1
    lda #5
    ldx #0
    sta $3E
    stx $3F
    lda $28
    ldx $29
    mul.16 .AX, #3
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
    sta $40
    stx $40+1
    lda $40
    ldx $41
    add.16 .AX, #2
    sta $42
    stx $43
    ldy #0
    lda ($42),y
    ldx #0
    sta $44
    ; [peephole-opt]     lda $44
    ldx #0
    ldx #0
    sta $46
    stx $47
    ; [peephole-opt]     lda $46
    ; [peephole-opt]     ldx $47
    sta _test_phase94_array__local_6
    stx _test_phase94_array__local_6+1
@for_inc23:
    lda $28
    ldx $29
    sta $48
    stx $49
    ; [peephole-opt]     lda $48
    clc
    adc #1
    sta $28
    lda $49
    adc #0
    sta $29
    bra @for_cond21
@for_cond25_ph:
    lda #6
    ldx #0
    sta $20
    stx $21
    lda #6
    ldx #0
    sta $22
    stx $23
    lda #6
    ldx #0
    sta $24
    stx $25
    ; [peephole-opt]     lda $20
    ; [peephole-opt]     ldx $21
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $26
    stx $26+1
    ; [peephole-opt]     lda $22
    ; [peephole-opt]     ldx $23
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
    lda $24
    ldx $25
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
@for_cond25:
    lda $28
    ldx $29
    cmp.16 .AX, #8
    bcc @for_body26
    bra @for_cond29_ph
@for_body26:
    lda #6
    ldx #0
    sta $2C
    stx $2D
    lda $28
    ldx $29
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $26
    ldx $26+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2E
    stx $2E+1
    ldy #0
    lda ($2E),y
    ldx #0
    sta $30
    ; [peephole-opt]     lda $30
    ldx #0
    ldx #0
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    ; [peephole-opt]     ldx $33
    sta _test_phase94_array__local_2
    stx _test_phase94_array__local_2+1
    lda #6
    ldx #0
    sta $34
    stx $35
    lda $28
    ldx $29
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
    sta $36
    stx $36+1
    lda $36
    ldx $37
    add.16 .AX, #1
    sta $38
    stx $39
    ldy #0
    lda ($38),y
    ldx #0
    sta $3A
    ; [peephole-opt]     lda $3A
    ldx #0
    ldx #0
    sta $3C
    stx $3D
    ; [peephole-opt]     lda $3C
    ; [peephole-opt]     ldx $3D
    sta _test_phase94_array__local_4
    stx _test_phase94_array__local_4+1
    lda #6
    ldx #0
    sta $3E
    stx $3F
    lda $28
    ldx $29
    mul.16 .AX, #3
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
    sta $40
    stx $40+1
    lda $40
    ldx $41
    add.16 .AX, #2
    sta $42
    stx $43
    ldy #0
    lda ($42),y
    ldx #0
    sta $44
    ; [peephole-opt]     lda $44
    ldx #0
    ldx #0
    sta $46
    stx $47
    ; [peephole-opt]     lda $46
    ; [peephole-opt]     ldx $47
    sta _test_phase94_array__local_6
    stx _test_phase94_array__local_6+1
@for_inc27:
    lda $28
    ldx $29
    sta $48
    stx $49
    ; [peephole-opt]     lda $48
    clc
    adc #1
    sta $28
    lda $49
    adc #0
    sta $29
    bra @for_cond25
@for_cond29_ph:
    lda #7
    ldx #0
    sta $20
    stx $21
    lda #7
    ldx #0
    sta $22
    stx $23
    lda #7
    ldx #0
    sta $24
    stx $25
    ; [peephole-opt]     lda $20
    ; [peephole-opt]     ldx $21
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $26
    stx $26+1
    ; [peephole-opt]     lda $22
    ; [peephole-opt]     ldx $23
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
    lda $24
    ldx $25
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
@for_cond29:
    lda _test_phase94_array__local_169
    ldx _test_phase94_array__local_169+1
    cmp.16 .AX, #8
    bcc @for_body30
    bra @for_end32
@for_body30:
    lda #7
    ldx #0
    sta $2A
    stx $2B
    lda _test_phase94_array__local_169
    ldx _test_phase94_array__local_169+1
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $26
    ldx $26+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2C
    stx $2C+1
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
    sta _test_phase94_array__local_2
    stx _test_phase94_array__local_2+1
    lda #7
    ldx #0
    sta $32
    stx $33
    lda _test_phase94_array__local_169
    ldx _test_phase94_array__local_169+1
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
    sta $34
    stx $34+1
    lda $34
    ldx $35
    add.16 .AX, #1
    sta $36
    stx $37
    ldy #0
    lda ($36),y
    ldx #0
    sta $38
    ; [peephole-opt]     lda $38
    ldx #0
    ldx #0
    sta $3A
    stx $3B
    ; [peephole-opt]     lda $3A
    ; [peephole-opt]     ldx $3B
    sta _test_phase94_array__local_4
    stx _test_phase94_array__local_4+1
    lda #7
    ldx #0
    sta $3C
    stx $3D
    lda _test_phase94_array__local_169
    ldx _test_phase94_array__local_169+1
    mul.16 .AX, #3
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
    sta $3E
    stx $3E+1
    lda $3E
    ldx $3F
    add.16 .AX, #2
    sta $40
    stx $41
    ldy #0
    lda ($40),y
    ldx #0
    sta $42
    ; [peephole-opt]     lda $42
    ldx #0
    ldx #0
    sta $44
    stx $45
    ; [peephole-opt]     lda $44
    ; [peephole-opt]     ldx $45
    sta _test_phase94_array__local_6
    stx _test_phase94_array__local_6+1
@for_inc31:
    lda _test_phase94_array__local_169
    ldx _test_phase94_array__local_169+1
    sta $46
    stx $47
    ; [peephole-opt]     lda $46
    ; [peephole-opt]     ldx $47
    add.16 .AX, #1
    sta _test_phase94_array__local_169
    stx _test_phase94_array__local_169+1
    bra @for_cond29
@for_end32:
    .loc "src/test-resources/test_phase95_phase94_compat.c", 63
    ldax #__str_33
    sta _test_phase94_array__local_192
    stx _test_phase94_array__local_192+1
    lda _test_phase94_array__local_6
    ldx _test_phase94_array__local_6+1
    sta $28
    stx $29
    lda _test_phase94_array__local_4
    ldx _test_phase94_array__local_4+1
    sta $2A
    stx $2B
    lda _test_phase94_array__local_2
    ldx _test_phase94_array__local_2+1
    sta $2C
    stx $2D
    lda _test_phase94_array__local_192
    ; [peephole-opt]     ldx _test_phase94_array__local_192+1
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
    .frame_size 22
    endproc

; function _test_regular_array
; SAC inline storage: 22 bytes
    _test_regular_array__local_0: .word 0
    _test_regular_array__local_2: .word 0
    _test_regular_array__local_4: .word 0
    _test_regular_array__local_6: .word 0
    _test_regular_array__local_14: .word 0
    _test_regular_array__local_37: .word 0
    _test_regular_array__local_60: .word 0
    _test_regular_array__local_83: .word 0
    _test_regular_array__local_106: .word 0
    _test_regular_array__local_129: .word 0
    _test_regular_array__local_152: .word 0
    _test_regular_array__local_169: .word 0
    _test_regular_array__local_175: .word 0
    _test_regular_array__local_192: .word 0
    proc _test_regular_array
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "src/test-resources/test_phase95_phase94_compat.c", 47
    .local @_l_b_sum = 4
    .local @_l_g_sum = 2
    .local @_l_r_sum = 0
    .local @_l_x = 20
; .debug_var: __test_regular_array @_l_b_sum offset=4 size=2 type=int16 scope=local
; .debug_var: __test_regular_array @_l_g_sum offset=2 size=2 type=int16 scope=local
; .debug_var: __test_regular_array @_l_r_sum offset=0 size=2 type=int16 scope=local
; .debug_var: __test_regular_array @_l_x offset=20 size=2 type=int16 scope=local

@entry:
    .loc "src/test-resources/test_phase95_phase94_compat.c", 68
    ldax #__str_34
    sta _test_regular_array__local_0
    stx _test_regular_array__local_0+1
    lda _test_regular_array__local_0
    ; [peephole-opt]     ldx _test_regular_array__local_0+1
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
    .loc "src/test-resources/test_phase95_phase94_compat.c", 69
    lda #0
    sta _test_regular_array__local_2
    sta _test_regular_array__local_2+1
    lda #0
    sta _test_regular_array__local_4
    sta _test_regular_array__local_4+1
    lda #0
    sta _test_regular_array__local_6
    sta _test_regular_array__local_6+1
@for_cond35_ph:
    lda #0
    sta $20
    sta $21
    lda #0
    sta $22
    sta $23
    lda #0
    sta $24
    sta $25
    ; [peephole-opt]     lda $20
    ldx $21
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $26
    stx $26+1
    ; [peephole-opt]     lda $22
    ldx $23
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
    lda $24
    ldx $25
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
@for_cond35:
    lda $28
    ldx $29
    cmp.16 .AX, #8
    bcc @for_body36
    bra @for_cond39_ph
@for_body36:
    lda #0
    sta $2C
    sta $2D
    lda $28
    ldx $29
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $26
    ldx $26+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2E
    stx $2E+1
    ldy #0
    lda ($2E),y
    ldx #0
    sta $30
    ; [peephole-opt]     lda $30
    ldx #0
    ldx #0
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    ; [peephole-opt]     ldx $33
    sta _test_regular_array__local_2
    stx _test_regular_array__local_2+1
    lda #0
    sta $34
    sta $35
    lda $28
    ldx $29
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
    sta $36
    stx $36+1
    lda $36
    ldx $37
    add.16 .AX, #1
    sta $38
    stx $39
    ldy #0
    lda ($38),y
    ldx #0
    sta $3A
    ; [peephole-opt]     lda $3A
    ldx #0
    ldx #0
    sta $3C
    stx $3D
    ; [peephole-opt]     lda $3C
    ; [peephole-opt]     ldx $3D
    sta _test_regular_array__local_4
    stx _test_regular_array__local_4+1
    lda #0
    sta $3E
    sta $3F
    lda $28
    ldx $29
    mul.16 .AX, #3
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
    sta $40
    stx $40+1
    lda $40
    ldx $41
    add.16 .AX, #2
    sta $42
    stx $43
    ldy #0
    lda ($42),y
    ldx #0
    sta $44
    ; [peephole-opt]     lda $44
    ldx #0
    ldx #0
    sta $46
    stx $47
    ; [peephole-opt]     lda $46
    ; [peephole-opt]     ldx $47
    sta _test_regular_array__local_6
    stx _test_regular_array__local_6+1
@for_inc37:
    lda $28
    ldx $29
    sta $48
    stx $49
    ; [peephole-opt]     lda $48
    clc
    adc #1
    sta $28
    lda $49
    adc #0
    sta $29
    bra @for_cond35
@for_cond39_ph:
    lda #1
    ldx #0
    sta $20
    stx $21
    lda #1
    ldx #0
    sta $22
    stx $23
    lda #1
    ldx #0
    sta $24
    stx $25
    ; [peephole-opt]     lda $20
    ; [peephole-opt]     ldx $21
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $26
    stx $26+1
    ; [peephole-opt]     lda $22
    ; [peephole-opt]     ldx $23
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
    lda $24
    ldx $25
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
@for_cond39:
    lda $28
    ldx $29
    cmp.16 .AX, #8
    bcc @for_body40
    bra @for_cond43_ph
@for_body40:
    lda #1
    ldx #0
    sta $2C
    stx $2D
    lda $28
    ldx $29
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $26
    ldx $26+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2E
    stx $2E+1
    ldy #0
    lda ($2E),y
    ldx #0
    sta $30
    ; [peephole-opt]     lda $30
    ldx #0
    ldx #0
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    ; [peephole-opt]     ldx $33
    sta _test_regular_array__local_2
    stx _test_regular_array__local_2+1
    lda #1
    ldx #0
    sta $34
    stx $35
    lda $28
    ldx $29
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
    sta $36
    stx $36+1
    lda $36
    ldx $37
    add.16 .AX, #1
    sta $38
    stx $39
    ldy #0
    lda ($38),y
    ldx #0
    sta $3A
    ; [peephole-opt]     lda $3A
    ldx #0
    ldx #0
    sta $3C
    stx $3D
    ; [peephole-opt]     lda $3C
    ; [peephole-opt]     ldx $3D
    sta _test_regular_array__local_4
    stx _test_regular_array__local_4+1
    lda #1
    ldx #0
    sta $3E
    stx $3F
    lda $28
    ldx $29
    mul.16 .AX, #3
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
    sta $40
    stx $40+1
    lda $40
    ldx $41
    add.16 .AX, #2
    sta $42
    stx $43
    ldy #0
    lda ($42),y
    ldx #0
    sta $44
    ; [peephole-opt]     lda $44
    ldx #0
    ldx #0
    sta $46
    stx $47
    ; [peephole-opt]     lda $46
    ; [peephole-opt]     ldx $47
    sta _test_regular_array__local_6
    stx _test_regular_array__local_6+1
@for_inc41:
    lda $28
    ldx $29
    sta $48
    stx $49
    ; [peephole-opt]     lda $48
    clc
    adc #1
    sta $28
    lda $49
    adc #0
    sta $29
    bra @for_cond39
@for_cond43_ph:
    lda #2
    ldx #0
    sta $20
    stx $21
    lda #2
    ldx #0
    sta $22
    stx $23
    lda #2
    ldx #0
    sta $24
    stx $25
    ; [peephole-opt]     lda $20
    ; [peephole-opt]     ldx $21
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $26
    stx $26+1
    ; [peephole-opt]     lda $22
    ; [peephole-opt]     ldx $23
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
    lda $24
    ldx $25
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
@for_cond43:
    lda $28
    ldx $29
    cmp.16 .AX, #8
    bcc @for_body44
    bra @for_cond47_ph
@for_body44:
    lda #2
    ldx #0
    sta $2C
    stx $2D
    lda $28
    ldx $29
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $26
    ldx $26+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2E
    stx $2E+1
    ldy #0
    lda ($2E),y
    ldx #0
    sta $30
    ; [peephole-opt]     lda $30
    ldx #0
    ldx #0
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    ; [peephole-opt]     ldx $33
    sta _test_regular_array__local_2
    stx _test_regular_array__local_2+1
    lda #2
    ldx #0
    sta $34
    stx $35
    lda $28
    ldx $29
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
    sta $36
    stx $36+1
    lda $36
    ldx $37
    add.16 .AX, #1
    sta $38
    stx $39
    ldy #0
    lda ($38),y
    ldx #0
    sta $3A
    ; [peephole-opt]     lda $3A
    ldx #0
    ldx #0
    sta $3C
    stx $3D
    ; [peephole-opt]     lda $3C
    ; [peephole-opt]     ldx $3D
    sta _test_regular_array__local_4
    stx _test_regular_array__local_4+1
    lda #2
    ldx #0
    sta $3E
    stx $3F
    lda $28
    ldx $29
    mul.16 .AX, #3
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
    sta $40
    stx $40+1
    lda $40
    ldx $41
    add.16 .AX, #2
    sta $42
    stx $43
    ldy #0
    lda ($42),y
    ldx #0
    sta $44
    ; [peephole-opt]     lda $44
    ldx #0
    ldx #0
    sta $46
    stx $47
    ; [peephole-opt]     lda $46
    ; [peephole-opt]     ldx $47
    sta _test_regular_array__local_6
    stx _test_regular_array__local_6+1
@for_inc45:
    lda $28
    ldx $29
    sta $48
    stx $49
    ; [peephole-opt]     lda $48
    clc
    adc #1
    sta $28
    lda $49
    adc #0
    sta $29
    bra @for_cond43
@for_cond47_ph:
    lda #3
    ldx #0
    sta $20
    stx $21
    lda #3
    ldx #0
    sta $22
    stx $23
    lda #3
    ldx #0
    sta $24
    stx $25
    ; [peephole-opt]     lda $20
    ; [peephole-opt]     ldx $21
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $26
    stx $26+1
    ; [peephole-opt]     lda $22
    ; [peephole-opt]     ldx $23
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
    lda $24
    ldx $25
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
@for_cond47:
    lda $28
    ldx $29
    cmp.16 .AX, #8
    bcc @for_body48
    bra @for_cond51_ph
@for_body48:
    lda #3
    ldx #0
    sta $2C
    stx $2D
    lda $28
    ldx $29
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $26
    ldx $26+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2E
    stx $2E+1
    ldy #0
    lda ($2E),y
    ldx #0
    sta $30
    ; [peephole-opt]     lda $30
    ldx #0
    ldx #0
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    ; [peephole-opt]     ldx $33
    sta _test_regular_array__local_2
    stx _test_regular_array__local_2+1
    lda #3
    ldx #0
    sta $34
    stx $35
    lda $28
    ldx $29
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
    sta $36
    stx $36+1
    lda $36
    ldx $37
    add.16 .AX, #1
    sta $38
    stx $39
    ldy #0
    lda ($38),y
    ldx #0
    sta $3A
    ; [peephole-opt]     lda $3A
    ldx #0
    ldx #0
    sta $3C
    stx $3D
    ; [peephole-opt]     lda $3C
    ; [peephole-opt]     ldx $3D
    sta _test_regular_array__local_4
    stx _test_regular_array__local_4+1
    lda #3
    ldx #0
    sta $3E
    stx $3F
    lda $28
    ldx $29
    mul.16 .AX, #3
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
    sta $40
    stx $40+1
    lda $40
    ldx $41
    add.16 .AX, #2
    sta $42
    stx $43
    ldy #0
    lda ($42),y
    ldx #0
    sta $44
    ; [peephole-opt]     lda $44
    ldx #0
    ldx #0
    sta $46
    stx $47
    ; [peephole-opt]     lda $46
    ; [peephole-opt]     ldx $47
    sta _test_regular_array__local_6
    stx _test_regular_array__local_6+1
@for_inc49:
    lda $28
    ldx $29
    sta $48
    stx $49
    ; [peephole-opt]     lda $48
    clc
    adc #1
    sta $28
    lda $49
    adc #0
    sta $29
    bra @for_cond47
@for_cond51_ph:
    lda #4
    ldx #0
    sta $20
    stx $21
    lda #4
    ldx #0
    sta $22
    stx $23
    lda #4
    ldx #0
    sta $24
    stx $25
    ; [peephole-opt]     lda $20
    ; [peephole-opt]     ldx $21
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $26
    stx $26+1
    ; [peephole-opt]     lda $22
    ; [peephole-opt]     ldx $23
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
    lda $24
    ldx $25
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
@for_cond51:
    lda $28
    ldx $29
    cmp.16 .AX, #8
    bcc @for_body52
    bra @for_cond55_ph
@for_body52:
    lda #4
    ldx #0
    sta $2C
    stx $2D
    lda $28
    ldx $29
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $26
    ldx $26+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2E
    stx $2E+1
    ldy #0
    lda ($2E),y
    ldx #0
    sta $30
    ; [peephole-opt]     lda $30
    ldx #0
    ldx #0
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    ; [peephole-opt]     ldx $33
    sta _test_regular_array__local_2
    stx _test_regular_array__local_2+1
    lda #4
    ldx #0
    sta $34
    stx $35
    lda $28
    ldx $29
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
    sta $36
    stx $36+1
    lda $36
    ldx $37
    add.16 .AX, #1
    sta $38
    stx $39
    ldy #0
    lda ($38),y
    ldx #0
    sta $3A
    ; [peephole-opt]     lda $3A
    ldx #0
    ldx #0
    sta $3C
    stx $3D
    ; [peephole-opt]     lda $3C
    ; [peephole-opt]     ldx $3D
    sta _test_regular_array__local_4
    stx _test_regular_array__local_4+1
    lda #4
    ldx #0
    sta $3E
    stx $3F
    lda $28
    ldx $29
    mul.16 .AX, #3
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
    sta $40
    stx $40+1
    lda $40
    ldx $41
    add.16 .AX, #2
    sta $42
    stx $43
    ldy #0
    lda ($42),y
    ldx #0
    sta $44
    ; [peephole-opt]     lda $44
    ldx #0
    ldx #0
    sta $46
    stx $47
    ; [peephole-opt]     lda $46
    ; [peephole-opt]     ldx $47
    sta _test_regular_array__local_6
    stx _test_regular_array__local_6+1
@for_inc53:
    lda $28
    ldx $29
    sta $48
    stx $49
    ; [peephole-opt]     lda $48
    clc
    adc #1
    sta $28
    lda $49
    adc #0
    sta $29
    bra @for_cond51
@for_cond55_ph:
    lda #5
    ldx #0
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
    ; [peephole-opt]     lda $20
    ; [peephole-opt]     ldx $21
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $26
    stx $26+1
    ; [peephole-opt]     lda $22
    ; [peephole-opt]     ldx $23
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
    lda $24
    ldx $25
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
@for_cond55:
    lda $28
    ldx $29
    cmp.16 .AX, #8
    bcc @for_body56
    bra @for_cond59_ph
@for_body56:
    lda #5
    ldx #0
    sta $2C
    stx $2D
    lda $28
    ldx $29
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $26
    ldx $26+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2E
    stx $2E+1
    ldy #0
    lda ($2E),y
    ldx #0
    sta $30
    ; [peephole-opt]     lda $30
    ldx #0
    ldx #0
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    ; [peephole-opt]     ldx $33
    sta _test_regular_array__local_2
    stx _test_regular_array__local_2+1
    lda #5
    ldx #0
    sta $34
    stx $35
    lda $28
    ldx $29
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
    sta $36
    stx $36+1
    lda $36
    ldx $37
    add.16 .AX, #1
    sta $38
    stx $39
    ldy #0
    lda ($38),y
    ldx #0
    sta $3A
    ; [peephole-opt]     lda $3A
    ldx #0
    ldx #0
    sta $3C
    stx $3D
    ; [peephole-opt]     lda $3C
    ; [peephole-opt]     ldx $3D
    sta _test_regular_array__local_4
    stx _test_regular_array__local_4+1
    lda #5
    ldx #0
    sta $3E
    stx $3F
    lda $28
    ldx $29
    mul.16 .AX, #3
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
    sta $40
    stx $40+1
    lda $40
    ldx $41
    add.16 .AX, #2
    sta $42
    stx $43
    ldy #0
    lda ($42),y
    ldx #0
    sta $44
    ; [peephole-opt]     lda $44
    ldx #0
    ldx #0
    sta $46
    stx $47
    ; [peephole-opt]     lda $46
    ; [peephole-opt]     ldx $47
    sta _test_regular_array__local_6
    stx _test_regular_array__local_6+1
@for_inc57:
    lda $28
    ldx $29
    sta $48
    stx $49
    ; [peephole-opt]     lda $48
    clc
    adc #1
    sta $28
    lda $49
    adc #0
    sta $29
    bra @for_cond55
@for_cond59_ph:
    lda #6
    ldx #0
    sta $20
    stx $21
    lda #6
    ldx #0
    sta $22
    stx $23
    lda #6
    ldx #0
    sta $24
    stx $25
    ; [peephole-opt]     lda $20
    ; [peephole-opt]     ldx $21
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $26
    stx $26+1
    ; [peephole-opt]     lda $22
    ; [peephole-opt]     ldx $23
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
    lda $24
    ldx $25
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
@for_cond59:
    lda $28
    ldx $29
    cmp.16 .AX, #8
    bcc @for_body60
    bra @for_cond63_ph
@for_body60:
    lda #6
    ldx #0
    sta $2C
    stx $2D
    lda $28
    ldx $29
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $26
    ldx $26+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2E
    stx $2E+1
    ldy #0
    lda ($2E),y
    ldx #0
    sta $30
    ; [peephole-opt]     lda $30
    ldx #0
    ldx #0
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    ; [peephole-opt]     ldx $33
    sta _test_regular_array__local_2
    stx _test_regular_array__local_2+1
    lda #6
    ldx #0
    sta $34
    stx $35
    lda $28
    ldx $29
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
    sta $36
    stx $36+1
    lda $36
    ldx $37
    add.16 .AX, #1
    sta $38
    stx $39
    ldy #0
    lda ($38),y
    ldx #0
    sta $3A
    ; [peephole-opt]     lda $3A
    ldx #0
    ldx #0
    sta $3C
    stx $3D
    ; [peephole-opt]     lda $3C
    ; [peephole-opt]     ldx $3D
    sta _test_regular_array__local_4
    stx _test_regular_array__local_4+1
    lda #6
    ldx #0
    sta $3E
    stx $3F
    lda $28
    ldx $29
    mul.16 .AX, #3
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
    sta $40
    stx $40+1
    lda $40
    ldx $41
    add.16 .AX, #2
    sta $42
    stx $43
    ldy #0
    lda ($42),y
    ldx #0
    sta $44
    ; [peephole-opt]     lda $44
    ldx #0
    ldx #0
    sta $46
    stx $47
    ; [peephole-opt]     lda $46
    ; [peephole-opt]     ldx $47
    sta _test_regular_array__local_6
    stx _test_regular_array__local_6+1
@for_inc61:
    lda $28
    ldx $29
    sta $48
    stx $49
    ; [peephole-opt]     lda $48
    clc
    adc #1
    sta $28
    lda $49
    adc #0
    sta $29
    bra @for_cond59
@for_cond63_ph:
    lda #7
    ldx #0
    sta $20
    stx $21
    lda #7
    ldx #0
    sta $22
    stx $23
    lda #7
    ldx #0
    sta $24
    stx $25
    ; [peephole-opt]     lda $20
    ; [peephole-opt]     ldx $21
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $26
    stx $26+1
    ; [peephole-opt]     lda $22
    ; [peephole-opt]     ldx $23
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
    lda $24
    ldx $25
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
@for_cond63:
    lda _test_regular_array__local_169
    ldx _test_regular_array__local_169+1
    cmp.16 .AX, #8
    bcc @for_body64
    bra @for_end66
@for_body64:
    lda #7
    ldx #0
    sta $2A
    stx $2B
    lda _test_regular_array__local_169
    ldx _test_regular_array__local_169+1
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $26
    ldx $26+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2C
    stx $2C+1
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
    sta _test_regular_array__local_2
    stx _test_regular_array__local_2+1
    lda #7
    ldx #0
    sta $32
    stx $33
    lda _test_regular_array__local_169
    ldx _test_regular_array__local_169+1
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
    sta $34
    stx $34+1
    lda $34
    ldx $35
    add.16 .AX, #1
    sta $36
    stx $37
    ldy #0
    lda ($36),y
    ldx #0
    sta $38
    ; [peephole-opt]     lda $38
    ldx #0
    ldx #0
    sta $3A
    stx $3B
    ; [peephole-opt]     lda $3A
    ; [peephole-opt]     ldx $3B
    sta _test_regular_array__local_4
    stx _test_regular_array__local_4+1
    lda #7
    ldx #0
    sta $3C
    stx $3D
    lda _test_regular_array__local_169
    ldx _test_regular_array__local_169+1
    mul.16 .AX, #3
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
    sta $3E
    stx $3E+1
    lda $3E
    ldx $3F
    add.16 .AX, #2
    sta $40
    stx $41
    ldy #0
    lda ($40),y
    ldx #0
    sta $42
    ; [peephole-opt]     lda $42
    ldx #0
    ldx #0
    sta $44
    stx $45
    ; [peephole-opt]     lda $44
    ; [peephole-opt]     ldx $45
    sta _test_regular_array__local_6
    stx _test_regular_array__local_6+1
@for_inc65:
    lda _test_regular_array__local_169
    ldx _test_regular_array__local_169+1
    sta $46
    stx $47
    ; [peephole-opt]     lda $46
    ; [peephole-opt]     ldx $47
    add.16 .AX, #1
    sta _test_regular_array__local_169
    stx _test_regular_array__local_169+1
    bra @for_cond63
@for_end66:
    .loc "src/test-resources/test_phase95_phase94_compat.c", 77
    ldax #__str_67
    sta _test_regular_array__local_192
    stx _test_regular_array__local_192+1
    lda _test_regular_array__local_6
    ldx _test_regular_array__local_6+1
    sta $28
    stx $29
    lda _test_regular_array__local_4
    ldx _test_regular_array__local_4+1
    sta $2A
    stx $2B
    lda _test_regular_array__local_2
    ldx _test_regular_array__local_2+1
    sta $2C
    stx $2D
    lda _test_regular_array__local_192
    ; [peephole-opt]     ldx _test_regular_array__local_192+1
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
    .frame_size 22
    endproc

; function _verify_data_integrity
; SAC inline storage: 6 bytes
    _verify_data_integrity__local_0: .word 0
    _verify_data_integrity__local_2: .word 0
    _verify_data_integrity__local_4: .word 0
    _verify_data_integrity__local_8: .word 0
    _verify_data_integrity__local_14: .word 0
    _verify_data_integrity__local_18: .word 0
    _verify_data_integrity__local_50: .word 0
    _verify_data_integrity__local_51: .word 0
    _verify_data_integrity__local_53: .word 0
    proc _verify_data_integrity
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "src/test-resources/test_phase95_phase94_compat.c", 61
    .local @_l_match = 0
    .local @_l_x = 4
    .local @_l_y = 2
; .debug_var: __verify_data_integrity @_l_match offset=0 size=2 type=int16 scope=local
; .debug_var: __verify_data_integrity @_l_x offset=4 size=2 type=int16 scope=local
; .debug_var: __verify_data_integrity @_l_y offset=2 size=2 type=int16 scope=local

@entry:
    .loc "src/test-resources/test_phase95_phase94_compat.c", 82
    ldax #__str_68
    sta _verify_data_integrity__local_0
    stx _verify_data_integrity__local_0+1
    lda _verify_data_integrity__local_0
    ; [peephole-opt]     ldx _verify_data_integrity__local_0+1
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
    .loc "src/test-resources/test_phase95_phase94_compat.c", 83
    lda #1
    sta _verify_data_integrity__local_2
    lda #0
    sta _verify_data_integrity__local_2+1
    .loc "src/test-resources/test_phase95_phase94_compat.c", 84
    lda #0
    sta _verify_data_integrity__local_4
    sta _verify_data_integrity__local_4+1
@for_cond69:
    lda _verify_data_integrity__local_4
    ldx _verify_data_integrity__local_4+1
    cmp.16 .AX, #8
    bcc @for_body70
    bra @for_end72
@for_body70:
    .loc "src/test-resources/test_phase95_phase94_compat.c", 85
    lda #0
    sta _verify_data_integrity__local_8
    sta _verify_data_integrity__local_8+1
@for_cond73_ph:
    .loc "src/test-resources/test_phase95_phase94_compat.c", 86
    lda _verify_data_integrity__local_4
    ldx _verify_data_integrity__local_4+1
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $26
    stx $26+1
    lda _verify_data_integrity__local_4
    ldx _verify_data_integrity__local_4+1
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $28
    stx $28+1
    .loc "src/test-resources/test_phase95_phase94_compat.c", 87
    lda _verify_data_integrity__local_4
    ldx _verify_data_integrity__local_4+1
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2A
    stx $2A+1
    lda _verify_data_integrity__local_4
    ldx _verify_data_integrity__local_4+1
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2C
    stx $2C+1
    .loc "src/test-resources/test_phase95_phase94_compat.c", 88
    lda _verify_data_integrity__local_4
    ldx _verify_data_integrity__local_4+1
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2E
    stx $2E+1
    lda _verify_data_integrity__local_4
    ldx _verify_data_integrity__local_4+1
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $30
    stx $30+1
@for_cond73:
    .loc "src/test-resources/test_phase95_phase94_compat.c", 85
    lda _verify_data_integrity__local_8
    ldx _verify_data_integrity__local_8+1
    cmp.16 .AX, #8
    bcc @for_body74
    bra @for_end76
@for_body74:
    .loc "src/test-resources/test_phase95_phase94_compat.c", 86
    lda _verify_data_integrity__local_8
    ldx _verify_data_integrity__local_8+1
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $26
    ldx $26+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $36
    stx $36+1
    ldy #0
    lda ($36),y
    ldx #0
    sta $38
    lda _verify_data_integrity__local_8
    ldx _verify_data_integrity__local_8+1
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $28
    ldx $28+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $3A
    stx $3A+1
    ldy #0
    lda ($3A),y
    ldx #0
    sta $3C
    ; [peephole-opt]     lda $38
    ldx #0
    ldx #0
    sta $3E
    stx $3F
    ; [peephole-opt]     lda $3C
    ldx #0
    ldx #0
    sta $40
    stx $41
    ; [peephole-opt]     lda $3E
    ; [peephole-opt]     ldx $3F
    cmp.16 .AX, $40
    bne @if_then77
@or_rhs81:
    .loc "src/test-resources/test_phase95_phase94_compat.c", 87
    lda _verify_data_integrity__local_8
    ldx _verify_data_integrity__local_8+1
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $2A
    ldx $2A+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $44
    stx $44+1
    lda $44
    ldx $45
    add.16 .AX, #1
    sta $46
    stx $47
    ldy #0
    lda ($46),y
    ldx #0
    sta $48
    lda _verify_data_integrity__local_8
    ldx _verify_data_integrity__local_8+1
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $2C
    ldx $2C+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $4A
    stx $4A+1
    lda $4A
    ldx $4B
    add.16 .AX, #1
    sta $4C
    stx $4D
    ldy #0
    lda ($4C),y
    ldx #0
    sta $4E
    ; [peephole-opt]     lda $48
    ldx #0
    ldx #0
    sta $50
    stx $51
    ; [peephole-opt]     lda $4E
    ldx #0
    ldx #0
    sta $52
    stx $53
    ; [peephole-opt]     lda $50
    ; [peephole-opt]     ldx $51
    cmp.16 .AX, $52
    .loc "src/test-resources/test_phase95_phase94_compat.c", 86
    bne @if_then77
@or_rhs80:
    .loc "src/test-resources/test_phase95_phase94_compat.c", 88
    lda _verify_data_integrity__local_8
    ldx _verify_data_integrity__local_8+1
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $2E
    ldx $2E+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $56
    stx $56+1
    lda $56
    ldx $57
    add.16 .AX, #2
    sta $58
    stx $59
    ldy #0
    lda ($58),y
    ldx #0
    sta $5A
    lda _verify_data_integrity__local_8
    ldx _verify_data_integrity__local_8+1
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $30
    ldx $30+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $5C
    stx $5C+1
    lda $5C
    ldx $5D
    add.16 .AX, #2
    sta $5E
    stx $5F
    ldy #0
    lda ($5E),y
    ldx #0
    sta $60
    ; [peephole-opt]     lda $5A
    ldx #0
    ldx #0
    sta $62
    stx $63
    ; [peephole-opt]     lda $60
    ldx #0
    ldx #0
    sta $64
    stx $65
    ; [peephole-opt]     lda $62
    ; [peephole-opt]     ldx $63
    cmp.16 .AX, $64
    .loc "src/test-resources/test_phase95_phase94_compat.c", 86
    bne @if_then77
    bra @if_end79
@if_then77:
    .loc "src/test-resources/test_phase95_phase94_compat.c", 89
    lda #0
    sta _verify_data_integrity__local_2
    sta _verify_data_integrity__local_2+1
    bra @for_end76
@if_end79:
@for_inc75:
    .loc "src/test-resources/test_phase95_phase94_compat.c", 85
    lda _verify_data_integrity__local_8
    ldx _verify_data_integrity__local_8+1
    sta $6A
    stx $6B
    ; [peephole-opt]     lda $6A
    clc
    adc #1
    sta $6C
    lda $6B
    adc #0
    sta $6D
    ; [peephole-opt]     lda $6C
    ldx $6D
    sta _verify_data_integrity__local_8
    stx _verify_data_integrity__local_8+1
    bra @for_cond73
@for_end76:
    .loc "src/test-resources/test_phase95_phase94_compat.c", 93
    lda _verify_data_integrity__local_2
    ldx _verify_data_integrity__local_2+1
    bne @if_end84
    cmp #$00
    bne @if_end84
    bra @for_end72
@if_end84:
@for_inc71:
    .loc "src/test-resources/test_phase95_phase94_compat.c", 84
    lda _verify_data_integrity__local_4
    ldx _verify_data_integrity__local_4+1
    sta $6E
    stx $6F
    ; [peephole-opt]     lda $6E
    ; [peephole-opt]     ldx $6F
    add.16 .AX, #1
    sta _verify_data_integrity__local_4
    stx _verify_data_integrity__local_4+1
    bra @for_cond69
@for_end72:
    .loc "src/test-resources/test_phase95_phase94_compat.c", 95
    ldax #__str_85
    sta _verify_data_integrity__local_50
    stx _verify_data_integrity__local_50+1
    lda _verify_data_integrity__local_2
    ldx _verify_data_integrity__local_2+1
    bne @tern_then86
    cmp #$00
    bne @tern_then86
    bra @tern_else87
@tern_then86:
    ldax #__str_89
    bra @tern_end88
@tern_else87:
    ldax #__str_90
    sta _verify_data_integrity__local_53
    stx _verify_data_integrity__local_53+1
@tern_end88:
    lda _verify_data_integrity__local_53
    ; [peephole-opt]     ldx _verify_data_integrity__local_53+1
    sta $28
    stx $29
    lda _verify_data_integrity__local_50
    ldx _verify_data_integrity__local_50+1
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
    .frame_size 6
    endproc

; function _main
; SAC inline storage: 50 bytes
    _main__local_0: .word 0
    _main__local_2: .word 0
    _main__local_4: .word 0
    _main__local_6: .word 0
    _main__local_8: .word 0
    _main__local_16: .word 0
    _main__local_39: .word 0
    _main__local_62: .word 0
    _main__local_85: .word 0
    _main__local_108: .word 0
    _main__local_131: .word 0
    _main__local_154: .word 0
    _main__local_177: .word 0
    _main__local_194: .word 0
    _main__local_196: .word 0
    _main__local_198: .word 0
    _main__local_200: .word 0
    _main__local_202: .word 0
    _main__local_210: .word 0
    _main__local_233: .word 0
    _main__local_256: .word 0
    _main__local_279: .word 0
    _main__local_302: .word 0
    _main__local_325: .word 0
    _main__local_348: .word 0
    _main__local_371: .word 0
    _main__local_388: .word 0
    _main__local_390: .word 0
    _main__local_392: .word 0
    _main__local_394: .word 0
    _main__local_398: .word 0
    _main__local_404: .word 0
    _main__local_408: .word 0
    _main__local_440: .word 0
    _main__local_441: .word 0
    _main__local_443: .word 0
    _main__local_445: .word 0
    _main__local_446: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "src/test-resources/test_phase95_phase94_compat.c", 79
    .local @_l_b_sum = 26
    .local @_l_g_sum = 24
    .local @_l_match = 44
    .local @_l_r_sum = 22
    .local @_l_x = 48
    .local @_l_y = 46
; .debug_var: __main @_l_b_sum offset=26 size=2 type=int16 scope=local
; .debug_var: __main @_l_g_sum offset=24 size=2 type=int16 scope=local
; .debug_var: __main @_l_match offset=44 size=2 type=int16 scope=local
; .debug_var: __main @_l_r_sum offset=22 size=2 type=int16 scope=local
; .debug_var: __main @_l_x offset=48 size=2 type=int16 scope=local
; .debug_var: __main @_l_y offset=46 size=2 type=int16 scope=local

@entry:
    .loc "src/test-resources/test_phase95_phase94_compat.c", 99
    ldax #__str_91
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
    .loc "src/test-resources/test_phase95_phase94_compat.c", 54
    ldax #__str_93
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
    .loc "src/test-resources/test_phase95_phase94_compat.c", 55
    lda #0
    sta _main__local_4
    sta _main__local_4+1
    lda #0
    sta _main__local_6
    sta _main__local_6+1
    lda #0
    sta _main__local_8
    sta _main__local_8+1
@for_cond94_ph:
    lda #0
    sta $20
    sta $21
    lda #0
    sta $22
    sta $23
    lda #0
    sta $24
    sta $25
    ; [peephole-opt]     lda $20
    ldx $21
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $26
    stx $26+1
    ; [peephole-opt]     lda $22
    ldx $23
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
    lda $24
    ldx $25
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
@for_cond94:
    lda $28
    ldx $29
    cmp.16 .AX, #8
    bcc @for_body95
    bra @for_cond98_ph
@for_body95:
    lda #0
    sta $2C
    sta $2D
    lda $28
    ldx $29
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $26
    ldx $26+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2E
    stx $2E+1
    ldy #0
    lda ($2E),y
    ldx #0
    sta $30
    ; [peephole-opt]     lda $30
    ldx #0
    ldx #0
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    ; [peephole-opt]     ldx $33
    sta _main__local_4
    stx _main__local_4+1
    lda #0
    sta $34
    sta $35
    lda $28
    ldx $29
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
    sta $36
    stx $36+1
    lda $36
    ldx $37
    add.16 .AX, #1
    sta $38
    stx $39
    ldy #0
    lda ($38),y
    ldx #0
    sta $3A
    ; [peephole-opt]     lda $3A
    ldx #0
    ldx #0
    sta $3C
    stx $3D
    ; [peephole-opt]     lda $3C
    ; [peephole-opt]     ldx $3D
    sta _main__local_6
    stx _main__local_6+1
    lda #0
    sta $3E
    sta $3F
    lda $28
    ldx $29
    mul.16 .AX, #3
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
    sta $40
    stx $40+1
    lda $40
    ldx $41
    add.16 .AX, #2
    sta $42
    stx $43
    ldy #0
    lda ($42),y
    ldx #0
    sta $44
    ; [peephole-opt]     lda $44
    ldx #0
    ldx #0
    sta $46
    stx $47
    ; [peephole-opt]     lda $46
    ; [peephole-opt]     ldx $47
    sta _main__local_8
    stx _main__local_8+1
@for_inc96:
    lda $28
    ldx $29
    sta $48
    stx $49
    ; [peephole-opt]     lda $48
    clc
    adc #1
    sta $28
    lda $49
    adc #0
    sta $29
    bra @for_cond94
@for_cond98_ph:
    lda #1
    ldx #0
    sta $20
    stx $21
    lda #1
    ldx #0
    sta $22
    stx $23
    lda #1
    ldx #0
    sta $24
    stx $25
    ; [peephole-opt]     lda $20
    ; [peephole-opt]     ldx $21
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $26
    stx $26+1
    ; [peephole-opt]     lda $22
    ; [peephole-opt]     ldx $23
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
    lda $24
    ldx $25
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
@for_cond98:
    lda $28
    ldx $29
    cmp.16 .AX, #8
    bcc @for_body99
    bra @for_cond102_ph
@for_body99:
    lda #1
    ldx #0
    sta $2C
    stx $2D
    lda $28
    ldx $29
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $26
    ldx $26+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2E
    stx $2E+1
    ldy #0
    lda ($2E),y
    ldx #0
    sta $30
    ; [peephole-opt]     lda $30
    ldx #0
    ldx #0
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    ; [peephole-opt]     ldx $33
    sta _main__local_4
    stx _main__local_4+1
    lda #1
    ldx #0
    sta $34
    stx $35
    lda $28
    ldx $29
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
    sta $36
    stx $36+1
    lda $36
    ldx $37
    add.16 .AX, #1
    sta $38
    stx $39
    ldy #0
    lda ($38),y
    ldx #0
    sta $3A
    ; [peephole-opt]     lda $3A
    ldx #0
    ldx #0
    sta $3C
    stx $3D
    ; [peephole-opt]     lda $3C
    ; [peephole-opt]     ldx $3D
    sta _main__local_6
    stx _main__local_6+1
    lda #1
    ldx #0
    sta $3E
    stx $3F
    lda $28
    ldx $29
    mul.16 .AX, #3
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
    sta $40
    stx $40+1
    lda $40
    ldx $41
    add.16 .AX, #2
    sta $42
    stx $43
    ldy #0
    lda ($42),y
    ldx #0
    sta $44
    ; [peephole-opt]     lda $44
    ldx #0
    ldx #0
    sta $46
    stx $47
    ; [peephole-opt]     lda $46
    ; [peephole-opt]     ldx $47
    sta _main__local_8
    stx _main__local_8+1
@for_inc100:
    lda $28
    ldx $29
    sta $48
    stx $49
    ; [peephole-opt]     lda $48
    clc
    adc #1
    sta $28
    lda $49
    adc #0
    sta $29
    bra @for_cond98
@for_cond102_ph:
    lda #2
    ldx #0
    sta $20
    stx $21
    lda #2
    ldx #0
    sta $22
    stx $23
    lda #2
    ldx #0
    sta $24
    stx $25
    ; [peephole-opt]     lda $20
    ; [peephole-opt]     ldx $21
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $26
    stx $26+1
    ; [peephole-opt]     lda $22
    ; [peephole-opt]     ldx $23
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
    lda $24
    ldx $25
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
@for_cond102:
    lda $28
    ldx $29
    cmp.16 .AX, #8
    bcc @for_body103
    bra @for_cond106_ph
@for_body103:
    lda #2
    ldx #0
    sta $2C
    stx $2D
    lda $28
    ldx $29
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $26
    ldx $26+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2E
    stx $2E+1
    ldy #0
    lda ($2E),y
    ldx #0
    sta $30
    ; [peephole-opt]     lda $30
    ldx #0
    ldx #0
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    ; [peephole-opt]     ldx $33
    sta _main__local_4
    stx _main__local_4+1
    lda #2
    ldx #0
    sta $34
    stx $35
    lda $28
    ldx $29
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
    sta $36
    stx $36+1
    lda $36
    ldx $37
    add.16 .AX, #1
    sta $38
    stx $39
    ldy #0
    lda ($38),y
    ldx #0
    sta $3A
    ; [peephole-opt]     lda $3A
    ldx #0
    ldx #0
    sta $3C
    stx $3D
    ; [peephole-opt]     lda $3C
    ; [peephole-opt]     ldx $3D
    sta _main__local_6
    stx _main__local_6+1
    lda #2
    ldx #0
    sta $3E
    stx $3F
    lda $28
    ldx $29
    mul.16 .AX, #3
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
    sta $40
    stx $40+1
    lda $40
    ldx $41
    add.16 .AX, #2
    sta $42
    stx $43
    ldy #0
    lda ($42),y
    ldx #0
    sta $44
    ; [peephole-opt]     lda $44
    ldx #0
    ldx #0
    sta $46
    stx $47
    ; [peephole-opt]     lda $46
    ; [peephole-opt]     ldx $47
    sta _main__local_8
    stx _main__local_8+1
@for_inc104:
    lda $28
    ldx $29
    sta $48
    stx $49
    ; [peephole-opt]     lda $48
    clc
    adc #1
    sta $28
    lda $49
    adc #0
    sta $29
    bra @for_cond102
@for_cond106_ph:
    lda #3
    ldx #0
    sta $20
    stx $21
    lda #3
    ldx #0
    sta $22
    stx $23
    lda #3
    ldx #0
    sta $24
    stx $25
    ; [peephole-opt]     lda $20
    ; [peephole-opt]     ldx $21
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $26
    stx $26+1
    ; [peephole-opt]     lda $22
    ; [peephole-opt]     ldx $23
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
    lda $24
    ldx $25
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
@for_cond106:
    lda $28
    ldx $29
    cmp.16 .AX, #8
    bcc @for_body107
    bra @for_cond110_ph
@for_body107:
    lda #3
    ldx #0
    sta $2C
    stx $2D
    lda $28
    ldx $29
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $26
    ldx $26+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2E
    stx $2E+1
    ldy #0
    lda ($2E),y
    ldx #0
    sta $30
    ; [peephole-opt]     lda $30
    ldx #0
    ldx #0
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    ; [peephole-opt]     ldx $33
    sta _main__local_4
    stx _main__local_4+1
    lda #3
    ldx #0
    sta $34
    stx $35
    lda $28
    ldx $29
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
    sta $36
    stx $36+1
    lda $36
    ldx $37
    add.16 .AX, #1
    sta $38
    stx $39
    ldy #0
    lda ($38),y
    ldx #0
    sta $3A
    ; [peephole-opt]     lda $3A
    ldx #0
    ldx #0
    sta $3C
    stx $3D
    ; [peephole-opt]     lda $3C
    ; [peephole-opt]     ldx $3D
    sta _main__local_6
    stx _main__local_6+1
    lda #3
    ldx #0
    sta $3E
    stx $3F
    lda $28
    ldx $29
    mul.16 .AX, #3
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
    sta $40
    stx $40+1
    lda $40
    ldx $41
    add.16 .AX, #2
    sta $42
    stx $43
    ldy #0
    lda ($42),y
    ldx #0
    sta $44
    ; [peephole-opt]     lda $44
    ldx #0
    ldx #0
    sta $46
    stx $47
    ; [peephole-opt]     lda $46
    ; [peephole-opt]     ldx $47
    sta _main__local_8
    stx _main__local_8+1
@for_inc108:
    lda $28
    ldx $29
    sta $48
    stx $49
    ; [peephole-opt]     lda $48
    clc
    adc #1
    sta $28
    lda $49
    adc #0
    sta $29
    bra @for_cond106
@for_cond110_ph:
    lda #4
    ldx #0
    sta $20
    stx $21
    lda #4
    ldx #0
    sta $22
    stx $23
    lda #4
    ldx #0
    sta $24
    stx $25
    ; [peephole-opt]     lda $20
    ; [peephole-opt]     ldx $21
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $26
    stx $26+1
    ; [peephole-opt]     lda $22
    ; [peephole-opt]     ldx $23
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
    lda $24
    ldx $25
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
@for_cond110:
    lda $28
    ldx $29
    cmp.16 .AX, #8
    bcc @for_body111
    bra @for_cond114_ph
@for_body111:
    lda #4
    ldx #0
    sta $2C
    stx $2D
    lda $28
    ldx $29
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $26
    ldx $26+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2E
    stx $2E+1
    ldy #0
    lda ($2E),y
    ldx #0
    sta $30
    ; [peephole-opt]     lda $30
    ldx #0
    ldx #0
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    ; [peephole-opt]     ldx $33
    sta _main__local_4
    stx _main__local_4+1
    lda #4
    ldx #0
    sta $34
    stx $35
    lda $28
    ldx $29
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
    sta $36
    stx $36+1
    lda $36
    ldx $37
    add.16 .AX, #1
    sta $38
    stx $39
    ldy #0
    lda ($38),y
    ldx #0
    sta $3A
    ; [peephole-opt]     lda $3A
    ldx #0
    ldx #0
    sta $3C
    stx $3D
    ; [peephole-opt]     lda $3C
    ; [peephole-opt]     ldx $3D
    sta _main__local_6
    stx _main__local_6+1
    lda #4
    ldx #0
    sta $3E
    stx $3F
    lda $28
    ldx $29
    mul.16 .AX, #3
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
    sta $40
    stx $40+1
    lda $40
    ldx $41
    add.16 .AX, #2
    sta $42
    stx $43
    ldy #0
    lda ($42),y
    ldx #0
    sta $44
    ; [peephole-opt]     lda $44
    ldx #0
    ldx #0
    sta $46
    stx $47
    ; [peephole-opt]     lda $46
    ; [peephole-opt]     ldx $47
    sta _main__local_8
    stx _main__local_8+1
@for_inc112:
    lda $28
    ldx $29
    sta $48
    stx $49
    ; [peephole-opt]     lda $48
    clc
    adc #1
    sta $28
    lda $49
    adc #0
    sta $29
    bra @for_cond110
@for_cond114_ph:
    lda #5
    ldx #0
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
    ; [peephole-opt]     lda $20
    ; [peephole-opt]     ldx $21
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $26
    stx $26+1
    ; [peephole-opt]     lda $22
    ; [peephole-opt]     ldx $23
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
    lda $24
    ldx $25
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
@for_cond114:
    lda $28
    ldx $29
    cmp.16 .AX, #8
    bcc @for_body115
    bra @for_cond118_ph
@for_body115:
    lda #5
    ldx #0
    sta $2C
    stx $2D
    lda $28
    ldx $29
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $26
    ldx $26+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2E
    stx $2E+1
    ldy #0
    lda ($2E),y
    ldx #0
    sta $30
    ; [peephole-opt]     lda $30
    ldx #0
    ldx #0
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    ; [peephole-opt]     ldx $33
    sta _main__local_4
    stx _main__local_4+1
    lda #5
    ldx #0
    sta $34
    stx $35
    lda $28
    ldx $29
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
    sta $36
    stx $36+1
    lda $36
    ldx $37
    add.16 .AX, #1
    sta $38
    stx $39
    ldy #0
    lda ($38),y
    ldx #0
    sta $3A
    ; [peephole-opt]     lda $3A
    ldx #0
    ldx #0
    sta $3C
    stx $3D
    ; [peephole-opt]     lda $3C
    ; [peephole-opt]     ldx $3D
    sta _main__local_6
    stx _main__local_6+1
    lda #5
    ldx #0
    sta $3E
    stx $3F
    lda $28
    ldx $29
    mul.16 .AX, #3
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
    sta $40
    stx $40+1
    lda $40
    ldx $41
    add.16 .AX, #2
    sta $42
    stx $43
    ldy #0
    lda ($42),y
    ldx #0
    sta $44
    ; [peephole-opt]     lda $44
    ldx #0
    ldx #0
    sta $46
    stx $47
    ; [peephole-opt]     lda $46
    ; [peephole-opt]     ldx $47
    sta _main__local_8
    stx _main__local_8+1
@for_inc116:
    lda $28
    ldx $29
    sta $48
    stx $49
    ; [peephole-opt]     lda $48
    clc
    adc #1
    sta $28
    lda $49
    adc #0
    sta $29
    bra @for_cond114
@for_cond118_ph:
    lda #6
    ldx #0
    sta $20
    stx $21
    lda #6
    ldx #0
    sta $22
    stx $23
    lda #6
    ldx #0
    sta $24
    stx $25
    ; [peephole-opt]     lda $20
    ; [peephole-opt]     ldx $21
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $26
    stx $26+1
    ; [peephole-opt]     lda $22
    ; [peephole-opt]     ldx $23
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
    lda $24
    ldx $25
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
@for_cond118:
    lda $28
    ldx $29
    cmp.16 .AX, #8
    bcc @for_body119
    bra @for_cond122_ph
@for_body119:
    lda #6
    ldx #0
    sta $2C
    stx $2D
    lda $28
    ldx $29
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $26
    ldx $26+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2E
    stx $2E+1
    ldy #0
    lda ($2E),y
    ldx #0
    sta $30
    ; [peephole-opt]     lda $30
    ldx #0
    ldx #0
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    ; [peephole-opt]     ldx $33
    sta _main__local_4
    stx _main__local_4+1
    lda #6
    ldx #0
    sta $34
    stx $35
    lda $28
    ldx $29
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
    sta $36
    stx $36+1
    lda $36
    ldx $37
    add.16 .AX, #1
    sta $38
    stx $39
    ldy #0
    lda ($38),y
    ldx #0
    sta $3A
    ; [peephole-opt]     lda $3A
    ldx #0
    ldx #0
    sta $3C
    stx $3D
    ; [peephole-opt]     lda $3C
    ; [peephole-opt]     ldx $3D
    sta _main__local_6
    stx _main__local_6+1
    lda #6
    ldx #0
    sta $3E
    stx $3F
    lda $28
    ldx $29
    mul.16 .AX, #3
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
    sta $40
    stx $40+1
    lda $40
    ldx $41
    add.16 .AX, #2
    sta $42
    stx $43
    ldy #0
    lda ($42),y
    ldx #0
    sta $44
    ; [peephole-opt]     lda $44
    ldx #0
    ldx #0
    sta $46
    stx $47
    ; [peephole-opt]     lda $46
    ; [peephole-opt]     ldx $47
    sta _main__local_8
    stx _main__local_8+1
@for_inc120:
    lda $28
    ldx $29
    sta $48
    stx $49
    ; [peephole-opt]     lda $48
    clc
    adc #1
    sta $28
    lda $49
    adc #0
    sta $29
    bra @for_cond118
@for_cond122_ph:
    lda #7
    ldx #0
    sta $20
    stx $21
    lda #7
    ldx #0
    sta $22
    stx $23
    lda #7
    ldx #0
    sta $24
    stx $25
    ; [peephole-opt]     lda $20
    ; [peephole-opt]     ldx $21
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $26
    stx $26+1
    ; [peephole-opt]     lda $22
    ; [peephole-opt]     ldx $23
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
    lda $24
    ldx $25
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
@for_cond122:
    lda $28
    ldx $29
    cmp.16 .AX, #8
    bcc @for_body123
    bra @for_end125
@for_body123:
    lda #7
    ldx #0
    sta $2C
    stx $2D
    lda $28
    ldx $29
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $26
    ldx $26+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2E
    stx $2E+1
    ldy #0
    lda ($2E),y
    ldx #0
    sta $30
    ; [peephole-opt]     lda $30
    ldx #0
    ldx #0
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    ; [peephole-opt]     ldx $33
    sta _main__local_4
    stx _main__local_4+1
    lda #7
    ldx #0
    sta $34
    stx $35
    lda $28
    ldx $29
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
    sta $36
    stx $36+1
    lda $36
    ldx $37
    add.16 .AX, #1
    sta $38
    stx $39
    ldy #0
    lda ($38),y
    ldx #0
    sta $3A
    ; [peephole-opt]     lda $3A
    ldx #0
    ldx #0
    sta $3C
    stx $3D
    ; [peephole-opt]     lda $3C
    ; [peephole-opt]     ldx $3D
    sta _main__local_6
    stx _main__local_6+1
    lda #7
    ldx #0
    sta $3E
    stx $3F
    lda $28
    ldx $29
    mul.16 .AX, #3
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
    sta $40
    stx $40+1
    lda $40
    ldx $41
    add.16 .AX, #2
    sta $42
    stx $43
    ldy #0
    lda ($42),y
    ldx #0
    sta $44
    ; [peephole-opt]     lda $44
    ldx #0
    ldx #0
    sta $46
    stx $47
    ; [peephole-opt]     lda $46
    ; [peephole-opt]     ldx $47
    sta _main__local_8
    stx _main__local_8+1
@for_inc124:
    lda $28
    ldx $29
    sta $48
    stx $49
    ; [peephole-opt]     lda $48
    clc
    adc #1
    sta $28
    lda $49
    adc #0
    sta $29
    bra @for_cond122
@for_end125:
    .loc "src/test-resources/test_phase95_phase94_compat.c", 63
    ldax #__str_126
    sta _main__local_194
    stx _main__local_194+1
    lda _main__local_8
    ldx _main__local_8+1
    sta $28
    stx $29
    lda _main__local_6
    ldx _main__local_6+1
    sta $2A
    stx $2B
    lda _main__local_4
    ldx _main__local_4+1
    sta $2C
    stx $2D
    lda _main__local_194
    ; [peephole-opt]     ldx _main__local_194+1
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
@inline_end92:
    .loc "src/test-resources/test_phase95_phase94_compat.c", 68
    ldax #__str_128
    sta _main__local_196
    stx _main__local_196+1
    lda _main__local_196
    ; [peephole-opt]     ldx _main__local_196+1
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
    .loc "src/test-resources/test_phase95_phase94_compat.c", 69
    lda #0
    sta _main__local_198
    sta _main__local_198+1
    lda #0
    sta _main__local_200
    sta _main__local_200+1
    lda #0
    sta _main__local_202
    sta _main__local_202+1
@for_cond129_ph:
    lda #0
    sta $20
    sta $21
    lda #0
    sta $22
    sta $23
    lda #0
    sta $24
    sta $25
    ; [peephole-opt]     lda $20
    ldx $21
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $26
    stx $26+1
    ; [peephole-opt]     lda $22
    ldx $23
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
    lda $24
    ldx $25
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
@for_cond129:
    lda $28
    ldx $29
    cmp.16 .AX, #8
    bcc @for_body130
    bra @for_cond133_ph
@for_body130:
    lda #0
    sta $2C
    sta $2D
    lda $28
    ldx $29
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $26
    ldx $26+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2E
    stx $2E+1
    ldy #0
    lda ($2E),y
    ldx #0
    sta $30
    ; [peephole-opt]     lda $30
    ldx #0
    ldx #0
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    ; [peephole-opt]     ldx $33
    sta _main__local_198
    stx _main__local_198+1
    lda #0
    sta $34
    sta $35
    lda $28
    ldx $29
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
    sta $36
    stx $36+1
    lda $36
    ldx $37
    add.16 .AX, #1
    sta $38
    stx $39
    ldy #0
    lda ($38),y
    ldx #0
    sta $3A
    ; [peephole-opt]     lda $3A
    ldx #0
    ldx #0
    sta $3C
    stx $3D
    ; [peephole-opt]     lda $3C
    ; [peephole-opt]     ldx $3D
    sta _main__local_200
    stx _main__local_200+1
    lda #0
    sta $3E
    sta $3F
    lda $28
    ldx $29
    mul.16 .AX, #3
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
    sta $40
    stx $40+1
    lda $40
    ldx $41
    add.16 .AX, #2
    sta $42
    stx $43
    ldy #0
    lda ($42),y
    ldx #0
    sta $44
    ; [peephole-opt]     lda $44
    ldx #0
    ldx #0
    sta $46
    stx $47
    ; [peephole-opt]     lda $46
    ; [peephole-opt]     ldx $47
    sta _main__local_202
    stx _main__local_202+1
@for_inc131:
    lda $28
    ldx $29
    sta $48
    stx $49
    ; [peephole-opt]     lda $48
    clc
    adc #1
    sta $28
    lda $49
    adc #0
    sta $29
    bra @for_cond129
@for_cond133_ph:
    lda #1
    ldx #0
    sta $20
    stx $21
    lda #1
    ldx #0
    sta $22
    stx $23
    lda #1
    ldx #0
    sta $24
    stx $25
    ; [peephole-opt]     lda $20
    ; [peephole-opt]     ldx $21
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $26
    stx $26+1
    ; [peephole-opt]     lda $22
    ; [peephole-opt]     ldx $23
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
    lda $24
    ldx $25
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
@for_cond133:
    lda $28
    ldx $29
    cmp.16 .AX, #8
    bcc @for_body134
    bra @for_cond137_ph
@for_body134:
    lda #1
    ldx #0
    sta $2C
    stx $2D
    lda $28
    ldx $29
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $26
    ldx $26+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2E
    stx $2E+1
    ldy #0
    lda ($2E),y
    ldx #0
    sta $30
    ; [peephole-opt]     lda $30
    ldx #0
    ldx #0
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    ; [peephole-opt]     ldx $33
    sta _main__local_198
    stx _main__local_198+1
    lda #1
    ldx #0
    sta $34
    stx $35
    lda $28
    ldx $29
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
    sta $36
    stx $36+1
    lda $36
    ldx $37
    add.16 .AX, #1
    sta $38
    stx $39
    ldy #0
    lda ($38),y
    ldx #0
    sta $3A
    ; [peephole-opt]     lda $3A
    ldx #0
    ldx #0
    sta $3C
    stx $3D
    ; [peephole-opt]     lda $3C
    ; [peephole-opt]     ldx $3D
    sta _main__local_200
    stx _main__local_200+1
    lda #1
    ldx #0
    sta $3E
    stx $3F
    lda $28
    ldx $29
    mul.16 .AX, #3
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
    sta $40
    stx $40+1
    lda $40
    ldx $41
    add.16 .AX, #2
    sta $42
    stx $43
    ldy #0
    lda ($42),y
    ldx #0
    sta $44
    ; [peephole-opt]     lda $44
    ldx #0
    ldx #0
    sta $46
    stx $47
    ; [peephole-opt]     lda $46
    ; [peephole-opt]     ldx $47
    sta _main__local_202
    stx _main__local_202+1
@for_inc135:
    lda $28
    ldx $29
    sta $48
    stx $49
    ; [peephole-opt]     lda $48
    clc
    adc #1
    sta $28
    lda $49
    adc #0
    sta $29
    bra @for_cond133
@for_cond137_ph:
    lda #2
    ldx #0
    sta $20
    stx $21
    lda #2
    ldx #0
    sta $22
    stx $23
    lda #2
    ldx #0
    sta $24
    stx $25
    ; [peephole-opt]     lda $20
    ; [peephole-opt]     ldx $21
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $26
    stx $26+1
    ; [peephole-opt]     lda $22
    ; [peephole-opt]     ldx $23
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
    lda $24
    ldx $25
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
@for_cond137:
    lda $28
    ldx $29
    cmp.16 .AX, #8
    bcc @for_body138
    bra @for_cond141_ph
@for_body138:
    lda #2
    ldx #0
    sta $2C
    stx $2D
    lda $28
    ldx $29
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $26
    ldx $26+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2E
    stx $2E+1
    ldy #0
    lda ($2E),y
    ldx #0
    sta $30
    ; [peephole-opt]     lda $30
    ldx #0
    ldx #0
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    ; [peephole-opt]     ldx $33
    sta _main__local_198
    stx _main__local_198+1
    lda #2
    ldx #0
    sta $34
    stx $35
    lda $28
    ldx $29
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
    sta $36
    stx $36+1
    lda $36
    ldx $37
    add.16 .AX, #1
    sta $38
    stx $39
    ldy #0
    lda ($38),y
    ldx #0
    sta $3A
    ; [peephole-opt]     lda $3A
    ldx #0
    ldx #0
    sta $3C
    stx $3D
    ; [peephole-opt]     lda $3C
    ; [peephole-opt]     ldx $3D
    sta _main__local_200
    stx _main__local_200+1
    lda #2
    ldx #0
    sta $3E
    stx $3F
    lda $28
    ldx $29
    mul.16 .AX, #3
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
    sta $40
    stx $40+1
    lda $40
    ldx $41
    add.16 .AX, #2
    sta $42
    stx $43
    ldy #0
    lda ($42),y
    ldx #0
    sta $44
    ; [peephole-opt]     lda $44
    ldx #0
    ldx #0
    sta $46
    stx $47
    ; [peephole-opt]     lda $46
    ; [peephole-opt]     ldx $47
    sta _main__local_202
    stx _main__local_202+1
@for_inc139:
    lda $28
    ldx $29
    sta $48
    stx $49
    ; [peephole-opt]     lda $48
    clc
    adc #1
    sta $28
    lda $49
    adc #0
    sta $29
    bra @for_cond137
@for_cond141_ph:
    lda #3
    ldx #0
    sta $20
    stx $21
    lda #3
    ldx #0
    sta $22
    stx $23
    lda #3
    ldx #0
    sta $24
    stx $25
    ; [peephole-opt]     lda $20
    ; [peephole-opt]     ldx $21
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $26
    stx $26+1
    ; [peephole-opt]     lda $22
    ; [peephole-opt]     ldx $23
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
    lda $24
    ldx $25
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
@for_cond141:
    lda $28
    ldx $29
    cmp.16 .AX, #8
    bcc @for_body142
    bra @for_cond145_ph
@for_body142:
    lda #3
    ldx #0
    sta $2C
    stx $2D
    lda $28
    ldx $29
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $26
    ldx $26+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2E
    stx $2E+1
    ldy #0
    lda ($2E),y
    ldx #0
    sta $30
    ; [peephole-opt]     lda $30
    ldx #0
    ldx #0
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    ; [peephole-opt]     ldx $33
    sta _main__local_198
    stx _main__local_198+1
    lda #3
    ldx #0
    sta $34
    stx $35
    lda $28
    ldx $29
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
    sta $36
    stx $36+1
    lda $36
    ldx $37
    add.16 .AX, #1
    sta $38
    stx $39
    ldy #0
    lda ($38),y
    ldx #0
    sta $3A
    ; [peephole-opt]     lda $3A
    ldx #0
    ldx #0
    sta $3C
    stx $3D
    ; [peephole-opt]     lda $3C
    ; [peephole-opt]     ldx $3D
    sta _main__local_200
    stx _main__local_200+1
    lda #3
    ldx #0
    sta $3E
    stx $3F
    lda $28
    ldx $29
    mul.16 .AX, #3
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
    sta $40
    stx $40+1
    lda $40
    ldx $41
    add.16 .AX, #2
    sta $42
    stx $43
    ldy #0
    lda ($42),y
    ldx #0
    sta $44
    ; [peephole-opt]     lda $44
    ldx #0
    ldx #0
    sta $46
    stx $47
    ; [peephole-opt]     lda $46
    ; [peephole-opt]     ldx $47
    sta _main__local_202
    stx _main__local_202+1
@for_inc143:
    lda $28
    ldx $29
    sta $48
    stx $49
    ; [peephole-opt]     lda $48
    clc
    adc #1
    sta $28
    lda $49
    adc #0
    sta $29
    bra @for_cond141
@for_cond145_ph:
    lda #4
    ldx #0
    sta $20
    stx $21
    lda #4
    ldx #0
    sta $22
    stx $23
    lda #4
    ldx #0
    sta $24
    stx $25
    ; [peephole-opt]     lda $20
    ; [peephole-opt]     ldx $21
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $26
    stx $26+1
    ; [peephole-opt]     lda $22
    ; [peephole-opt]     ldx $23
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
    lda $24
    ldx $25
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
@for_cond145:
    lda $28
    ldx $29
    cmp.16 .AX, #8
    bcc @for_body146
    bra @for_cond149_ph
@for_body146:
    lda #4
    ldx #0
    sta $2C
    stx $2D
    lda $28
    ldx $29
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $26
    ldx $26+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2E
    stx $2E+1
    ldy #0
    lda ($2E),y
    ldx #0
    sta $30
    ; [peephole-opt]     lda $30
    ldx #0
    ldx #0
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    ; [peephole-opt]     ldx $33
    sta _main__local_198
    stx _main__local_198+1
    lda #4
    ldx #0
    sta $34
    stx $35
    lda $28
    ldx $29
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
    sta $36
    stx $36+1
    lda $36
    ldx $37
    add.16 .AX, #1
    sta $38
    stx $39
    ldy #0
    lda ($38),y
    ldx #0
    sta $3A
    ; [peephole-opt]     lda $3A
    ldx #0
    ldx #0
    sta $3C
    stx $3D
    ; [peephole-opt]     lda $3C
    ; [peephole-opt]     ldx $3D
    sta _main__local_200
    stx _main__local_200+1
    lda #4
    ldx #0
    sta $3E
    stx $3F
    lda $28
    ldx $29
    mul.16 .AX, #3
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
    sta $40
    stx $40+1
    lda $40
    ldx $41
    add.16 .AX, #2
    sta $42
    stx $43
    ldy #0
    lda ($42),y
    ldx #0
    sta $44
    ; [peephole-opt]     lda $44
    ldx #0
    ldx #0
    sta $46
    stx $47
    ; [peephole-opt]     lda $46
    ; [peephole-opt]     ldx $47
    sta _main__local_202
    stx _main__local_202+1
@for_inc147:
    lda $28
    ldx $29
    sta $48
    stx $49
    ; [peephole-opt]     lda $48
    clc
    adc #1
    sta $28
    lda $49
    adc #0
    sta $29
    bra @for_cond145
@for_cond149_ph:
    lda #5
    ldx #0
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
    ; [peephole-opt]     lda $20
    ; [peephole-opt]     ldx $21
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $26
    stx $26+1
    ; [peephole-opt]     lda $22
    ; [peephole-opt]     ldx $23
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
    lda $24
    ldx $25
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
@for_cond149:
    lda $28
    ldx $29
    cmp.16 .AX, #8
    bcc @for_body150
    bra @for_cond153_ph
@for_body150:
    lda #5
    ldx #0
    sta $2C
    stx $2D
    lda $28
    ldx $29
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $26
    ldx $26+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2E
    stx $2E+1
    ldy #0
    lda ($2E),y
    ldx #0
    sta $30
    ; [peephole-opt]     lda $30
    ldx #0
    ldx #0
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    ; [peephole-opt]     ldx $33
    sta _main__local_198
    stx _main__local_198+1
    lda #5
    ldx #0
    sta $34
    stx $35
    lda $28
    ldx $29
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
    sta $36
    stx $36+1
    lda $36
    ldx $37
    add.16 .AX, #1
    sta $38
    stx $39
    ldy #0
    lda ($38),y
    ldx #0
    sta $3A
    ; [peephole-opt]     lda $3A
    ldx #0
    ldx #0
    sta $3C
    stx $3D
    ; [peephole-opt]     lda $3C
    ; [peephole-opt]     ldx $3D
    sta _main__local_200
    stx _main__local_200+1
    lda #5
    ldx #0
    sta $3E
    stx $3F
    lda $28
    ldx $29
    mul.16 .AX, #3
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
    sta $40
    stx $40+1
    lda $40
    ldx $41
    add.16 .AX, #2
    sta $42
    stx $43
    ldy #0
    lda ($42),y
    ldx #0
    sta $44
    ; [peephole-opt]     lda $44
    ldx #0
    ldx #0
    sta $46
    stx $47
    ; [peephole-opt]     lda $46
    ; [peephole-opt]     ldx $47
    sta _main__local_202
    stx _main__local_202+1
@for_inc151:
    lda $28
    ldx $29
    sta $48
    stx $49
    ; [peephole-opt]     lda $48
    clc
    adc #1
    sta $28
    lda $49
    adc #0
    sta $29
    bra @for_cond149
@for_cond153_ph:
    lda #6
    ldx #0
    sta $20
    stx $21
    lda #6
    ldx #0
    sta $22
    stx $23
    lda #6
    ldx #0
    sta $24
    stx $25
    ; [peephole-opt]     lda $20
    ; [peephole-opt]     ldx $21
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $26
    stx $26+1
    ; [peephole-opt]     lda $22
    ; [peephole-opt]     ldx $23
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
    lda $24
    ldx $25
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
@for_cond153:
    lda $28
    ldx $29
    cmp.16 .AX, #8
    bcc @for_body154
    bra @for_cond157_ph
@for_body154:
    lda #6
    ldx #0
    sta $2C
    stx $2D
    lda $28
    ldx $29
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $26
    ldx $26+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2E
    stx $2E+1
    ldy #0
    lda ($2E),y
    ldx #0
    sta $30
    ; [peephole-opt]     lda $30
    ldx #0
    ldx #0
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    ; [peephole-opt]     ldx $33
    sta _main__local_198
    stx _main__local_198+1
    lda #6
    ldx #0
    sta $34
    stx $35
    lda $28
    ldx $29
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
    sta $36
    stx $36+1
    lda $36
    ldx $37
    add.16 .AX, #1
    sta $38
    stx $39
    ldy #0
    lda ($38),y
    ldx #0
    sta $3A
    ; [peephole-opt]     lda $3A
    ldx #0
    ldx #0
    sta $3C
    stx $3D
    ; [peephole-opt]     lda $3C
    ; [peephole-opt]     ldx $3D
    sta _main__local_200
    stx _main__local_200+1
    lda #6
    ldx #0
    sta $3E
    stx $3F
    lda $28
    ldx $29
    mul.16 .AX, #3
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
    sta $40
    stx $40+1
    lda $40
    ldx $41
    add.16 .AX, #2
    sta $42
    stx $43
    ldy #0
    lda ($42),y
    ldx #0
    sta $44
    ; [peephole-opt]     lda $44
    ldx #0
    ldx #0
    sta $46
    stx $47
    ; [peephole-opt]     lda $46
    ; [peephole-opt]     ldx $47
    sta _main__local_202
    stx _main__local_202+1
@for_inc155:
    lda $28
    ldx $29
    sta $48
    stx $49
    ; [peephole-opt]     lda $48
    clc
    adc #1
    sta $28
    lda $49
    adc #0
    sta $29
    bra @for_cond153
@for_cond157_ph:
    lda #7
    ldx #0
    sta $20
    stx $21
    lda #7
    ldx #0
    sta $22
    stx $23
    lda #7
    ldx #0
    sta $24
    stx $25
    ; [peephole-opt]     lda $20
    ; [peephole-opt]     ldx $21
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $26
    stx $26+1
    ; [peephole-opt]     lda $22
    ; [peephole-opt]     ldx $23
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
    lda $24
    ldx $25
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
@for_cond157:
    lda $28
    ldx $29
    cmp.16 .AX, #8
    bcc @for_body158
    bra @for_end160
@for_body158:
    lda #7
    ldx #0
    sta $2C
    stx $2D
    lda $28
    ldx $29
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $26
    ldx $26+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2E
    stx $2E+1
    ldy #0
    lda ($2E),y
    ldx #0
    sta $30
    ; [peephole-opt]     lda $30
    ldx #0
    ldx #0
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    ; [peephole-opt]     ldx $33
    sta _main__local_198
    stx _main__local_198+1
    lda #7
    ldx #0
    sta $34
    stx $35
    lda $28
    ldx $29
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
    sta $36
    stx $36+1
    lda $36
    ldx $37
    add.16 .AX, #1
    sta $38
    stx $39
    ldy #0
    lda ($38),y
    ldx #0
    sta $3A
    ; [peephole-opt]     lda $3A
    ldx #0
    ldx #0
    sta $3C
    stx $3D
    ; [peephole-opt]     lda $3C
    ; [peephole-opt]     ldx $3D
    sta _main__local_200
    stx _main__local_200+1
    lda #7
    ldx #0
    sta $3E
    stx $3F
    lda $28
    ldx $29
    mul.16 .AX, #3
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
    sta $40
    stx $40+1
    lda $40
    ldx $41
    add.16 .AX, #2
    sta $42
    stx $43
    ldy #0
    lda ($42),y
    ldx #0
    sta $44
    ; [peephole-opt]     lda $44
    ldx #0
    ldx #0
    sta $46
    stx $47
    ; [peephole-opt]     lda $46
    ; [peephole-opt]     ldx $47
    sta _main__local_202
    stx _main__local_202+1
@for_inc159:
    lda $28
    ldx $29
    sta $48
    stx $49
    ; [peephole-opt]     lda $48
    clc
    adc #1
    sta $28
    lda $49
    adc #0
    sta $29
    bra @for_cond157
@for_end160:
    .loc "src/test-resources/test_phase95_phase94_compat.c", 77
    ldax #__str_161
    sta _main__local_388
    stx _main__local_388+1
    lda _main__local_202
    ldx _main__local_202+1
    sta $28
    stx $29
    lda _main__local_200
    ldx _main__local_200+1
    sta $2A
    stx $2B
    lda _main__local_198
    ldx _main__local_198+1
    sta $2C
    stx $2D
    lda _main__local_388
    ; [peephole-opt]     ldx _main__local_388+1
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
@inline_end127:
    .loc "src/test-resources/test_phase95_phase94_compat.c", 82
    ldax #__str_163
    sta _main__local_390
    stx _main__local_390+1
    lda _main__local_390
    ; [peephole-opt]     ldx _main__local_390+1
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
    .loc "src/test-resources/test_phase95_phase94_compat.c", 83
    lda #1
    sta _main__local_392
    lda #0
    sta _main__local_392+1
    .loc "src/test-resources/test_phase95_phase94_compat.c", 84
    lda #0
    sta _main__local_394
    sta _main__local_394+1
@for_cond164:
    lda _main__local_394
    ldx _main__local_394+1
    cmp.16 .AX, #8
    bcc @for_body165
    bra @for_end167
@for_body165:
    .loc "src/test-resources/test_phase95_phase94_compat.c", 85
    lda #0
    sta _main__local_398
    sta _main__local_398+1
@for_cond168_ph:
    .loc "src/test-resources/test_phase95_phase94_compat.c", 86
    lda _main__local_394
    ldx _main__local_394+1
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $26
    stx $26+1
    lda _main__local_394
    ldx _main__local_394+1
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $28
    stx $28+1
    .loc "src/test-resources/test_phase95_phase94_compat.c", 87
    lda _main__local_394
    ldx _main__local_394+1
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2A
    stx $2A+1
    lda _main__local_394
    ldx _main__local_394+1
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2C
    stx $2C+1
    .loc "src/test-resources/test_phase95_phase94_compat.c", 88
    lda _main__local_394
    ldx _main__local_394+1
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_phase94_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2E
    stx $2E+1
    lda _main__local_394
    ldx _main__local_394+1
    mul.16 .AX, #24
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_regular_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $30
    stx $30+1
@for_cond168:
    .loc "src/test-resources/test_phase95_phase94_compat.c", 85
    lda _main__local_398
    ldx _main__local_398+1
    cmp.16 .AX, #8
    bcc @for_body169
    bra @for_end171
@for_body169:
    .loc "src/test-resources/test_phase95_phase94_compat.c", 86
    lda _main__local_398
    ldx _main__local_398+1
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $26
    ldx $26+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $36
    stx $36+1
    ldy #0
    lda ($36),y
    ldx #0
    sta $38
    lda _main__local_398
    ldx _main__local_398+1
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $28
    ldx $28+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $3A
    stx $3A+1
    ldy #0
    lda ($3A),y
    ldx #0
    sta $3C
    ; [peephole-opt]     lda $38
    ldx #0
    ldx #0
    sta $3E
    stx $3F
    ; [peephole-opt]     lda $3C
    ldx #0
    ldx #0
    sta $40
    stx $41
    ; [peephole-opt]     lda $3E
    ; [peephole-opt]     ldx $3F
    cmp.16 .AX, $40
    bne @if_then172
@or_rhs176:
    .loc "src/test-resources/test_phase95_phase94_compat.c", 87
    lda _main__local_398
    ldx _main__local_398+1
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $2A
    ldx $2A+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $44
    stx $44+1
    lda $44
    ldx $45
    add.16 .AX, #1
    sta $46
    stx $47
    ldy #0
    lda ($46),y
    ldx #0
    sta $48
    lda _main__local_398
    ldx _main__local_398+1
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $2C
    ldx $2C+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $4A
    stx $4A+1
    lda $4A
    ldx $4B
    add.16 .AX, #1
    sta $4C
    stx $4D
    ldy #0
    lda ($4C),y
    ldx #0
    sta $4E
    ; [peephole-opt]     lda $48
    ldx #0
    ldx #0
    sta $50
    stx $51
    ; [peephole-opt]     lda $4E
    ldx #0
    ldx #0
    sta $52
    stx $53
    ; [peephole-opt]     lda $50
    ; [peephole-opt]     ldx $51
    cmp.16 .AX, $52
    .loc "src/test-resources/test_phase95_phase94_compat.c", 86
    bne @if_then172
@or_rhs175:
    .loc "src/test-resources/test_phase95_phase94_compat.c", 88
    lda _main__local_398
    ldx _main__local_398+1
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $2E
    ldx $2E+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $56
    stx $56+1
    lda $56
    ldx $57
    add.16 .AX, #2
    sta $58
    stx $59
    ldy #0
    lda ($58),y
    ldx #0
    sta $5A
    lda _main__local_398
    ldx _main__local_398+1
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $30
    ldx $30+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $5C
    stx $5C+1
    lda $5C
    ldx $5D
    add.16 .AX, #2
    sta $5E
    stx $5F
    ldy #0
    lda ($5E),y
    ldx #0
    sta $60
    ; [peephole-opt]     lda $5A
    ldx #0
    ldx #0
    sta $62
    stx $63
    ; [peephole-opt]     lda $60
    ldx #0
    ldx #0
    sta $64
    stx $65
    ; [peephole-opt]     lda $62
    ; [peephole-opt]     ldx $63
    cmp.16 .AX, $64
    .loc "src/test-resources/test_phase95_phase94_compat.c", 86
    bne @if_then172
    bra @if_end174
@if_then172:
    .loc "src/test-resources/test_phase95_phase94_compat.c", 89
    lda #0
    sta _main__local_392
    sta _main__local_392+1
    bra @for_end171
@if_end174:
@for_inc170:
    .loc "src/test-resources/test_phase95_phase94_compat.c", 85
    lda _main__local_398
    ldx _main__local_398+1
    sta $6A
    stx $6B
    ; [peephole-opt]     lda $6A
    clc
    adc #1
    sta $6C
    lda $6B
    adc #0
    sta $6D
    ; [peephole-opt]     lda $6C
    ldx $6D
    sta _main__local_398
    stx _main__local_398+1
    bra @for_cond168
@for_end171:
    .loc "src/test-resources/test_phase95_phase94_compat.c", 93
    lda _main__local_392
    ldx _main__local_392+1
    bne @if_end179
    cmp #$00
    bne @if_end179
    bra @for_end167
@if_end179:
@for_inc166:
    .loc "src/test-resources/test_phase95_phase94_compat.c", 84
    lda _main__local_394
    ldx _main__local_394+1
    sta $6E
    stx $6F
    ; [peephole-opt]     lda $6E
    ; [peephole-opt]     ldx $6F
    add.16 .AX, #1
    sta _main__local_394
    stx _main__local_394+1
    bra @for_cond164
@for_end167:
    .loc "src/test-resources/test_phase95_phase94_compat.c", 95
    ldax #__str_180
    sta _main__local_440
    stx _main__local_440+1
    lda _main__local_392
    ldx _main__local_392+1
    bne @tern_then181
    cmp #$00
    bne @tern_then181
    bra @tern_else182
@tern_then181:
    ldax #__str_184
    bra @tern_end183
@tern_else182:
    ldax #__str_185
    sta _main__local_443
    stx _main__local_443+1
@tern_end183:
    lda _main__local_443
    ; [peephole-opt]     ldx _main__local_443+1
    sta $28
    stx $29
    lda _main__local_440
    ldx _main__local_440+1
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
    .frame_size 50
    endproc


    .segment "data"
__str_0:
    .text "Phase 94 array: "
    .byte 0
__str_33:
    .text "R=%d, G=%d, B=%d
"
    .byte 0
__str_34:
    .text "Regular array: "
    .byte 0
__str_67:
    .text "R=%d, G=%d, B=%d
"
    .byte 0
__str_68:
    .text "Data integrity check: "
    .byte 0
__str_85:
    .text "%s
"
    .byte 0
__str_89:
    .text "PASS"
    .byte 0
__str_90:
    .text "FAIL"
    .byte 0
__str_91:
    .text "Phase 95.6: Phase 94 backward compatibility test
"
    .byte 0
__str_93:
    .text "Phase 94 array: "
    .byte 0
__str_126:
    .text "R=%d, G=%d, B=%d
"
    .byte 0
__str_128:
    .text "Regular array: "
    .byte 0
__str_161:
    .text "R=%d, G=%d, B=%d
"
    .byte 0
__str_163:
    .text "Data integrity check: "
    .byte 0
__str_180:
    .text "%s
"
    .byte 0
__str_184:
    .text "PASS"
    .byte 0
__str_185:
    .text "FAIL"
    .byte 0
__str_186:
    .text "Compatibility test completed
"
    .byte 0

__zp_save_buf:
; [DEBUG] Phase 87 code reached, optimize=true

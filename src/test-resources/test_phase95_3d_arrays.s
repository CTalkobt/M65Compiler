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

    .global _layers
    .global _init_3d_array
    .global _test_layer0_red
    .global _test_layer1_green
    .global _test_both_layers_mixed
    .global _main

    .segment "bss"
_layers:
; .debug_var: @global _layers offset=0 size=2 type=int16 scope=global
    .res 96

    .segment "code"

; function _init_3d_array
; SAC inline storage: 12 bytes
    _init_3d_array__local_13: .word 0
    _init_3d_array__local_18: .word 0
    _init_3d_array__local_20: .word 0
    _init_3d_array__local_25: .word 0
    _init_3d_array__local_32: .word 0
    _init_3d_array__local_37: .word 0
    _init_3d_array__local_47: .word 0
    _init_3d_array__local_50: .word 0
    _init_3d_array__local_53: .word 0
    _init_3d_array__local_60: .word 0
    _init_3d_array__local_65: .word 0
    _init_3d_array__local_67: .word 0
    _init_3d_array__local_72: .word 0
    _init_3d_array__local_79: .word 0
    _init_3d_array__local_84: .word 0
    proc _init_3d_array
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdio.h", 13
    .local @_l_idx = 10
    .local @_l_x = 8
    .local @_l_y = 6
; .debug_var: __init_3d_array @_l_idx offset=10 size=2 type=int8 scope=local
; .debug_var: __init_3d_array @_l_x offset=8 size=2 type=int16 scope=local
; .debug_var: __init_3d_array @_l_y offset=6 size=2 type=int16 scope=local

@entry:
@for_cond0_ph:
    lda #17
    sta $20
    lda #255
    ldx #0
    sta $22
    stx $23
    lda #0
    sta $24
    sta $25
    lda #23
    sta $26
    lda #255
    ldx #0
    sta $28
    stx $29
    lda #0
    sta $2A
    sta $2B
    lda #31
    sta $2C
    lda #255
    ldx #0
    sta $2E
    stx $2F
    lda #0
    sta $30
    sta $31
    ldx #0
    sta $32
    stx $33
    ; [peephole-opt]     lda $20
    ldx #0
    ldx #0
    sta __zp_scratch2
    stx __zp_scratch2+1
    ; [peephole-opt]     lda $32
    ; [peephole-opt]     ldx $33
    mul.16 .AX, __zp_scratch2
    and $22
    sta $20
    stx $21
    lda $24
    ldx $25
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_layers
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
    lda $26
    ldx #0
    ldx #0
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $32
    ldx $33
    mul.16 .AX, __zp_scratch2
    and $28
    sta $24
    stx $25
    lda $2A
    ldx $2B
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_layers
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $26
    stx $26+1
    lda $2C
    ldx #0
    ldx #0
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $32
    ldx $33
    mul.16 .AX, __zp_scratch2
    and $2E
    sta $28
    stx $29
    lda $30
    ldx $31
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_layers
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2A
    stx $2A+1
@for_cond0:
    lda $2E
    ldx $2F
    cmp.16 .AX, #4
    bcc @for_cond4_ph
    bra @for_cond8_ph
@for_cond4_ph:
    lda #17
    sta $32
    lda #255
    ldx #0
    sta $34
    stx $35
    lda #0
    sta $36
    sta $37
    lda #23
    sta $38
    lda #255
    ldx #0
    sta $3A
    stx $3B
    lda #0
    sta $3C
    sta $3D
    lda #31
    sta $3E
    lda #255
    ldx #0
    sta $40
    stx $41
    lda #0
    sta $42
    sta $43
    lda $2E
    ldx $2F
    mul.16 .AX, #12
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
    sta $44
    stx $44+1
    lda $2E
    ldx $2F
    mul.16 .AX, #12
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
    sta $46
    stx $46+1
    lda $2E
    ldx $2F
    mul.16 .AX, #12
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
    sta $48
    stx $48+1
@for_cond4:
    lda $4C
    ldx $4D
    cmp.16 .AX, #4
    bcc @for_body5
    bra @for_end7
@for_body5:
    lda #17
    sta $50
    lda #255
    ldx #0
    sta $52
    stx $53
    lda #0
    sta $54
    sta $55
    lda $4C
    ldx $4D
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $44
    ldx $44+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $56
    stx $56+1
    lda $20
    ldy #0
    sta ($56),y
    lda #23
    sta $58
    lda #255
    ldx #0
    sta $5A
    stx $5B
    lda #0
    sta $5C
    sta $5D
    lda $4C
    ldx $4D
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $46
    ldx $46+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $5E
    stx $5E+1
    lda $5E
    ldx $5F
    add.16 .AX, #1
    sta $60
    stx $61
    lda $24
    ldy #0
    sta ($60),y
    lda #31
    sta $62
    lda #255
    ldx #0
    sta $64
    stx $65
    lda #0
    sta $66
    sta $67
    lda $4C
    ldx $4D
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $48
    ldx $48+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $68
    stx $68+1
    lda $68
    ldx $69
    add.16 .AX, #2
    sta $6A
    stx $6B
    lda $28
    ldy #0
    sta ($6A),y
@for_inc6:
    lda $4C
    ldx $4D
    sta $6C
    stx $6D
    ; [peephole-opt]     lda $6C
    clc
    adc #1
    sta $4C
    lda $6D
    adc #0
    sta $4D
    bra @for_cond4
@for_end7:
@for_inc2:
    lda $2E
    ldx $2F
    sta $70
    stx $71
    ; [peephole-opt]     lda $70
    clc
    adc #1
    sta $2E
    lda $71
    adc #0
    sta $2F
    bra @for_cond0
@for_cond8_ph:
    lda #17
    sta $20
    lda #255
    ldx #0
    sta $22
    stx $23
    lda #1
    ldx #0
    sta $24
    stx $25
    lda #23
    sta $26
    lda #255
    ldx #0
    sta $28
    stx $29
    lda #1
    ldx #0
    sta $2A
    stx $2B
    lda #31
    sta $2C
    lda #255
    ldx #0
    sta $2E
    stx $2F
    lda #1
    ldx #0
    sta $30
    stx $31
    lda _init_3d_array__local_53
    ldx #0
    sta $32
    stx $33
    lda $20
    ldx #0
    ldx #0
    sta __zp_scratch2
    stx __zp_scratch2+1
    ; [peephole-opt]     lda $32
    ; [peephole-opt]     ldx $33
    mul.16 .AX, __zp_scratch2
    and $22
    sta $20
    stx $21
    lda $24
    ldx $25
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_layers
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
    lda $26
    ldx #0
    ldx #0
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $32
    ldx $33
    mul.16 .AX, __zp_scratch2
    and $28
    sta $24
    stx $25
    lda $2A
    ldx $2B
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_layers
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $26
    stx $26+1
    lda $2C
    ldx #0
    ldx #0
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $32
    ldx $33
    mul.16 .AX, __zp_scratch2
    and $2E
    sta $28
    stx $29
    lda $30
    ldx $31
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_layers
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2A
    stx $2A+1
@for_cond8:
    lda _init_3d_array__local_47
    ldx _init_3d_array__local_47+1
    cmp.16 .AX, #4
    bcc @for_cond12_ph
    bra @for_end11
@for_cond12_ph:
    lda #17
    sta $30
    lda #255
    ldx #0
    sta $32
    stx $33
    lda #1
    ldx #0
    sta $34
    stx $35
    lda #23
    sta $36
    lda #255
    ldx #0
    sta $38
    stx $39
    lda #1
    ldx #0
    sta $3A
    stx $3B
    lda #31
    sta $3C
    lda #255
    ldx #0
    sta $3E
    stx $3F
    lda #1
    ldx #0
    sta $40
    stx $41
    lda _init_3d_array__local_47
    ldx _init_3d_array__local_47+1
    mul.16 .AX, #12
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
    sta $42
    stx $42+1
    lda _init_3d_array__local_47
    ldx _init_3d_array__local_47+1
    mul.16 .AX, #12
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
    sta $44
    stx $44+1
    lda _init_3d_array__local_47
    ldx _init_3d_array__local_47+1
    mul.16 .AX, #12
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
    sta $46
    stx $46+1
@for_cond12:
    lda _init_3d_array__local_50
    ldx _init_3d_array__local_50+1
    cmp.16 .AX, #4
    bcc @for_body13
    bra @for_end15
@for_body13:
    lda #17
    sta $4C
    lda #255
    ldx #0
    sta $4E
    stx $4F
    lda #1
    ldx #0
    sta $50
    stx $51
    lda _init_3d_array__local_50
    ldx _init_3d_array__local_50+1
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $42
    ldx $42+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $52
    stx $52+1
    lda $20
    ldy #0
    sta ($52),y
    lda #23
    sta $54
    lda #255
    ldx #0
    sta $56
    stx $57
    lda #1
    ldx #0
    sta $58
    stx $59
    lda _init_3d_array__local_50
    ldx _init_3d_array__local_50+1
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $44
    ldx $44+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $5A
    stx $5A+1
    lda $5A
    ldx $5B
    add.16 .AX, #1
    sta $5C
    stx $5D
    lda $24
    ldy #0
    sta ($5C),y
    lda #31
    sta $5E
    lda #255
    ldx #0
    sta $60
    stx $61
    lda #1
    ldx #0
    sta $62
    stx $63
    lda _init_3d_array__local_50
    ldx _init_3d_array__local_50+1
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $46
    ldx $46+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $64
    stx $64+1
    lda $64
    ldx $65
    add.16 .AX, #2
    sta $66
    stx $67
    lda $28
    ldy #0
    sta ($66),y
@for_inc14:
    lda _init_3d_array__local_50
    ldx _init_3d_array__local_50+1
    sta $68
    stx $69
    ; [peephole-opt]     lda $68
    clc
    adc #1
    sta $6A
    lda $69
    adc #0
    sta $6B
    ; [peephole-opt]     lda $6A
    ldx $6B
    sta _init_3d_array__local_50
    stx _init_3d_array__local_50+1
    bra @for_cond12
@for_end15:
@for_inc10:
    lda _init_3d_array__local_47
    ldx _init_3d_array__local_47+1
    sta $6C
    stx $6D
    ; [peephole-opt]     lda $6C
    ; [peephole-opt]     ldx $6D
    add.16 .AX, #1
    sta _init_3d_array__local_47
    stx _init_3d_array__local_47+1
    bra @for_cond8
@for_end11:
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 12
    endproc

; function _test_layer0_red
; SAC inline storage: 10 bytes
    _test_layer0_red__local_0: .word 0
    _test_layer0_red__local_2: .word 0
    _test_layer0_red__local_12: .word 0
    _test_layer0_red__local_25: .word 0
    _test_layer0_red__local_38: .word 0
    _test_layer0_red__local_43: .word 0
    _test_layer0_red__local_51: .word 0
    _test_layer0_red__local_56: .word 0
    proc _test_layer0_red
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "src/test-resources/test_phase95_3d_arrays.c", 10
    .local @_l_sum = 0
    .local @_l_x = 8
; .debug_var: __test_layer0_red @_l_sum offset=0 size=2 type=int16 scope=local
; .debug_var: __test_layer0_red @_l_x offset=8 size=2 type=int16 scope=local

@entry:
    .loc "src/test-resources/test_phase95_3d_arrays.c", 30
    ldax #__str_16
    sta _test_layer0_red__local_0
    stx _test_layer0_red__local_0+1
    lda _test_layer0_red__local_0
    ; [peephole-opt]     ldx _test_layer0_red__local_0+1
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
    .loc "src/test-resources/test_phase95_3d_arrays.c", 31
    lda #0
    sta _test_layer0_red__local_2
    sta _test_layer0_red__local_2+1
@for_cond17_ph:
    lda #0
    tax
    lda #0
    tax
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_layers
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    mul.16 .AX, #12
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
    sta $20
    stx $20+1
@for_cond17:
    lda $24
    ldx $25
    cmp.16 .AX, #4
    bcc @for_body18
    bra @for_cond21_ph
@for_body18:
    lda #0
    sta $28
    sta $29
    lda #0
    sta $2A
    sta $2B
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
    sta _test_layer0_red__local_2
    stx _test_layer0_red__local_2+1
@for_inc19:
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
    bra @for_cond17
@for_cond21_ph:
    lda #0
    tax
    lda #1
    ldx #0
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_layers
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    mul.16 .AX, #12
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
    sta $20
    stx $20+1
@for_cond21:
    lda $24
    ldx $25
    cmp.16 .AX, #4
    bcc @for_body22
    bra @for_cond25_ph
@for_body22:
    lda #0
    sta $28
    sta $29
    lda #1
    ldx #0
    sta $2A
    stx $2B
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
    sta _test_layer0_red__local_2
    stx _test_layer0_red__local_2+1
@for_inc23:
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
    bra @for_cond21
@for_cond25_ph:
    lda #0
    tax
    lda #2
    ldx #0
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_layers
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    mul.16 .AX, #12
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
    sta $20
    stx $20+1
@for_cond25:
    lda $24
    ldx $25
    cmp.16 .AX, #4
    bcc @for_body26
    bra @for_cond29_ph
@for_body26:
    lda #0
    sta $28
    sta $29
    lda #2
    ldx #0
    sta $2A
    stx $2B
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
    sta _test_layer0_red__local_2
    stx _test_layer0_red__local_2+1
@for_inc27:
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
    bra @for_cond25
@for_cond29_ph:
    lda #0
    tax
    lda #3
    ldx #0
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_layers
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    mul.16 .AX, #12
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
    sta $20
    stx $20+1
@for_cond29:
    lda _test_layer0_red__local_43
    ldx _test_layer0_red__local_43+1
    cmp.16 .AX, #4
    bcc @for_body30
    bra @for_end32
@for_body30:
    lda #0
    sta $26
    sta $27
    lda #3
    ldx #0
    sta $28
    stx $29
    lda _test_layer0_red__local_43
    ldx _test_layer0_red__local_43+1
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
    sta _test_layer0_red__local_2
    stx _test_layer0_red__local_2+1
@for_inc31:
    lda _test_layer0_red__local_43
    ldx _test_layer0_red__local_43+1
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    add.16 .AX, #1
    sta _test_layer0_red__local_43
    stx _test_layer0_red__local_43+1
    bra @for_cond29
@for_end32:
    .loc "src/test-resources/test_phase95_3d_arrays.c", 37
    ldax #__str_33
    sta _test_layer0_red__local_56
    stx _test_layer0_red__local_56+1
    lda _test_layer0_red__local_2
    ldx _test_layer0_red__local_2+1
    sta $28
    stx $29
    lda _test_layer0_red__local_56
    ; [peephole-opt]     ldx _test_layer0_red__local_56+1
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

; function _test_layer1_green
; SAC inline storage: 10 bytes
    _test_layer1_green__local_0: .word 0
    _test_layer1_green__local_2: .word 0
    _test_layer1_green__local_43: .word 0
    _test_layer1_green__local_56: .word 0
    proc _test_layer1_green
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "src/test-resources/test_phase95_3d_arrays.c", 21
    .local @_l_sum = 0
    .local @_l_x = 8
; .debug_var: __test_layer1_green @_l_sum offset=0 size=2 type=int16 scope=local
; .debug_var: __test_layer1_green @_l_x offset=8 size=2 type=int16 scope=local

@entry:
    .loc "src/test-resources/test_phase95_3d_arrays.c", 41
    ldax #__str_34
    sta _test_layer1_green__local_0
    stx _test_layer1_green__local_0+1
    lda _test_layer1_green__local_0
    ; [peephole-opt]     ldx _test_layer1_green__local_0+1
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
    .loc "src/test-resources/test_phase95_3d_arrays.c", 42
    lda #0
    sta _test_layer1_green__local_2
    sta _test_layer1_green__local_2+1
@for_cond35_ph:
    lda #1
    ldx #0
    lda #0
    tax
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_layers
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    mul.16 .AX, #12
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
    sta $20
    stx $20+1
@for_cond35:
    lda $24
    ldx $25
    cmp.16 .AX, #4
    bcc @for_body36
    bra @for_cond39_ph
@for_body36:
    lda #1
    ldx #0
    sta $28
    stx $29
    lda #0
    sta $2A
    sta $2B
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
    sta $2C
    stx $2C+1
    lda $2C
    ldx $2D
    add.16 .AX, #1
    sta $2E
    stx $2F
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
    sta _test_layer1_green__local_2
    stx _test_layer1_green__local_2+1
@for_inc37:
    lda $24
    ldx $25
    sta $34
    stx $35
    ; [peephole-opt]     lda $34
    clc
    adc #1
    sta $24
    lda $35
    adc #0
    sta $25
    bra @for_cond35
@for_cond39_ph:
    lda #1
    ldx #0
    lda #1
    ldx #0
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_layers
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    mul.16 .AX, #12
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
    sta $20
    stx $20+1
@for_cond39:
    lda $24
    ldx $25
    cmp.16 .AX, #4
    bcc @for_body40
    bra @for_cond43_ph
@for_body40:
    lda #1
    ldx #0
    sta $28
    stx $29
    lda #1
    ldx #0
    sta $2A
    stx $2B
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
    sta $2C
    stx $2C+1
    lda $2C
    ldx $2D
    add.16 .AX, #1
    sta $2E
    stx $2F
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
    sta _test_layer1_green__local_2
    stx _test_layer1_green__local_2+1
@for_inc41:
    lda $24
    ldx $25
    sta $34
    stx $35
    ; [peephole-opt]     lda $34
    clc
    adc #1
    sta $24
    lda $35
    adc #0
    sta $25
    bra @for_cond39
@for_cond43_ph:
    lda #1
    ldx #0
    lda #2
    ldx #0
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_layers
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    mul.16 .AX, #12
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
    sta $20
    stx $20+1
@for_cond43:
    lda $24
    ldx $25
    cmp.16 .AX, #4
    bcc @for_body44
    bra @for_cond47_ph
@for_body44:
    lda #1
    ldx #0
    sta $28
    stx $29
    lda #2
    ldx #0
    sta $2A
    stx $2B
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
    sta $2C
    stx $2C+1
    lda $2C
    ldx $2D
    add.16 .AX, #1
    sta $2E
    stx $2F
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
    sta _test_layer1_green__local_2
    stx _test_layer1_green__local_2+1
@for_inc45:
    lda $24
    ldx $25
    sta $34
    stx $35
    ; [peephole-opt]     lda $34
    clc
    adc #1
    sta $24
    lda $35
    adc #0
    sta $25
    bra @for_cond43
@for_cond47_ph:
    lda #1
    ldx #0
    lda #3
    ldx #0
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_layers
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    mul.16 .AX, #12
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
    sta $20
    stx $20+1
@for_cond47:
    lda _test_layer1_green__local_43
    ldx _test_layer1_green__local_43+1
    cmp.16 .AX, #4
    bcc @for_body48
    bra @for_end50
@for_body48:
    lda #1
    ldx #0
    sta $26
    stx $27
    lda #3
    ldx #0
    sta $28
    stx $29
    lda _test_layer1_green__local_43
    ldx _test_layer1_green__local_43+1
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
    sta _test_layer1_green__local_2
    stx _test_layer1_green__local_2+1
@for_inc49:
    lda _test_layer1_green__local_43
    ldx _test_layer1_green__local_43+1
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    ; [peephole-opt]     ldx $33
    add.16 .AX, #1
    sta _test_layer1_green__local_43
    stx _test_layer1_green__local_43+1
    bra @for_cond47
@for_end50:
    .loc "src/test-resources/test_phase95_3d_arrays.c", 48
    ldax #__str_51
    sta _test_layer1_green__local_56
    stx _test_layer1_green__local_56+1
    lda _test_layer1_green__local_2
    ldx _test_layer1_green__local_2+1
    sta $28
    stx $29
    lda _test_layer1_green__local_56
    ; [peephole-opt]     ldx _test_layer1_green__local_56+1
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

; function _test_both_layers_mixed
; SAC inline storage: 10 bytes
    _test_both_layers_mixed__local_0: .word 0
    _test_both_layers_mixed__local_2: .word 0
    _test_both_layers_mixed__local_29: .word 0
    _test_both_layers_mixed__local_32: .word 0
    _test_both_layers_mixed__local_54: .word 0
    proc _test_both_layers_mixed
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "src/test-resources/test_phase95_3d_arrays.c", 32
    .local @_l_sum = 0
    .local @_l_x = 8
    .local @_l_y = 6
; .debug_var: __test_both_layers_mixed @_l_sum offset=0 size=2 type=int16 scope=local
; .debug_var: __test_both_layers_mixed @_l_x offset=8 size=2 type=int16 scope=local
; .debug_var: __test_both_layers_mixed @_l_y offset=6 size=2 type=int16 scope=local

@entry:
    .loc "src/test-resources/test_phase95_3d_arrays.c", 52
    ldax #__str_52
    sta _test_both_layers_mixed__local_0
    stx _test_both_layers_mixed__local_0+1
    lda _test_both_layers_mixed__local_0
    ; [peephole-opt]     ldx _test_both_layers_mixed__local_0+1
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
    .loc "src/test-resources/test_phase95_3d_arrays.c", 53
    lda #0
    sta _test_both_layers_mixed__local_2
    sta _test_both_layers_mixed__local_2+1
@for_cond53_ph:
    lda #0
    tax
    lda #0
    tax
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_layers
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_layers
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
@for_cond53:
    lda $26
    ldx $27
    cmp.16 .AX, #4
    bcc @for_cond57_ph
    bra @for_cond61_ph
@for_cond57_ph:
    lda #0
    sta $2A
    sta $2B
    lda #0
    sta $2C
    sta $2D
    lda $26
    ldx $27
    mul.16 .AX, #12
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
    sta $2E
    stx $2E+1
    lda $26
    ldx $27
    mul.16 .AX, #12
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
    sta $30
    stx $30+1
@for_cond57:
    lda $34
    ldx $35
    cmp.16 .AX, #4
    bcc @for_body58
    bra @for_end60
@for_body58:
    lda #0
    sta $38
    sta $39
    lda $34
    ldx $35
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
    sta $3A
    stx $3A+1
    lda $3A
    ldx $3B
    sta $3C
    stx $3D
    ldy #0
    lda ($3A),y
    ldx #0
    sta $3E
    lda #0
    sta $40
    sta $41
    lda $34
    ldx $35
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
    sta $42
    stx $42+1
    lda $42
    ldx $43
    add.16 .AX, #1
    sta $44
    stx $45
    ldy #0
    lda ($44),y
    ldx #0
    sta $46
    ; [peephole-opt]     lda $3E
    ldx #0
    ldx #0
    sta $48
    stx $49
    ; [peephole-opt]     lda $46
    ldx #0
    ldx #0
    sta $4A
    stx $4B
    ; [peephole-opt]     lda $48
    clc
    adc $4A
    sta $4C
    lda $49
    adc $4A+1
    sta $4D
    ; [peephole-opt]     lda $4C
    ldx $4D
    sta _test_both_layers_mixed__local_2
    stx _test_both_layers_mixed__local_2+1
@for_inc59:
    lda $34
    ldx $35
    sta $4E
    stx $4F
    ; [peephole-opt]     lda $4E
    clc
    adc #1
    sta $34
    lda $4F
    adc #0
    sta $35
    bra @for_cond57
@for_end60:
@for_inc55:
    lda $26
    ldx $27
    sta $52
    stx $53
    ; [peephole-opt]     lda $52
    clc
    adc #1
    sta $26
    lda $53
    adc #0
    sta $27
    bra @for_cond53
@for_cond61_ph:
    lda #1
    ldx #0
    lda #1
    ldx #0
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_layers
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_layers
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
@for_cond61:
    lda _test_both_layers_mixed__local_29
    ldx _test_both_layers_mixed__local_29+1
    cmp.16 .AX, #4
    bcc @for_cond65_ph
    bra @for_end64
@for_cond65_ph:
    lda #1
    ldx #0
    sta $28
    stx $29
    lda #1
    ldx #0
    sta $2A
    stx $2B
    lda _test_both_layers_mixed__local_29
    ldx _test_both_layers_mixed__local_29+1
    mul.16 .AX, #12
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
    sta $2C
    stx $2C+1
    lda _test_both_layers_mixed__local_29
    ldx _test_both_layers_mixed__local_29+1
    mul.16 .AX, #12
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
    sta $2E
    stx $2E+1
@for_cond65:
    lda _test_both_layers_mixed__local_32
    ldx _test_both_layers_mixed__local_32+1
    cmp.16 .AX, #4
    bcc @for_body66
    bra @for_end68
@for_body66:
    lda #1
    ldx #0
    sta $34
    stx $35
    lda _test_both_layers_mixed__local_32
    ldx _test_both_layers_mixed__local_32+1
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
    sta $36
    stx $36+1
    lda $36
    ldx $37
    sta $38
    stx $39
    ldy #0
    lda ($36),y
    ldx #0
    sta $3A
    lda #1
    ldx #0
    sta $3C
    stx $3D
    lda _test_both_layers_mixed__local_32
    ldx _test_both_layers_mixed__local_32+1
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
    sta $3E
    stx $3E+1
    lda $3E
    ldx $3F
    add.16 .AX, #1
    sta $40
    stx $41
    ldy #0
    lda ($40),y
    ldx #0
    sta $42
    lda $3A
    ldx #0
    ldx #0
    sta $44
    stx $45
    ; [peephole-opt]     lda $42
    ldx #0
    ldx #0
    sta $46
    stx $47
    ; [peephole-opt]     lda $44
    clc
    adc $46
    sta $48
    lda $45
    adc $46+1
    sta $49
    ; [peephole-opt]     lda $48
    ldx $49
    sta _test_both_layers_mixed__local_2
    stx _test_both_layers_mixed__local_2+1
@for_inc67:
    lda _test_both_layers_mixed__local_32
    ldx _test_both_layers_mixed__local_32+1
    sta $4A
    stx $4B
    ; [peephole-opt]     lda $4A
    clc
    adc #1
    sta $4C
    lda $4B
    adc #0
    sta $4D
    ; [peephole-opt]     lda $4C
    ldx $4D
    sta _test_both_layers_mixed__local_32
    stx _test_both_layers_mixed__local_32+1
    bra @for_cond65
@for_end68:
@for_inc63:
    lda _test_both_layers_mixed__local_29
    ldx _test_both_layers_mixed__local_29+1
    sta $4E
    stx $4F
    ; [peephole-opt]     lda $4E
    ; [peephole-opt]     ldx $4F
    add.16 .AX, #1
    sta _test_both_layers_mixed__local_29
    stx _test_both_layers_mixed__local_29+1
    bra @for_cond61
@for_end64:
    .loc "src/test-resources/test_phase95_3d_arrays.c", 61
    ldax #__str_69
    sta _test_both_layers_mixed__local_54
    stx _test_both_layers_mixed__local_54+1
    lda _test_both_layers_mixed__local_2
    ldx _test_both_layers_mixed__local_2+1
    sta $28
    stx $29
    lda _test_both_layers_mixed__local_54
    ; [peephole-opt]     ldx _test_both_layers_mixed__local_54+1
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

; function _main
; SAC inline storage: 42 bytes
    _main__local_0: .word 0
    _main__local_55: .word 0
    _main__local_96: .word 0
    _main__local_98: .word 0
    _main__local_152: .word 0
    _main__local_154: .word 0
    _main__local_156: .word 0
    _main__local_210: .word 0
    _main__local_212: .word 0
    _main__local_214: .word 0
    _main__local_241: .word 0
    _main__local_244: .word 0
    _main__local_266: .word 0
    _main__local_268: .word 0
    _main__local_269: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "src/test-resources/test_phase95_3d_arrays.c", 45
    .local @_l_idx = 10
    .local @_l_sum = 32
    .local @_l_x = 40
    .local @_l_y = 38
; .debug_var: __main @_l_idx offset=10 size=2 type=int8 scope=local
; .debug_var: __main @_l_sum offset=32 size=2 type=int16 scope=local
; .debug_var: __main @_l_x offset=40 size=2 type=int16 scope=local
; .debug_var: __main @_l_y offset=38 size=2 type=int16 scope=local

@entry:
    .loc "src/test-resources/test_phase95_3d_arrays.c", 65
    ldax #__str_70
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
@for_cond72_ph:
    lda #17
    sta $20
    lda #255
    ldx #0
    sta $22
    stx $23
    lda #0
    sta $24
    sta $25
    lda #23
    sta $26
    lda #255
    ldx #0
    sta $28
    stx $29
    lda #0
    sta $2A
    sta $2B
    lda #31
    sta $2C
    lda #255
    ldx #0
    sta $2E
    stx $2F
    lda #0
    sta $30
    sta $31
    ldx #0
    sta $32
    stx $33
    ; [peephole-opt]     lda $20
    ldx #0
    ldx #0
    sta __zp_scratch2
    stx __zp_scratch2+1
    ; [peephole-opt]     lda $32
    ; [peephole-opt]     ldx $33
    mul.16 .AX, __zp_scratch2
    and $22
    sta $20
    stx $21
    ; [peephole-opt]     lda $20
    ; [peephole-opt]     ldx $21
    lda $24
    ldx $25
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_layers
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
    lda $32
    ldx $33
    lda $26
    ldx #0
    ldx #0
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $32
    ldx $33
    mul.16 .AX, __zp_scratch2
    and $28
    sta $24
    stx $25
    ; [peephole-opt]     lda $24
    ; [peephole-opt]     ldx $25
    lda $2A
    ldx $2B
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_layers
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $26
    stx $26+1
    lda $32
    ldx $33
    lda $2C
    ldx #0
    ldx #0
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $32
    ldx $33
    mul.16 .AX, __zp_scratch2
    and $2E
    sta $28
    stx $29
    ; [peephole-opt]     lda $28
    ; [peephole-opt]     ldx $29
    lda $30
    ldx $31
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_layers
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2A
    stx $2A+1
@for_cond72:
    lda $2E
    ldx $2F
    cmp.16 .AX, #4
    bcc @for_cond76_ph
    bra @for_cond80_ph
@for_cond76_ph:
    lda #17
    sta $32
    lda #255
    ldx #0
    sta $34
    stx $35
    lda #0
    sta $36
    sta $37
    lda #23
    sta $38
    lda #255
    ldx #0
    sta $3A
    stx $3B
    lda #0
    sta $3C
    sta $3D
    lda #31
    sta $3E
    lda #255
    ldx #0
    sta $40
    stx $41
    lda #0
    sta $42
    sta $43
    lda $2E
    ldx $2F
    mul.16 .AX, #12
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
    sta $44
    stx $44+1
    lda $2E
    ldx $2F
    mul.16 .AX, #12
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
    sta $46
    stx $46+1
    lda $2E
    ldx $2F
    mul.16 .AX, #12
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
    sta $48
    stx $48+1
@for_cond76:
    lda $4C
    ldx $4D
    cmp.16 .AX, #4
    bcc @for_body77
    bra @for_end79
@for_body77:
    lda #17
    sta $50
    lda #255
    ldx #0
    sta $52
    stx $53
    lda #0
    sta $54
    sta $55
    lda $4C
    ldx $4D
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $44
    ldx $44+1
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
    sta $58
    stx $59
    lda $20
    ldy #0
    sta ($56),y
    lda #23
    sta $5A
    lda #255
    ldx #0
    sta $5C
    stx $5D
    lda #0
    sta $5E
    sta $5F
    lda $4C
    ldx $4D
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $46
    ldx $46+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $60
    stx $60+1
    lda $60
    ldx $61
    add.16 .AX, #1
    sta $62
    stx $63
    lda $24
    ldy #0
    sta ($62),y
    lda #31
    sta $64
    lda #255
    ldx #0
    sta $66
    stx $67
    lda #0
    sta $68
    sta $69
    lda $4C
    ldx $4D
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $48
    ldx $48+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $6A
    stx $6A+1
    lda $6A
    ldx $6B
    add.16 .AX, #2
    sta $6C
    stx $6D
    lda $28
    ldy #0
    sta ($6C),y
@for_inc78:
    lda $4C
    ldx $4D
    sta $6E
    stx $6F
    ; [peephole-opt]     lda $6E
    clc
    adc #1
    sta $4C
    lda $6F
    adc #0
    sta $4D
    bra @for_cond76
@for_end79:
@for_inc74:
    lda $2E
    ldx $2F
    sta $72
    stx $73
    ; [peephole-opt]     lda $72
    clc
    adc #1
    sta $2E
    lda $73
    adc #0
    sta $2F
    bra @for_cond72
@for_cond80_ph:
    lda #17
    sta $20
    lda #255
    ldx #0
    sta $22
    stx $23
    lda #1
    ldx #0
    sta $24
    stx $25
    lda #23
    sta $26
    lda #255
    ldx #0
    sta $28
    stx $29
    lda #1
    ldx #0
    sta $2A
    stx $2B
    lda #31
    sta $2C
    lda #255
    ldx #0
    sta $2E
    stx $2F
    lda #1
    ldx #0
    sta $30
    stx $31
    lda _main__local_55
    ldx #0
    sta $32
    stx $33
    lda $20
    ldx #0
    ldx #0
    sta __zp_scratch2
    stx __zp_scratch2+1
    ; [peephole-opt]     lda $32
    ; [peephole-opt]     ldx $33
    mul.16 .AX, __zp_scratch2
    and $22
    sta $20
    stx $21
    ; [peephole-opt]     lda $20
    ; [peephole-opt]     ldx $21
    lda $24
    ldx $25
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_layers
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
    lda $32
    ldx $33
    lda $26
    ldx #0
    ldx #0
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $32
    ldx $33
    mul.16 .AX, __zp_scratch2
    and $28
    sta $24
    stx $25
    ; [peephole-opt]     lda $24
    ; [peephole-opt]     ldx $25
    lda $2A
    ldx $2B
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_layers
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $26
    stx $26+1
    lda $32
    ldx $33
    lda $2C
    ldx #0
    ldx #0
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $32
    ldx $33
    mul.16 .AX, __zp_scratch2
    and $2E
    sta $28
    stx $29
    ; [peephole-opt]     lda $28
    ; [peephole-opt]     ldx $29
    lda $30
    ldx $31
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_layers
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2A
    stx $2A+1
@for_cond80:
    lda $2E
    ldx $2F
    cmp.16 .AX, #4
    bcc @for_cond84_ph
    bra @for_end83
@for_cond84_ph:
    lda #17
    sta $32
    lda #255
    ldx #0
    sta $34
    stx $35
    lda #1
    ldx #0
    sta $36
    stx $37
    lda #23
    sta $38
    lda #255
    ldx #0
    sta $3A
    stx $3B
    lda #1
    ldx #0
    sta $3C
    stx $3D
    lda #31
    sta $3E
    lda #255
    ldx #0
    sta $40
    stx $41
    lda #1
    ldx #0
    sta $42
    stx $43
    lda $2E
    ldx $2F
    mul.16 .AX, #12
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
    sta $44
    stx $44+1
    lda $2E
    ldx $2F
    mul.16 .AX, #12
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
    sta $46
    stx $46+1
    lda $2E
    ldx $2F
    mul.16 .AX, #12
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
    sta $48
    stx $48+1
@for_cond84:
    lda $4C
    ldx $4D
    cmp.16 .AX, #4
    bcc @for_body85
    bra @for_end87
@for_body85:
    lda #17
    sta $50
    lda #255
    ldx #0
    sta $52
    stx $53
    lda #1
    ldx #0
    sta $54
    stx $55
    lda $4C
    ldx $4D
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $44
    ldx $44+1
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
    sta $58
    stx $59
    lda $20
    ldy #0
    sta ($56),y
    lda #23
    sta $5A
    lda #255
    ldx #0
    sta $5C
    stx $5D
    lda #1
    ldx #0
    sta $5E
    stx $5F
    lda $4C
    ldx $4D
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $46
    ldx $46+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $60
    stx $60+1
    lda $60
    ldx $61
    add.16 .AX, #1
    sta $62
    stx $63
    lda $24
    ldy #0
    sta ($62),y
    lda #31
    sta $64
    lda #255
    ldx #0
    sta $66
    stx $67
    lda #1
    ldx #0
    sta $68
    stx $69
    lda $4C
    ldx $4D
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $48
    ldx $48+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $6A
    stx $6A+1
    lda $6A
    ldx $6B
    add.16 .AX, #2
    sta $6C
    stx $6D
    lda $28
    ldy #0
    sta ($6C),y
@for_inc86:
    lda $4C
    ldx $4D
    sta $6E
    stx $6F
    ; [peephole-opt]     lda $6E
    clc
    adc #1
    sta $4C
    lda $6F
    adc #0
    sta $4D
    bra @for_cond84
@for_end87:
@for_inc82:
    lda $2E
    ldx $2F
    sta $72
    stx $73
    ; [peephole-opt]     lda $72
    clc
    adc #1
    sta $2E
    lda $73
    adc #0
    sta $2F
    bra @for_cond80
@for_end83:
@inline_end71:
    .loc "src/test-resources/test_phase95_3d_arrays.c", 30
    ldax #__str_89
    sta _main__local_96
    stx _main__local_96+1
    lda _main__local_96
    ; [peephole-opt]     ldx _main__local_96+1
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
    .loc "src/test-resources/test_phase95_3d_arrays.c", 31
    lda #0
    sta _main__local_98
    sta _main__local_98+1
@for_cond90_ph:
    lda #0
    tax
    lda #0
    tax
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_layers
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    mul.16 .AX, #12
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
    sta $20
    stx $20+1
@for_cond90:
    lda $24
    ldx $25
    cmp.16 .AX, #4
    bcc @for_body91
    bra @for_cond94_ph
@for_body91:
    lda #0
    sta $28
    sta $29
    lda #0
    sta $2A
    sta $2B
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
    sta $2C
    stx $2C+1
    lda $2C
    ldx $2D
    sta $2E
    stx $2F
    ldy #0
    lda ($2C),y
    ldx #0
    sta $30
    ; [peephole-opt]     lda $30
    ldx #0
    ldx #0
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    ; [peephole-opt]     ldx $33
    sta _main__local_98
    stx _main__local_98+1
@for_inc92:
    lda $24
    ldx $25
    sta $34
    stx $35
    ; [peephole-opt]     lda $34
    clc
    adc #1
    sta $24
    lda $35
    adc #0
    sta $25
    bra @for_cond90
@for_cond94_ph:
    lda #0
    tax
    lda #1
    ldx #0
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_layers
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    mul.16 .AX, #12
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
    sta $20
    stx $20+1
@for_cond94:
    lda $24
    ldx $25
    cmp.16 .AX, #4
    bcc @for_body95
    bra @for_cond98_ph
@for_body95:
    lda #0
    sta $28
    sta $29
    lda #1
    ldx #0
    sta $2A
    stx $2B
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
    sta $2C
    stx $2C+1
    lda $2C
    ldx $2D
    sta $2E
    stx $2F
    ldy #0
    lda ($2C),y
    ldx #0
    sta $30
    ; [peephole-opt]     lda $30
    ldx #0
    ldx #0
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    ; [peephole-opt]     ldx $33
    sta _main__local_98
    stx _main__local_98+1
@for_inc96:
    lda $24
    ldx $25
    sta $34
    stx $35
    ; [peephole-opt]     lda $34
    clc
    adc #1
    sta $24
    lda $35
    adc #0
    sta $25
    bra @for_cond94
@for_cond98_ph:
    lda #0
    tax
    lda #2
    ldx #0
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_layers
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    mul.16 .AX, #12
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
    sta $20
    stx $20+1
@for_cond98:
    lda $24
    ldx $25
    cmp.16 .AX, #4
    bcc @for_body99
    bra @for_cond102_ph
@for_body99:
    lda #0
    sta $28
    sta $29
    lda #2
    ldx #0
    sta $2A
    stx $2B
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
    sta $2C
    stx $2C+1
    lda $2C
    ldx $2D
    sta $2E
    stx $2F
    ldy #0
    lda ($2C),y
    ldx #0
    sta $30
    ; [peephole-opt]     lda $30
    ldx #0
    ldx #0
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    ; [peephole-opt]     ldx $33
    sta _main__local_98
    stx _main__local_98+1
@for_inc100:
    lda $24
    ldx $25
    sta $34
    stx $35
    ; [peephole-opt]     lda $34
    clc
    adc #1
    sta $24
    lda $35
    adc #0
    sta $25
    bra @for_cond98
@for_cond102_ph:
    lda #0
    tax
    lda #3
    ldx #0
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_layers
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    mul.16 .AX, #12
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
    sta $20
    stx $20+1
@for_cond102:
    lda $24
    ldx $25
    cmp.16 .AX, #4
    bcc @for_body103
    bra @for_end105
@for_body103:
    lda #0
    sta $28
    sta $29
    lda #3
    ldx #0
    sta $2A
    stx $2B
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
    sta $2C
    stx $2C+1
    lda $2C
    ldx $2D
    sta $2E
    stx $2F
    ldy #0
    lda ($2C),y
    ldx #0
    sta $30
    ; [peephole-opt]     lda $30
    ldx #0
    ldx #0
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    ; [peephole-opt]     ldx $33
    sta _main__local_98
    stx _main__local_98+1
@for_inc104:
    lda $24
    ldx $25
    sta $34
    stx $35
    ; [peephole-opt]     lda $34
    clc
    adc #1
    sta $24
    lda $35
    adc #0
    sta $25
    bra @for_cond102
@for_end105:
    .loc "src/test-resources/test_phase95_3d_arrays.c", 37
    ldax #__str_106
    sta _main__local_152
    stx _main__local_152+1
    lda _main__local_98
    ldx _main__local_98+1
    sta $28
    stx $29
    lda _main__local_152
    ; [peephole-opt]     ldx _main__local_152+1
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
@inline_end88:
    .loc "src/test-resources/test_phase95_3d_arrays.c", 41
    ldax #__str_108
    sta _main__local_154
    stx _main__local_154+1
    lda _main__local_154
    ; [peephole-opt]     ldx _main__local_154+1
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
    .loc "src/test-resources/test_phase95_3d_arrays.c", 42
    lda #0
    sta _main__local_156
    sta _main__local_156+1
@for_cond109_ph:
    lda #1
    ldx #0
    lda #0
    tax
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_layers
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    mul.16 .AX, #12
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
    sta $20
    stx $20+1
@for_cond109:
    lda $24
    ldx $25
    cmp.16 .AX, #4
    bcc @for_body110
    bra @for_cond113_ph
@for_body110:
    lda #1
    ldx #0
    sta $28
    stx $29
    lda #0
    sta $2A
    sta $2B
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
    sta $2C
    stx $2C+1
    lda $2C
    ldx $2D
    add.16 .AX, #1
    sta $2E
    stx $2F
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
    sta _main__local_156
    stx _main__local_156+1
@for_inc111:
    lda $24
    ldx $25
    sta $34
    stx $35
    ; [peephole-opt]     lda $34
    clc
    adc #1
    sta $24
    lda $35
    adc #0
    sta $25
    bra @for_cond109
@for_cond113_ph:
    lda #1
    ldx #0
    lda #1
    ldx #0
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_layers
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    mul.16 .AX, #12
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
    sta $20
    stx $20+1
@for_cond113:
    lda $24
    ldx $25
    cmp.16 .AX, #4
    bcc @for_body114
    bra @for_cond117_ph
@for_body114:
    lda #1
    ldx #0
    sta $28
    stx $29
    lda #1
    ldx #0
    sta $2A
    stx $2B
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
    sta $2C
    stx $2C+1
    lda $2C
    ldx $2D
    add.16 .AX, #1
    sta $2E
    stx $2F
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
    sta _main__local_156
    stx _main__local_156+1
@for_inc115:
    lda $24
    ldx $25
    sta $34
    stx $35
    ; [peephole-opt]     lda $34
    clc
    adc #1
    sta $24
    lda $35
    adc #0
    sta $25
    bra @for_cond113
@for_cond117_ph:
    lda #1
    ldx #0
    lda #2
    ldx #0
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_layers
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    mul.16 .AX, #12
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
    sta $20
    stx $20+1
@for_cond117:
    lda $24
    ldx $25
    cmp.16 .AX, #4
    bcc @for_body118
    bra @for_cond121_ph
@for_body118:
    lda #1
    ldx #0
    sta $28
    stx $29
    lda #2
    ldx #0
    sta $2A
    stx $2B
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
    sta $2C
    stx $2C+1
    lda $2C
    ldx $2D
    add.16 .AX, #1
    sta $2E
    stx $2F
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
    sta _main__local_156
    stx _main__local_156+1
@for_inc119:
    lda $24
    ldx $25
    sta $34
    stx $35
    ; [peephole-opt]     lda $34
    clc
    adc #1
    sta $24
    lda $35
    adc #0
    sta $25
    bra @for_cond117
@for_cond121_ph:
    lda #1
    ldx #0
    lda #3
    ldx #0
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_layers
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    mul.16 .AX, #12
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
    sta $20
    stx $20+1
@for_cond121:
    lda $24
    ldx $25
    cmp.16 .AX, #4
    bcc @for_body122
    bra @for_end124
@for_body122:
    lda #1
    ldx #0
    sta $28
    stx $29
    lda #3
    ldx #0
    sta $2A
    stx $2B
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
    sta $2C
    stx $2C+1
    lda $2C
    ldx $2D
    add.16 .AX, #1
    sta $2E
    stx $2F
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
    sta _main__local_156
    stx _main__local_156+1
@for_inc123:
    lda $24
    ldx $25
    sta $34
    stx $35
    ; [peephole-opt]     lda $34
    clc
    adc #1
    sta $24
    lda $35
    adc #0
    sta $25
    bra @for_cond121
@for_end124:
    .loc "src/test-resources/test_phase95_3d_arrays.c", 48
    ldax #__str_125
    sta _main__local_210
    stx _main__local_210+1
    lda _main__local_156
    ldx _main__local_156+1
    sta $28
    stx $29
    lda _main__local_210
    ; [peephole-opt]     ldx _main__local_210+1
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
@inline_end107:
    .loc "src/test-resources/test_phase95_3d_arrays.c", 52
    ldax #__str_127
    sta _main__local_212
    stx _main__local_212+1
    lda _main__local_212
    ; [peephole-opt]     ldx _main__local_212+1
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
    .loc "src/test-resources/test_phase95_3d_arrays.c", 53
    lda #0
    sta _main__local_214
    sta _main__local_214+1
@for_cond128_ph:
    lda #0
    tax
    lda #0
    tax
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_layers
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_layers
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
@for_cond128:
    lda $26
    ldx $27
    cmp.16 .AX, #4
    bcc @for_cond132_ph
    bra @for_cond136_ph
@for_cond132_ph:
    lda #0
    sta $2A
    sta $2B
    lda #0
    sta $2C
    sta $2D
    lda $26
    ldx $27
    mul.16 .AX, #12
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
    sta $2E
    stx $2E+1
    lda $26
    ldx $27
    mul.16 .AX, #12
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
    sta $30
    stx $30+1
@for_cond132:
    lda $34
    ldx $35
    cmp.16 .AX, #4
    bcc @for_body133
    bra @for_end135
@for_body133:
    lda #0
    sta $38
    sta $39
    lda $34
    ldx $35
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
    sta $3A
    stx $3A+1
    lda $3A
    ldx $3B
    sta $3C
    stx $3D
    ldy #0
    lda ($3A),y
    ldx #0
    sta $3E
    lda #0
    sta $40
    sta $41
    lda $34
    ldx $35
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
    sta $42
    stx $42+1
    lda $42
    ldx $43
    add.16 .AX, #1
    sta $44
    stx $45
    ldy #0
    lda ($44),y
    ldx #0
    sta $46
    ; [peephole-opt]     lda $3E
    ldx #0
    ldx #0
    sta $48
    stx $49
    ; [peephole-opt]     lda $46
    ldx #0
    ldx #0
    sta $4A
    stx $4B
    ; [peephole-opt]     lda $48
    clc
    adc $4A
    sta $4C
    lda $49
    adc $4A+1
    sta $4D
    ; [peephole-opt]     lda $4C
    ldx $4D
    sta _main__local_214
    stx _main__local_214+1
@for_inc134:
    lda $34
    ldx $35
    sta $4E
    stx $4F
    ; [peephole-opt]     lda $4E
    clc
    adc #1
    sta $34
    lda $4F
    adc #0
    sta $35
    bra @for_cond132
@for_end135:
@for_inc130:
    lda $26
    ldx $27
    sta $52
    stx $53
    ; [peephole-opt]     lda $52
    clc
    adc #1
    sta $26
    lda $53
    adc #0
    sta $27
    bra @for_cond128
@for_cond136_ph:
    lda #1
    ldx #0
    lda #1
    ldx #0
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_layers
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $20
    stx $20+1
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_layers
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
@for_cond136:
    lda _main__local_241
    ldx _main__local_241+1
    cmp.16 .AX, #4
    bcc @for_cond140_ph
    bra @for_end139
@for_cond140_ph:
    lda #1
    ldx #0
    sta $28
    stx $29
    lda #1
    ldx #0
    sta $2A
    stx $2B
    lda _main__local_241
    ldx _main__local_241+1
    mul.16 .AX, #12
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
    sta $2C
    stx $2C+1
    lda _main__local_241
    ldx _main__local_241+1
    mul.16 .AX, #12
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
    sta $2E
    stx $2E+1
@for_cond140:
    lda _main__local_244
    ldx _main__local_244+1
    cmp.16 .AX, #4
    bcc @for_body141
    bra @for_end143
@for_body141:
    lda #1
    ldx #0
    sta $34
    stx $35
    lda _main__local_244
    ldx _main__local_244+1
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
    sta $36
    stx $36+1
    lda $36
    ldx $37
    sta $38
    stx $39
    ldy #0
    lda ($36),y
    ldx #0
    sta $3A
    lda #1
    ldx #0
    sta $3C
    stx $3D
    lda _main__local_244
    ldx _main__local_244+1
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
    sta $3E
    stx $3E+1
    lda $3E
    ldx $3F
    add.16 .AX, #1
    sta $40
    stx $41
    ldy #0
    lda ($40),y
    ldx #0
    sta $42
    lda $3A
    ldx #0
    ldx #0
    sta $44
    stx $45
    ; [peephole-opt]     lda $42
    ldx #0
    ldx #0
    sta $46
    stx $47
    ; [peephole-opt]     lda $44
    clc
    adc $46
    sta $48
    lda $45
    adc $46+1
    sta $49
    ; [peephole-opt]     lda $48
    ldx $49
    sta _main__local_214
    stx _main__local_214+1
@for_inc142:
    lda _main__local_244
    ldx _main__local_244+1
    sta $4A
    stx $4B
    ; [peephole-opt]     lda $4A
    clc
    adc #1
    sta $4C
    lda $4B
    adc #0
    sta $4D
    ; [peephole-opt]     lda $4C
    ldx $4D
    sta _main__local_244
    stx _main__local_244+1
    bra @for_cond140
@for_end143:
@for_inc138:
    lda _main__local_241
    ldx _main__local_241+1
    sta $4E
    stx $4F
    ; [peephole-opt]     lda $4E
    ; [peephole-opt]     ldx $4F
    add.16 .AX, #1
    sta _main__local_241
    stx _main__local_241+1
    bra @for_cond136
@for_end139:
    .loc "src/test-resources/test_phase95_3d_arrays.c", 61
    ldax #__str_144
    sta _main__local_266
    stx _main__local_266+1
    lda _main__local_214
    ldx _main__local_214+1
    sta $28
    stx $29
    lda _main__local_266
    ; [peephole-opt]     ldx _main__local_266+1
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
    .frame_size 42
    endproc


    .segment "data"
__str_16:
    .text "Layer 0 red values: "
    .byte 0
__str_33:
    .text "sum=%d
"
    .byte 0
__str_34:
    .text "Layer 1 green values: "
    .byte 0
__str_51:
    .text "sum=%d
"
    .byte 0
__str_52:
    .text "Both layers mixed field access: "
    .byte 0
__str_69:
    .text "sum=%d
"
    .byte 0
__str_70:
    .text "Phase 95.6: 3D field-striped array test
"
    .byte 0
__str_89:
    .text "Layer 0 red values: "
    .byte 0
__str_106:
    .text "sum=%d
"
    .byte 0
__str_108:
    .text "Layer 1 green values: "
    .byte 0
__str_125:
    .text "sum=%d
"
    .byte 0
__str_127:
    .text "Both layers mixed field access: "
    .byte 0
__str_144:
    .text "sum=%d
"
    .byte 0
__str_145:
    .text "All 3D tests completed
"
    .byte 0

__zp_save_buf:
; [DEBUG] Phase 87 code reached, optimize=true

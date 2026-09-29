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

    .global _pixels
    .global _init_pixels
    .global _sum_red_field
    .global _sum_green_field
    .global _sum_all_fields
    .global _sum_r_and_g
    .global _main

    .segment "bss"
_pixels:
; .debug_var: @global _pixels offset=0 size=2 type=int16 scope=global
    .res 768

    .segment "code"

; function _init_pixels
; SAC inline storage: 6 bytes
    _init_pixels__local_0: .word 0
    _init_pixels__local_2: .word 0
    _init_pixels__local_6: .word 0
    _init_pixels__local_14: .word 0
    _init_pixels__local_17: .word 0
    _init_pixels__local_22: .word 0
    _init_pixels__local_23: .word 0
    _init_pixels__local_24: .word 0
    _init_pixels__local_30: .word 0
    _init_pixels__local_31: .word 0
    _init_pixels__local_32: .word 0
    proc _init_pixels
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdio.h", 12
    .local @_l_idx = 0
    .local @_l_x = 4
    .local @_l_y = 2
; .debug_var: __init_pixels @_l_idx offset=0 size=2 type=int16 scope=local
; .debug_var: __init_pixels @_l_x offset=4 size=2 type=int16 scope=local
; .debug_var: __init_pixels @_l_y offset=2 size=2 type=int16 scope=local

@entry:
    .loc "src/test-resources/test_phase95_dead_code.c", 16
    lda #0
    sta _init_pixels__local_0
    sta _init_pixels__local_0+1
    .loc "src/test-resources/test_phase95_dead_code.c", 17
    lda #0
    sta _init_pixels__local_2
    sta _init_pixels__local_2+1
@for_cond0:
    lda _init_pixels__local_2
    ldx _init_pixels__local_2+1
    cmp.16 .AX, #16
    bcc @for_body1
    bra @for_end3
@for_body1:
    .loc "src/test-resources/test_phase95_dead_code.c", 18
    lda #0
    sta _init_pixels__local_6
    sta _init_pixels__local_6+1
@for_cond4_ph:
    .loc "src/test-resources/test_phase95_dead_code.c", 19
    lda _init_pixels__local_2
    ldx _init_pixels__local_2+1
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $26
    stx $26+1
    .loc "src/test-resources/test_phase95_dead_code.c", 20
    .loc "src/test-resources/test_phase95_dead_code.c", 21
@for_cond4:
    .loc "src/test-resources/test_phase95_dead_code.c", 18
    lda _init_pixels__local_6
    ldx _init_pixels__local_6+1
    cmp.16 .AX, #16
    bcc @for_body5
    bra @for_end7
@for_body5:
    .loc "src/test-resources/test_phase95_dead_code.c", 19
    lda #17
    ldx #0
    sta $2C
    stx $2D
    lda _init_pixels__local_0
    ldx _init_pixels__local_0+1
    mul.16 .AX, $2C
    sta $2E
    stx $2F
    lda #255
    ldx #0
    sta $30
    stx $31
    ; [peephole-opt]     lda $2E
    ; [peephole-opt]     ldx $2F
    and $30
    sta $32
    stx $33
    lda _init_pixels__local_6
    ldx _init_pixels__local_6+1
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
    sta $34
    stx $34+1
    ; [peephole-opt]     lda $32
    ldy #0
    sta ($34),y
    .loc "src/test-resources/test_phase95_dead_code.c", 20
    lda #23
    ldx #0
    sta $36
    stx $37
    lda _init_pixels__local_0
    ldx _init_pixels__local_0+1
    mul.16 .AX, $36
    sta $38
    stx $39
    lda #255
    ldx #0
    sta $3A
    stx $3B
    ; [peephole-opt]     lda $38
    ; [peephole-opt]     ldx $39
    and $3A
    sta $3C
    stx $3D
    lda $34
    ldx $35
    add.16 .AX, #1
    sta $3E
    stx $3F
    ; [peephole-opt]     lda $3C
    ldy #0
    sta ($3E),y
    .loc "src/test-resources/test_phase95_dead_code.c", 21
    lda #31
    ldx #0
    sta $40
    stx $41
    lda _init_pixels__local_0
    ldx _init_pixels__local_0+1
    mul.16 .AX, $40
    sta $42
    stx $43
    lda #255
    ldx #0
    sta $44
    stx $45
    ; [peephole-opt]     lda $42
    ; [peephole-opt]     ldx $43
    and $44
    sta $46
    stx $47
    lda $34
    ldx $35
    add.16 .AX, #2
    sta $48
    stx $49
    ; [peephole-opt]     lda $46
    ldy #0
    sta ($48),y
    .loc "src/test-resources/test_phase95_dead_code.c", 22
    lda _init_pixels__local_0
    ldx _init_pixels__local_0+1
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
    sta _init_pixels__local_0
    stx _init_pixels__local_0+1
@for_inc6:
    .loc "src/test-resources/test_phase95_dead_code.c", 18
    lda _init_pixels__local_6
    ldx _init_pixels__local_6+1
    sta $4E
    stx $4F
    ; [peephole-opt]     lda $4E
    clc
    adc #1
    sta $50
    lda $4F
    adc #0
    sta $51
    ; [peephole-opt]     lda $50
    ldx $51
    sta _init_pixels__local_6
    stx _init_pixels__local_6+1
    bra @for_cond4
@for_end7:
@for_inc2:
    .loc "src/test-resources/test_phase95_dead_code.c", 17
    lda _init_pixels__local_2
    ldx _init_pixels__local_2+1
    sta $52
    stx $53
    ; [peephole-opt]     lda $52
    ; [peephole-opt]     ldx $53
    add.16 .AX, #1
    sta _init_pixels__local_2
    stx _init_pixels__local_2+1
    bra @for_cond0
@for_end3:
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _sum_red_field
; SAC inline storage: 6 bytes
    _sum_red_field__local_0: .word 0
    _sum_red_field__local_2: .word 0
    _sum_red_field__local_6: .word 0
    _sum_red_field__local_12: .word 0
    proc _sum_red_field
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "src/test-resources/test_phase95_dead_code.c", 10
    .local @_l_sum = 0
    .local @_l_x = 4
    .local @_l_y = 2
; .debug_var: __sum_red_field @_l_sum offset=0 size=2 type=int16 scope=local
; .debug_var: __sum_red_field @_l_x offset=4 size=2 type=int16 scope=local
; .debug_var: __sum_red_field @_l_y offset=2 size=2 type=int16 scope=local

@entry:
    .loc "src/test-resources/test_phase95_dead_code.c", 30
    lda #0
    sta _sum_red_field__local_0
    sta _sum_red_field__local_0+1
    .loc "src/test-resources/test_phase95_dead_code.c", 31
    lda #0
    sta _sum_red_field__local_2
    sta _sum_red_field__local_2+1
@for_cond8:
    lda _sum_red_field__local_2
    ldx _sum_red_field__local_2+1
    cmp.16 .AX, #16
    bcc @for_body9
    bra @for_end11
@for_body9:
    .loc "src/test-resources/test_phase95_dead_code.c", 32
    lda #0
    sta _sum_red_field__local_6
    sta _sum_red_field__local_6+1
@for_cond12_ph:
    .loc "src/test-resources/test_phase95_dead_code.c", 33
    lda _sum_red_field__local_2
    ldx _sum_red_field__local_2+1
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $26
    stx $26+1
@for_cond12:
    .loc "src/test-resources/test_phase95_dead_code.c", 32
    lda _sum_red_field__local_6
    ldx _sum_red_field__local_6+1
    cmp.16 .AX, #16
    bcc @for_body13
    bra @for_end15
@for_body13:
    .loc "src/test-resources/test_phase95_dead_code.c", 33
    lda _sum_red_field__local_6
    ldx _sum_red_field__local_6+1
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
    lda _sum_red_field__local_0
    ldx _sum_red_field__local_0+1
    add.16 .AX, $30
    sta $32
    stx $33
    sta _sum_red_field__local_0
    stx _sum_red_field__local_0+1
@for_inc14:
    .loc "src/test-resources/test_phase95_dead_code.c", 32
    lda _sum_red_field__local_6
    ldx _sum_red_field__local_6+1
    sta $34
    stx $35
    ; [peephole-opt]     lda $34
    clc
    adc #1
    sta $36
    lda $35
    adc #0
    sta $37
    ; [peephole-opt]     lda $36
    ldx $37
    sta _sum_red_field__local_6
    stx _sum_red_field__local_6+1
    bra @for_cond12
@for_end15:
@for_inc10:
    .loc "src/test-resources/test_phase95_dead_code.c", 31
    lda _sum_red_field__local_2
    ldx _sum_red_field__local_2+1
    sta $38
    stx $39
    ; [peephole-opt]     lda $38
    ; [peephole-opt]     ldx $39
    add.16 .AX, #1
    sta _sum_red_field__local_2
    stx _sum_red_field__local_2+1
    bra @for_cond8
@for_end11:
    .loc "src/test-resources/test_phase95_dead_code.c", 36
    lda _sum_red_field__local_0
    ldx _sum_red_field__local_0+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _sum_green_field
; SAC inline storage: 6 bytes
    _sum_green_field__local_0: .word 0
    _sum_green_field__local_2: .word 0
    _sum_green_field__local_6: .word 0
    proc _sum_green_field
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "src/test-resources/test_phase95_dead_code.c", 22
    .local @_l_sum = 0
    .local @_l_x = 4
    .local @_l_y = 2
; .debug_var: __sum_green_field @_l_sum offset=0 size=2 type=int16 scope=local
; .debug_var: __sum_green_field @_l_x offset=4 size=2 type=int16 scope=local
; .debug_var: __sum_green_field @_l_y offset=2 size=2 type=int16 scope=local

@entry:
    .loc "src/test-resources/test_phase95_dead_code.c", 42
    lda #0
    sta _sum_green_field__local_0
    sta _sum_green_field__local_0+1
    .loc "src/test-resources/test_phase95_dead_code.c", 43
    lda #0
    sta _sum_green_field__local_2
    sta _sum_green_field__local_2+1
@for_cond16:
    lda _sum_green_field__local_2
    ldx _sum_green_field__local_2+1
    cmp.16 .AX, #16
    bcc @for_body17
    bra @for_end19
@for_body17:
    .loc "src/test-resources/test_phase95_dead_code.c", 44
    lda #0
    sta _sum_green_field__local_6
    sta _sum_green_field__local_6+1
@for_cond20_ph:
    .loc "src/test-resources/test_phase95_dead_code.c", 45
    lda _sum_green_field__local_2
    ldx _sum_green_field__local_2+1
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $26
    stx $26+1
@for_cond20:
    .loc "src/test-resources/test_phase95_dead_code.c", 44
    lda _sum_green_field__local_6
    ldx _sum_green_field__local_6+1
    cmp.16 .AX, #16
    bcc @for_body21
    bra @for_end23
@for_body21:
    .loc "src/test-resources/test_phase95_dead_code.c", 45
    lda _sum_green_field__local_6
    ldx _sum_green_field__local_6+1
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
    lda _sum_green_field__local_0
    ldx _sum_green_field__local_0+1
    add.16 .AX, $32
    sta $34
    stx $35
    sta _sum_green_field__local_0
    stx _sum_green_field__local_0+1
@for_inc22:
    .loc "src/test-resources/test_phase95_dead_code.c", 44
    lda _sum_green_field__local_6
    ldx _sum_green_field__local_6+1
    sta $36
    stx $37
    ; [peephole-opt]     lda $36
    clc
    adc #1
    sta $38
    lda $37
    adc #0
    sta $39
    ; [peephole-opt]     lda $38
    ldx $39
    sta _sum_green_field__local_6
    stx _sum_green_field__local_6+1
    bra @for_cond20
@for_end23:
@for_inc18:
    .loc "src/test-resources/test_phase95_dead_code.c", 43
    lda _sum_green_field__local_2
    ldx _sum_green_field__local_2+1
    sta $3A
    stx $3B
    ; [peephole-opt]     lda $3A
    ; [peephole-opt]     ldx $3B
    add.16 .AX, #1
    sta _sum_green_field__local_2
    stx _sum_green_field__local_2+1
    bra @for_cond16
@for_end19:
    .loc "src/test-resources/test_phase95_dead_code.c", 48
    lda _sum_green_field__local_0
    ldx _sum_green_field__local_0+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _sum_all_fields
; SAC inline storage: 6 bytes
    _sum_all_fields__local_0: .word 0
    _sum_all_fields__local_2: .word 0
    _sum_all_fields__local_6: .word 0
    proc _sum_all_fields
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "src/test-resources/test_phase95_dead_code.c", 34
    .local @_l_sum = 0
    .local @_l_x = 4
    .local @_l_y = 2
; .debug_var: __sum_all_fields @_l_sum offset=0 size=2 type=int16 scope=local
; .debug_var: __sum_all_fields @_l_x offset=4 size=2 type=int16 scope=local
; .debug_var: __sum_all_fields @_l_y offset=2 size=2 type=int16 scope=local

@entry:
    .loc "src/test-resources/test_phase95_dead_code.c", 54
    lda #0
    sta _sum_all_fields__local_0
    sta _sum_all_fields__local_0+1
    .loc "src/test-resources/test_phase95_dead_code.c", 55
    lda #0
    sta _sum_all_fields__local_2
    sta _sum_all_fields__local_2+1
@for_cond24:
    lda _sum_all_fields__local_2
    ldx _sum_all_fields__local_2+1
    cmp.16 .AX, #16
    bcc @for_body25
    bra @for_end27
@for_body25:
    .loc "src/test-resources/test_phase95_dead_code.c", 56
    lda #0
    sta _sum_all_fields__local_6
    sta _sum_all_fields__local_6+1
@for_cond28_ph:
    .loc "src/test-resources/test_phase95_dead_code.c", 57
    lda _sum_all_fields__local_2
    ldx _sum_all_fields__local_2+1
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $26
    stx $26+1
    lda $26
    ldx $27
    sta $28
    stx $29
    lda $26
    ldx $27
    sta $2A
    stx $2B
@for_cond28:
    .loc "src/test-resources/test_phase95_dead_code.c", 56
    lda _sum_all_fields__local_6
    ldx _sum_all_fields__local_6+1
    cmp.16 .AX, #16
    bcc @for_body29
    bra @for_end31
@for_body29:
    .loc "src/test-resources/test_phase95_dead_code.c", 57
    lda _sum_all_fields__local_6
    ldx _sum_all_fields__local_6+1
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
    sta $30
    stx $30+1
    lda $30
    ldx $31
    sta $32
    stx $33
    ldy #0
    lda ($30),y
    ldx #0
    sta $34
    lda $30
    ldx $31
    sta $36
    stx $37
    lda $30
    ldx $31
    add.16 .AX, #1
    sta $38
    stx $39
    ldy #0
    lda ($38),y
    ldx #0
    sta $3A
    ; [peephole-opt]     lda $34
    ldx #0
    ldx #0
    sta $3C
    stx $3D
    ; [peephole-opt]     lda $3A
    ldx #0
    ldx #0
    sta $3E
    stx $3F
    ; [peephole-opt]     lda $3C
    ; [peephole-opt]     ldx $3D
    clc
    adc $3E
    sta $40
    stx $41
    lda $30
    ldx $31
    sta $42
    stx $43
    lda $30
    ldx $31
    add.16 .AX, #2
    sta $44
    stx $45
    ldy #0
    lda ($44),y
    ldx #0
    sta $46
    ; [peephole-opt]     lda $40
    ; [peephole-opt]     ldx $41
    sta $48
    ; [peephole-opt]     lda $40
    ; [peephole-opt]     ldx $41
    sta $4A
    stx $4B
    ; [peephole-opt]     lda $46
    ldx #0
    ldx #0
    sta $4C
    stx $4D
    ; [peephole-opt]     lda $4A
    clc
    adc $4C
    sta $4E
    lda $4B
    adc $4C+1
    sta $4F
    lda _sum_all_fields__local_0
    ldx _sum_all_fields__local_0+1
    add.16 .AX, $4E
    sta $50
    stx $51
    sta _sum_all_fields__local_0
    stx _sum_all_fields__local_0+1
@for_inc30:
    .loc "src/test-resources/test_phase95_dead_code.c", 56
    lda _sum_all_fields__local_6
    ldx _sum_all_fields__local_6+1
    sta $52
    stx $53
    ; [peephole-opt]     lda $52
    clc
    adc #1
    sta $54
    lda $53
    adc #0
    sta $55
    ; [peephole-opt]     lda $54
    ldx $55
    sta _sum_all_fields__local_6
    stx _sum_all_fields__local_6+1
    bra @for_cond28
@for_end31:
@for_inc26:
    .loc "src/test-resources/test_phase95_dead_code.c", 55
    lda _sum_all_fields__local_2
    ldx _sum_all_fields__local_2+1
    sta $56
    stx $57
    ; [peephole-opt]     lda $56
    ; [peephole-opt]     ldx $57
    add.16 .AX, #1
    sta _sum_all_fields__local_2
    stx _sum_all_fields__local_2+1
    bra @for_cond24
@for_end27:
    .loc "src/test-resources/test_phase95_dead_code.c", 60
    lda _sum_all_fields__local_0
    ldx _sum_all_fields__local_0+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _sum_r_and_g
; SAC inline storage: 6 bytes
    _sum_r_and_g__local_0: .word 0
    _sum_r_and_g__local_2: .word 0
    _sum_r_and_g__local_6: .word 0
    proc _sum_r_and_g
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "src/test-resources/test_phase95_dead_code.c", 46
    .local @_l_sum = 0
    .local @_l_x = 4
    .local @_l_y = 2
; .debug_var: __sum_r_and_g @_l_sum offset=0 size=2 type=int16 scope=local
; .debug_var: __sum_r_and_g @_l_x offset=4 size=2 type=int16 scope=local
; .debug_var: __sum_r_and_g @_l_y offset=2 size=2 type=int16 scope=local

@entry:
    .loc "src/test-resources/test_phase95_dead_code.c", 66
    lda #0
    sta _sum_r_and_g__local_0
    sta _sum_r_and_g__local_0+1
    .loc "src/test-resources/test_phase95_dead_code.c", 67
    lda #0
    sta _sum_r_and_g__local_2
    sta _sum_r_and_g__local_2+1
@for_cond32:
    lda _sum_r_and_g__local_2
    ldx _sum_r_and_g__local_2+1
    cmp.16 .AX, #16
    bcc @for_body33
    bra @for_end35
@for_body33:
    .loc "src/test-resources/test_phase95_dead_code.c", 68
    lda #0
    sta _sum_r_and_g__local_6
    sta _sum_r_and_g__local_6+1
@for_cond36_ph:
    .loc "src/test-resources/test_phase95_dead_code.c", 69
    lda _sum_r_and_g__local_2
    ldx _sum_r_and_g__local_2+1
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $26
    stx $26+1
    lda $26
    ldx $27
    sta $28
    stx $29
@for_cond36:
    .loc "src/test-resources/test_phase95_dead_code.c", 68
    lda _sum_r_and_g__local_6
    ldx _sum_r_and_g__local_6+1
    cmp.16 .AX, #16
    bcc @for_body37
    bra @for_end39
@for_body37:
    .loc "src/test-resources/test_phase95_dead_code.c", 69
    lda _sum_r_and_g__local_6
    ldx _sum_r_and_g__local_6+1
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
    lda $2E
    ldx $2F
    sta $30
    stx $31
    ldy #0
    lda ($2E),y
    ldx #0
    sta $32
    lda $2E
    ldx $2F
    sta $34
    stx $35
    lda $2E
    ldx $2F
    add.16 .AX, #1
    sta $36
    stx $37
    ldy #0
    lda ($36),y
    ldx #0
    sta $38
    ; [peephole-opt]     lda $32
    ldx #0
    ldx #0
    sta $3A
    stx $3B
    ; [peephole-opt]     lda $38
    ldx #0
    ldx #0
    sta $3C
    stx $3D
    ; [peephole-opt]     lda $3A
    clc
    adc $3C
    sta $3E
    lda $3B
    adc $3C+1
    sta $3F
    lda _sum_r_and_g__local_0
    ldx _sum_r_and_g__local_0+1
    add.16 .AX, $3E
    sta $40
    stx $41
    sta _sum_r_and_g__local_0
    stx _sum_r_and_g__local_0+1
@for_inc38:
    .loc "src/test-resources/test_phase95_dead_code.c", 68
    lda _sum_r_and_g__local_6
    ldx _sum_r_and_g__local_6+1
    sta $42
    stx $43
    ; [peephole-opt]     lda $42
    clc
    adc #1
    sta $44
    lda $43
    adc #0
    sta $45
    ; [peephole-opt]     lda $44
    ldx $45
    sta _sum_r_and_g__local_6
    stx _sum_r_and_g__local_6+1
    bra @for_cond36
@for_end39:
@for_inc34:
    .loc "src/test-resources/test_phase95_dead_code.c", 67
    lda _sum_r_and_g__local_2
    ldx _sum_r_and_g__local_2+1
    sta $46
    stx $47
    ; [peephole-opt]     lda $46
    ; [peephole-opt]     ldx $47
    add.16 .AX, #1
    sta _sum_r_and_g__local_2
    stx _sum_r_and_g__local_2+1
    bra @for_cond32
@for_end35:
    .loc "src/test-resources/test_phase95_dead_code.c", 72
    lda _sum_r_and_g__local_0
    ldx _sum_r_and_g__local_0+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _main
; SAC inline storage: 50 bytes
    _main__local_0: .word 0
    _main__local_2: .word 0
    _main__local_42: .word 0
    _main__local_64: .word 0
    _main__local_86: .word 0
    _main__local_122: .word 0
    _main__local_150: .word 0
    _main__local_152: .word 0
    _main__local_154: .word 0
    _main__local_156: .word 0
    _main__local_158: .word 0
    _main__local_181: .word 0
    _main__local_183: .word 0
    _main__local_187: .word 0
    _main__local_202: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "src/test-resources/test_phase95_dead_code.c", 56
    .local @_l_all_sum = 22
    .local @_l_g_sum = 14
    .local @_l_idx = 0
    .local @_l_r_sum = 6
    .local @_l_rg_sum = 30
    .local @_l_sum = 44
    .local @_l_x = 48
    .local @_l_y = 46
; .debug_var: __main @_l_all_sum offset=22 size=2 type=int16 scope=local
; .debug_var: __main @_l_g_sum offset=14 size=2 type=int16 scope=local
; .debug_var: __main @_l_idx offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_r_sum offset=6 size=2 type=int16 scope=local
; .debug_var: __main @_l_rg_sum offset=30 size=2 type=int16 scope=local
; .debug_var: __main @_l_sum offset=44 size=2 type=int16 scope=local
; .debug_var: __main @_l_x offset=48 size=2 type=int16 scope=local
; .debug_var: __main @_l_y offset=46 size=2 type=int16 scope=local

@entry:
    .loc "src/test-resources/test_phase95_dead_code.c", 76
    ldax #__str_40
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
    .loc "src/test-resources/test_phase95_dead_code.c", 16
    lda #0
    sta _main__local_2
    sta _main__local_2+1
    .loc "src/test-resources/test_phase95_dead_code.c", 17
    lda #0
    sta $20
    sta $21
@for_cond42:
    ; [peephole-opt]     lda $20
    ldx $21
    cmp.16 .AX, #16
    bcc @for_body43
    bra @for_end45
@for_body43:
    .loc "src/test-resources/test_phase95_dead_code.c", 18
    lda #0
    sta $28
    sta $29
@for_cond46_ph:
    .loc "src/test-resources/test_phase95_dead_code.c", 19
    lda $20
    ldx $21
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2A
    stx $2A+1
    .loc "src/test-resources/test_phase95_dead_code.c", 20
    lda $2A
    ldx $2B
    sta $2C
    stx $2D
    .loc "src/test-resources/test_phase95_dead_code.c", 21
    lda $2A
    ldx $2B
    sta $2E
    stx $2F
@for_cond46:
    .loc "src/test-resources/test_phase95_dead_code.c", 18
    lda $28
    ldx $29
    cmp.16 .AX, #16
    bcc @for_body47
    bra @for_end49
@for_body47:
    .loc "src/test-resources/test_phase95_dead_code.c", 19
    lda #17
    ldx #0
    sta $34
    stx $35
    lda _main__local_2
    ldx _main__local_2+1
    mul.16 .AX, $34
    sta $36
    stx $37
    lda #255
    ldx #0
    sta $38
    stx $39
    ; [peephole-opt]     lda $36
    ; [peephole-opt]     ldx $37
    and $38
    sta $3A
    stx $3B
    ; [peephole-opt]     lda $3A
    ; [peephole-opt]     ldx $3B
    sta $3C
    lda $28
    ldx $29
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
    sta $3E
    stx $3E+1
    lda $3E
    ldx $3F
    sta $40
    stx $41
    ; [peephole-opt]     lda $3A
    ldy #0
    sta ($3E),y
    .loc "src/test-resources/test_phase95_dead_code.c", 20
    lda #23
    ldx #0
    sta $42
    stx $43
    lda _main__local_2
    ldx _main__local_2+1
    mul.16 .AX, $42
    sta $44
    stx $45
    lda #255
    ldx #0
    sta $46
    stx $47
    ; [peephole-opt]     lda $44
    ; [peephole-opt]     ldx $45
    and $46
    sta $48
    stx $49
    ; [peephole-opt]     lda $48
    ; [peephole-opt]     ldx $49
    sta $4A
    lda $3E
    ldx $3F
    sta $4C
    stx $4D
    lda $3E
    ldx $3F
    add.16 .AX, #1
    sta $4E
    stx $4F
    ; [peephole-opt]     lda $48
    ldy #0
    sta ($4E),y
    .loc "src/test-resources/test_phase95_dead_code.c", 21
    lda #31
    ldx #0
    sta $50
    stx $51
    lda _main__local_2
    ldx _main__local_2+1
    mul.16 .AX, $50
    sta $52
    stx $53
    lda #255
    ldx #0
    sta $54
    stx $55
    ; [peephole-opt]     lda $52
    ; [peephole-opt]     ldx $53
    and $54
    sta $56
    stx $57
    ; [peephole-opt]     lda $56
    ; [peephole-opt]     ldx $57
    sta $58
    lda $3E
    ldx $3F
    sta $5A
    stx $5B
    lda $3E
    ldx $3F
    add.16 .AX, #2
    sta $5C
    stx $5D
    ; [peephole-opt]     lda $56
    ldy #0
    sta ($5C),y
    .loc "src/test-resources/test_phase95_dead_code.c", 22
    lda _main__local_2
    ldx _main__local_2+1
    sta $5E
    stx $5F
    ; [peephole-opt]     lda $5E
    clc
    adc #1
    sta $60
    lda $5F
    adc #0
    sta $61
    ; [peephole-opt]     lda $60
    ldx $61
    sta _main__local_2
    stx _main__local_2+1
@for_inc48:
    .loc "src/test-resources/test_phase95_dead_code.c", 18
    lda $28
    ldx $29
    sta $62
    stx $63
    ; [peephole-opt]     lda $62
    clc
    adc #1
    sta $28
    lda $63
    adc #0
    sta $29
    bra @for_cond46
@for_end49:
@for_inc44:
    .loc "src/test-resources/test_phase95_dead_code.c", 17
    lda $20
    ldx $21
    sta $66
    stx $67
    ; [peephole-opt]     lda $66
    clc
    adc #1
    sta $20
    lda $67
    adc #0
    sta $21
    bra @for_cond42
@for_end45:
@inline_end41:
    .loc "src/test-resources/test_phase95_dead_code.c", 30
    lda #0
    sta $20
    sta $21
    .loc "src/test-resources/test_phase95_dead_code.c", 31
    lda #0
    sta $22
    sta $23
@for_cond51:
    ; [peephole-opt]     lda $22
    ldx $23
    cmp.16 .AX, #16
    bcc @for_body52
    bra @for_end54
@for_body52:
    .loc "src/test-resources/test_phase95_dead_code.c", 32
    lda #0
    sta $2A
    sta $2B
@for_cond55_ph:
    .loc "src/test-resources/test_phase95_dead_code.c", 33
    lda $22
    ldx $23
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2C
    stx $2C+1
@for_cond55:
    .loc "src/test-resources/test_phase95_dead_code.c", 32
    ; [peephole-opt]     lda $2A
    ldx $2B
    cmp.16 .AX, #16
    bcc @for_body56
    bra @for_end58
@for_body56:
    .loc "src/test-resources/test_phase95_dead_code.c", 33
    lda $2A
    ldx $2B
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
    sta $32
    stx $32+1
    lda $32
    ldx $33
    sta $34
    stx $35
    ldy #0
    lda ($32),y
    ldx #0
    sta $36
    ; [peephole-opt]     lda $36
    ldx #0
    ldx #0
    sta $38
    stx $39
    lda $20
    clc
    adc $38
    sta $20
    lda $21
    adc $38+1
    sta $21
@for_inc57:
    .loc "src/test-resources/test_phase95_dead_code.c", 32
    lda $2A
    ldx $2B
    sta $3C
    stx $3D
    ; [peephole-opt]     lda $3C
    clc
    adc #1
    sta $2A
    lda $3D
    adc #0
    sta $2B
    bra @for_cond55
@for_end58:
@for_inc53:
    .loc "src/test-resources/test_phase95_dead_code.c", 31
    lda $22
    ldx $23
    sta $40
    stx $41
    ; [peephole-opt]     lda $40
    clc
    adc #1
    sta $22
    lda $41
    adc #0
    sta $23
    bra @for_cond51
@for_end54:
    .loc "src/test-resources/test_phase95_dead_code.c", 36
    lda $20
    ldx $21
@inline_end50:
    .loc "src/test-resources/test_phase95_dead_code.c", 80
    sta _main__local_42
    stx _main__local_42+1
    .loc "src/test-resources/test_phase95_dead_code.c", 42
    lda #0
    sta $20
    sta $21
    .loc "src/test-resources/test_phase95_dead_code.c", 43
    lda #0
    sta $22
    sta $23
@for_cond61:
    ; [peephole-opt]     lda $22
    ldx $23
    cmp.16 .AX, #16
    bcc @for_body62
    bra @for_end64
@for_body62:
    .loc "src/test-resources/test_phase95_dead_code.c", 44
    lda #0
    sta $2A
    sta $2B
@for_cond65_ph:
    .loc "src/test-resources/test_phase95_dead_code.c", 45
    lda $22
    ldx $23
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2C
    stx $2C+1
@for_cond65:
    .loc "src/test-resources/test_phase95_dead_code.c", 44
    ; [peephole-opt]     lda $2A
    ldx $2B
    cmp.16 .AX, #16
    bcc @for_body66
    bra @for_end68
@for_body66:
    .loc "src/test-resources/test_phase95_dead_code.c", 45
    lda $2A
    ldx $2B
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
    sta $32
    stx $32+1
    lda $32
    ldx $33
    add.16 .AX, #1
    sta $34
    stx $35
    ldy #0
    lda ($34),y
    ldx #0
    sta $36
    ; [peephole-opt]     lda $36
    ldx #0
    ldx #0
    sta $38
    stx $39
    lda $20
    clc
    adc $38
    sta $20
    lda $21
    adc $38+1
    sta $21
@for_inc67:
    .loc "src/test-resources/test_phase95_dead_code.c", 44
    lda $2A
    ldx $2B
    sta $3C
    stx $3D
    ; [peephole-opt]     lda $3C
    clc
    adc #1
    sta $2A
    lda $3D
    adc #0
    sta $2B
    bra @for_cond65
@for_end68:
@for_inc63:
    .loc "src/test-resources/test_phase95_dead_code.c", 43
    lda $22
    ldx $23
    sta $40
    stx $41
    ; [peephole-opt]     lda $40
    clc
    adc #1
    sta $22
    lda $41
    adc #0
    sta $23
    bra @for_cond61
@for_end64:
    .loc "src/test-resources/test_phase95_dead_code.c", 48
    lda $20
    ldx $21
@inline_end60:
    .loc "src/test-resources/test_phase95_dead_code.c", 81
    sta _main__local_64
    stx _main__local_64+1
    .loc "src/test-resources/test_phase95_dead_code.c", 54
    lda #0
    sta $20
    sta $21
    .loc "src/test-resources/test_phase95_dead_code.c", 55
    lda #0
    sta $22
    sta $23
@for_cond71:
    ; [peephole-opt]     lda $22
    ldx $23
    cmp.16 .AX, #16
    bcc @for_body72
    bra @for_end74
@for_body72:
    .loc "src/test-resources/test_phase95_dead_code.c", 56
    lda #0
    sta $2A
    sta $2B
@for_cond75_ph:
    .loc "src/test-resources/test_phase95_dead_code.c", 57
    lda $22
    ldx $23
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_pixels
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
    lda $2C
    ldx $2D
    sta $30
    stx $31
@for_cond75:
    .loc "src/test-resources/test_phase95_dead_code.c", 56
    ; [peephole-opt]     lda $2A
    ldx $2B
    cmp.16 .AX, #16
    bcc @for_body76
    bra @for_end78
@for_body76:
    .loc "src/test-resources/test_phase95_dead_code.c", 57
    lda $2A
    ldx $2B
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
    lda $36
    ldx $37
    sta $3C
    stx $3D
    lda $36
    ldx $37
    add.16 .AX, #1
    sta $3E
    stx $3F
    ldy #0
    lda ($3E),y
    ldx #0
    sta $40
    ; [peephole-opt]     lda $3A
    ldx #0
    ldx #0
    sta $42
    stx $43
    ; [peephole-opt]     lda $40
    ldx #0
    ldx #0
    sta $44
    stx $45
    ; [peephole-opt]     lda $42
    ; [peephole-opt]     ldx $43
    clc
    adc $44
    sta $46
    stx $47
    lda $36
    ldx $37
    sta $48
    stx $49
    lda $36
    ldx $37
    add.16 .AX, #2
    sta $4A
    stx $4B
    ldy #0
    lda ($4A),y
    ldx #0
    sta $4C
    ; [peephole-opt]     lda $46
    ; [peephole-opt]     ldx $47
    sta $4E
    ; [peephole-opt]     lda $46
    ; [peephole-opt]     ldx $47
    sta $50
    stx $51
    ; [peephole-opt]     lda $4C
    ldx #0
    ldx #0
    sta $52
    stx $53
    ; [peephole-opt]     lda $50
    clc
    adc $52
    sta $54
    lda $51
    adc $52+1
    sta $55
    lda $20
    clc
    adc $54
    sta $20
    lda $21
    adc $54+1
    sta $21
@for_inc77:
    .loc "src/test-resources/test_phase95_dead_code.c", 56
    lda $2A
    ldx $2B
    sta $58
    stx $59
    ; [peephole-opt]     lda $58
    clc
    adc #1
    sta $2A
    lda $59
    adc #0
    sta $2B
    bra @for_cond75
@for_end78:
@for_inc73:
    .loc "src/test-resources/test_phase95_dead_code.c", 55
    lda $22
    ldx $23
    sta $5C
    stx $5D
    ; [peephole-opt]     lda $5C
    clc
    adc #1
    sta $22
    lda $5D
    adc #0
    sta $23
    bra @for_cond71
@for_end74:
    .loc "src/test-resources/test_phase95_dead_code.c", 60
    lda $20
    ldx $21
@inline_end70:
    .loc "src/test-resources/test_phase95_dead_code.c", 82
    sta _main__local_86
    stx _main__local_86+1
    .loc "src/test-resources/test_phase95_dead_code.c", 66
    lda #0
    sta $20
    sta $21
    .loc "src/test-resources/test_phase95_dead_code.c", 67
    lda #0
    sta $22
    sta $23
@for_cond81:
    ; [peephole-opt]     lda $22
    ldx $23
    cmp.16 .AX, #16
    bcc @for_body82
    bra @for_end84
@for_body82:
    .loc "src/test-resources/test_phase95_dead_code.c", 68
    lda #0
    sta $2A
    sta $2B
@for_cond85_ph:
    .loc "src/test-resources/test_phase95_dead_code.c", 69
    lda $22
    ldx $23
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_pixels
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
@for_cond85:
    .loc "src/test-resources/test_phase95_dead_code.c", 68
    ; [peephole-opt]     lda $2A
    ldx $2B
    cmp.16 .AX, #16
    bcc @for_body86
    bra @for_end88
@for_body86:
    .loc "src/test-resources/test_phase95_dead_code.c", 69
    lda $2A
    ldx $2B
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
    sta $34
    stx $34+1
    lda $34
    ldx $35
    sta $36
    stx $37
    ldy #0
    lda ($34),y
    ldx #0
    sta $38
    lda $34
    ldx $35
    sta $3A
    stx $3B
    lda $34
    ldx $35
    add.16 .AX, #1
    sta $3C
    stx $3D
    ldy #0
    lda ($3C),y
    ldx #0
    sta $3E
    ; [peephole-opt]     lda $38
    ldx #0
    ldx #0
    sta $40
    stx $41
    ; [peephole-opt]     lda $3E
    ldx #0
    ldx #0
    sta $42
    stx $43
    ; [peephole-opt]     lda $40
    clc
    adc $42
    sta $44
    lda $41
    adc $42+1
    sta $45
    lda $20
    clc
    adc $44
    sta $20
    lda $21
    adc $44+1
    sta $21
@for_inc87:
    .loc "src/test-resources/test_phase95_dead_code.c", 68
    lda $2A
    ldx $2B
    sta $48
    stx $49
    ; [peephole-opt]     lda $48
    clc
    adc #1
    sta $2A
    lda $49
    adc #0
    sta $2B
    bra @for_cond85
@for_end88:
@for_inc83:
    .loc "src/test-resources/test_phase95_dead_code.c", 67
    lda $22
    ldx $23
    sta $4C
    stx $4D
    ; [peephole-opt]     lda $4C
    clc
    adc #1
    sta $22
    lda $4D
    adc #0
    sta $23
    bra @for_cond81
@for_end84:
    .loc "src/test-resources/test_phase95_dead_code.c", 72
    lda $20
    ldx $21
@inline_end80:
    .loc "src/test-resources/test_phase95_dead_code.c", 83
    sta _main__local_122
    stx _main__local_122+1
    .loc "src/test-resources/test_phase95_dead_code.c", 85
    ldax #__str_90
    sta _main__local_150
    stx _main__local_150+1
    lda _main__local_42
    ldx _main__local_42+1
    sta $28
    stx $29
    lda _main__local_150
    ; [peephole-opt]     ldx _main__local_150+1
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
    .loc "src/test-resources/test_phase95_dead_code.c", 86
    ldax #__str_91
    sta _main__local_152
    stx _main__local_152+1
    lda _main__local_64
    ldx _main__local_64+1
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
    .loc "src/test-resources/test_phase95_dead_code.c", 87
    ldax #__str_92
    sta _main__local_154
    stx _main__local_154+1
    lda _main__local_86
    ldx _main__local_86+1
    sta $28
    stx $29
    lda _main__local_154
    ; [peephole-opt]     ldx _main__local_154+1
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
    .loc "src/test-resources/test_phase95_dead_code.c", 88
    ldax #__str_93
    sta _main__local_156
    stx _main__local_156+1
    lda _main__local_122
    ldx _main__local_122+1
    sta $28
    stx $29
    lda _main__local_156
    ; [peephole-opt]     ldx _main__local_156+1
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
    .loc "src/test-resources/test_phase95_dead_code.c", 89
    ldax #__str_94
    sta _main__local_158
    stx _main__local_158+1
    .loc "src/test-resources/test_phase95_dead_code.c", 30
    lda #0
    sta $20
    sta $21
    .loc "src/test-resources/test_phase95_dead_code.c", 31
    lda #0
    sta $22
    sta $23
@for_cond96:
    ; [peephole-opt]     lda $22
    ldx $23
    cmp.16 .AX, #16
    bcc @for_body97
    bra @for_end99
@for_body97:
    .loc "src/test-resources/test_phase95_dead_code.c", 32
    lda #0
    sta $2A
    sta $2B
@for_cond100_ph:
    .loc "src/test-resources/test_phase95_dead_code.c", 33
    lda $22
    ldx $23
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $2C
    stx $2C+1
@for_cond100:
    .loc "src/test-resources/test_phase95_dead_code.c", 32
    ; [peephole-opt]     lda $2A
    ldx $2B
    cmp.16 .AX, #16
    bcc @for_body101
    bra @for_end103
@for_body101:
    .loc "src/test-resources/test_phase95_dead_code.c", 33
    lda $2A
    ldx $2B
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
    sta $32
    stx $32+1
    lda $32
    ldx $33
    sta $34
    stx $35
    ldy #0
    lda ($32),y
    ldx #0
    sta $36
    ; [peephole-opt]     lda $36
    ldx #0
    ldx #0
    sta $38
    stx $39
    lda $20
    clc
    adc $38
    sta $20
    lda $21
    adc $38+1
    sta $21
@for_inc102:
    .loc "src/test-resources/test_phase95_dead_code.c", 32
    lda $2A
    ldx $2B
    sta $3C
    stx $3D
    ; [peephole-opt]     lda $3C
    clc
    adc #1
    sta $2A
    lda $3D
    adc #0
    sta $2B
    bra @for_cond100
@for_end103:
@for_inc98:
    .loc "src/test-resources/test_phase95_dead_code.c", 31
    lda $22
    ldx $23
    sta $40
    stx $41
    ; [peephole-opt]     lda $40
    clc
    adc #1
    sta $22
    lda $41
    adc #0
    sta $23
    bra @for_cond96
@for_end99:
    .loc "src/test-resources/test_phase95_dead_code.c", 36
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end95:
    .loc "src/test-resources/test_phase95_dead_code.c", 42
    lda #0
    sta _main__local_181
    sta _main__local_181+1
    .loc "src/test-resources/test_phase95_dead_code.c", 43
    lda #0
    sta _main__local_183
    sta _main__local_183+1
@for_cond106:
    lda _main__local_183
    ldx _main__local_183+1
    cmp.16 .AX, #16
    bcc @for_body107
    bra @for_end109
@for_body107:
    .loc "src/test-resources/test_phase95_dead_code.c", 44
    lda #0
    sta _main__local_187
    sta _main__local_187+1
@for_cond110_ph:
    .loc "src/test-resources/test_phase95_dead_code.c", 45
    lda _main__local_183
    ldx _main__local_183+1
    mul.16 .AX, #48
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_pixels
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $28
    stx $28+1
@for_cond110:
    .loc "src/test-resources/test_phase95_dead_code.c", 44
    lda _main__local_187
    ldx _main__local_187+1
    cmp.16 .AX, #16
    bcc @for_body111
    bra @for_end113
@for_body111:
    .loc "src/test-resources/test_phase95_dead_code.c", 45
    lda _main__local_187
    ldx _main__local_187+1
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
    sta $2E
    stx $2E+1
    lda $2E
    ldx $2F
    add.16 .AX, #1
    sta $30
    stx $31
    ldy #0
    lda ($30),y
    ldx #0
    sta $32
    ; [peephole-opt]     lda $32
    ldx #0
    ldx #0
    sta $34
    stx $35
    lda _main__local_181
    ldx _main__local_181+1
    add.16 .AX, $34
    sta $36
    stx $37
    sta _main__local_181
    stx _main__local_181+1
@for_inc112:
    .loc "src/test-resources/test_phase95_dead_code.c", 44
    lda _main__local_187
    ldx _main__local_187+1
    sta $38
    stx $39
    ; [peephole-opt]     lda $38
    clc
    adc #1
    sta $3A
    lda $39
    adc #0
    sta $3B
    ; [peephole-opt]     lda $3A
    ldx $3B
    sta _main__local_187
    stx _main__local_187+1
    bra @for_cond110
@for_end113:
@for_inc108:
    .loc "src/test-resources/test_phase95_dead_code.c", 43
    lda _main__local_183
    ldx _main__local_183+1
    sta $3C
    stx $3D
    ; [peephole-opt]     lda $3C
    ; [peephole-opt]     ldx $3D
    add.16 .AX, #1
    sta _main__local_183
    stx _main__local_183+1
    bra @for_cond106
@for_end109:
    .loc "src/test-resources/test_phase95_dead_code.c", 48
    lda _main__local_181
    ldx _main__local_181+1
@inline_end105:
    .loc "src/test-resources/test_phase95_dead_code.c", 89
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $22
    ldx $23
    add.16 .AX, __zp_scratch2
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_86
    ldx _main__local_86+1
    sub.16 .AX, __zp_scratch2
    sta _main__local_202
    stx _main__local_202+1
    lda _main__local_202
    ; [peephole-opt]     ldx _main__local_202+1
    sta $28
    stx $29
    lda _main__local_122
    ldx _main__local_122+1
    sta $2A
    stx $2B
    lda _main__local_158
    ldx _main__local_158+1
    sta $2C
    stx $2D
    ; [peephole-opt]     lda $28
    ; [peephole-opt]     ldx $29
    push .ax
    ; [peephole-opt]     lda $2A
    ; [peephole-opt]     ldx $2B
    push .ax
    ; [peephole-opt]     lda $2C
    ; [peephole-opt]     ldx $2D
    push .ax
    jsr _printf
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    plz
    lda __zp_scratch4
    ; [peephole-opt]     ldx __zp_scratch4+1
    .loc "src/test-resources/test_phase95_dead_code.c", 91
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 50
    endproc


    .segment "data"
__str_40:
    .text "Phase 95.6: Dead code elimination test
"
    .byte 0
__str_90:
    .text "Red sum: %d
"
    .byte 0
__str_91:
    .text "Green sum: %d
"
    .byte 0
__str_92:
    .text "All sum: %d
"
    .byte 0
__str_93:
    .text "R+G sum: %d
"
    .byte 0
__str_94:
    .text "Verification: R+G=%d, All-B=%d
"
    .byte 0

__zp_save_buf:
; [DEBUG] Phase 87 code reached, optimize=true

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

    .global _palette
    .global _entity
    .global _audio_left
    .global _audio_right
    .global _synth_state
    .global _vertices
    .global _normals
    .global _test_module1_profile
    .global _test_module2_profile
    .global _test_module3_profile
    .global _test_bank_assignments
    .global _main

    .segment "bss"
_palette:
; .debug_var: @global _palette offset=0 size=2 type=int8 scope=global
    .res 256
_entity:
; .debug_var: @global _entity offset=0 size=2 type=int16 scope=global
    .res 192
_audio_left:
; .debug_var: @global _audio_left offset=0 size=2 type=int8 scope=global
    .res 512
_audio_right:
; .debug_var: @global _audio_right offset=0 size=2 type=int8 scope=global
    .res 512
_synth_state:
; .debug_var: @global _synth_state offset=0 size=4 type=int32 scope=global
    .res 4
_vertices:
; .debug_var: @global _vertices offset=0 size=2 type=int16 scope=global
    .res 2048
_normals:
; .debug_var: @global _normals offset=0 size=2 type=int8 scope=global
    .res 1024

    .segment "code"

; function _test_module1_profile
; SAC inline storage: 2 bytes
    _test_module1_profile__local_0: .word 0
    _test_module1_profile__local_2: .word 0
    _test_module1_profile__local_15: .word 0
    _test_module1_profile__local_17: .word 0
    _test_module1_profile__local_20: .word 0
    _test_module1_profile__local_21: .word 0
    _test_module1_profile__local_25: .word 0
    _test_module1_profile__local_26: .word 0
    proc _test_module1_profile
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdio.h", 14
    .local @_l_i = 0
; .debug_var: __test_module1_profile @_l_i offset=0 size=2 type=int16 scope=local

@entry:
    .loc "src/test-resources/test_phase99_database.c", 19
    ldax #__str_0
    sta _test_module1_profile__local_0
    stx _test_module1_profile__local_0+1
    lda _test_module1_profile__local_0
    ; [peephole-opt]     ldx _test_module1_profile__local_0+1
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
    .loc "src/test-resources/test_phase99_database.c", 22
    lda #0
    sta _test_module1_profile__local_2
    sta _test_module1_profile__local_2+1
@for_cond1:
    lda _test_module1_profile__local_2
    ldx _test_module1_profile__local_2+1
    cmp.16 .AX, #256
    bcc @for_body2
    bra @for_end4
@for_body2:
    lda _test_module1_profile__local_2
    ldx _test_module1_profile__local_2+1
    sta $24
    .loc "src/test-resources/test_phase99_database.c", 23
    ; [peephole-opt]     lda $24
    ldx #0
    pha
    lda _test_module1_profile__local_2
    ldx _test_module1_profile__local_2+1
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_palette
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
@for_inc3:
    .loc "src/test-resources/test_phase99_database.c", 22
    lda _test_module1_profile__local_2
    ldx _test_module1_profile__local_2+1
    sta $28
    stx $29
    ; [peephole-opt]     lda $28
    ; [peephole-opt]     ldx $29
    add.16 .AX, #1
    sta _test_module1_profile__local_2
    stx _test_module1_profile__local_2+1
    bra @for_cond1
@for_end4:
    .loc "src/test-resources/test_phase99_database.c", 26
    lda #0
    sta _test_module1_profile__local_2
    sta _test_module1_profile__local_2+1
@for_cond5:
    lda _test_module1_profile__local_2
    ldx _test_module1_profile__local_2+1
    cmp.16 .AX, #64
    bcc @for_body6
    bra @for_end8
@for_body6:
    .loc "src/test-resources/test_phase99_database.c", 27
    lda #4
    ldx #0
    sta $24
    stx $25
    lda _test_module1_profile__local_2
    ldx _test_module1_profile__local_2+1
    lsl.16 .AX
    lsl.16 .AX
    sta $26
    stx $27
    lda _test_module1_profile__local_2
    ldx _test_module1_profile__local_2+1
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_entity
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $28
    stx $28+1
    ; [peephole-opt]     lda $26
    ldy #0
    sta ($28),y
    .loc "src/test-resources/test_phase99_database.c", 28
    lda #2
    ldx #0
    sta $2A
    stx $2B
    lda _test_module1_profile__local_2
    ldx _test_module1_profile__local_2+1
    lsl.16 .AX
    sta $2C
    stx $2D
    lda $28
    ldx $29
    add.16 .AX, #1
    sta $2E
    stx $2F
    ; [peephole-opt]     lda $2C
    ldy #0
    sta ($2E),y
    .loc "src/test-resources/test_phase99_database.c", 29
    lda #15
    ldx #0
    sta $30
    stx $31
    lda _test_module1_profile__local_2
    ldx _test_module1_profile__local_2+1
    and $30
    sta $32
    stx $33
    lda $28
    ldx $29
    add.16 .AX, #2
    sta $34
    stx $35
    ; [peephole-opt]     lda $32
    ldy #0
    sta ($34),y
@for_inc7:
    .loc "src/test-resources/test_phase99_database.c", 26
    lda _test_module1_profile__local_2
    ldx _test_module1_profile__local_2+1
    sta $36
    stx $37
    ; [peephole-opt]     lda $36
    ; [peephole-opt]     ldx $37
    add.16 .AX, #1
    sta _test_module1_profile__local_2
    stx _test_module1_profile__local_2+1
    bra @for_cond5
@for_end8:
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _test_module2_profile
; SAC inline storage: 2 bytes
    _test_module2_profile__local_0: .word 0
    _test_module2_profile__local_2: .word 0
    _test_module2_profile__local_8: .word 0
    _test_module2_profile__local_14: .word 0
    proc _test_module2_profile
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "src/test-resources/test_phase99_database.c", 22
    .local @_l_i = 0
; .debug_var: __test_module2_profile @_l_i offset=0 size=2 type=int16 scope=local

@entry:
    .loc "src/test-resources/test_phase99_database.c", 43
    ldax #__str_9
    sta _test_module2_profile__local_0
    stx _test_module2_profile__local_0+1
    lda _test_module2_profile__local_0
    ; [peephole-opt]     ldx _test_module2_profile__local_0+1
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
    .loc "src/test-resources/test_phase99_database.c", 46
    lda #0
    sta _test_module2_profile__local_2
    sta _test_module2_profile__local_2+1
@for_cond10:
    lda _test_module2_profile__local_2
    ldx _test_module2_profile__local_2+1
    cmp.16 .AX, #512
    bcc @for_body11
    bra @for_end13
@for_body11:
    .loc "src/test-resources/test_phase99_database.c", 47
    lda #255
    ldx #0
    sta $24
    stx $25
    lda _test_module2_profile__local_2
    ldx _test_module2_profile__local_2+1
    and $24
    sta $26
    stx $27
    ; [peephole-opt]     lda $26
    ; [peephole-opt]     ldx $27
    pha
    lda _test_module2_profile__local_2
    ldx _test_module2_profile__local_2+1
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_audio_left
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "src/test-resources/test_phase99_database.c", 48
    lda _test_module2_profile__local_2
    ldx _test_module2_profile__local_2+1
    add.16 .AX, #128
    sta $2C
    stx $2D
    lda #255
    ldx #0
    sta $2E
    stx $2F
    ; [peephole-opt]     lda $2C
    ; [peephole-opt]     ldx $2D
    and $2E
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    pha
    lda _test_module2_profile__local_2
    ldx _test_module2_profile__local_2+1
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_audio_right
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
@for_inc12:
    .loc "src/test-resources/test_phase99_database.c", 46
    lda _test_module2_profile__local_2
    ldx _test_module2_profile__local_2+1
    sta $34
    stx $35
    ; [peephole-opt]     lda $34
    ; [peephole-opt]     ldx $35
    add.16 .AX, #1
    sta _test_module2_profile__local_2
    stx _test_module2_profile__local_2+1
    bra @for_cond10
@for_end13:
    .loc "src/test-resources/test_phase99_database.c", 51
    lda #120
    ldx #86
    ldy #52
    ldz #18
    sta _synth_state
    stx _synth_state+1
    sty _synth_state+2
    stz _synth_state+3
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _test_module3_profile
; SAC inline storage: 4 bytes
    _test_module3_profile__local_0: .word 0
    _test_module3_profile__local_2: .word 0
    _test_module3_profile__local_11: .word 0
    _test_module3_profile__local_15: .word 0
    _test_module3_profile__local_19: .word 0
    proc _test_module3_profile
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "src/test-resources/test_phase99_database.c", 42
    .local @_l_color = 2
    .local @_l_i = 0
; .debug_var: __test_module3_profile @_l_color offset=2 size=2 type=int8 scope=local
; .debug_var: __test_module3_profile @_l_i offset=0 size=2 type=int16 scope=local

@entry:
    .loc "src/test-resources/test_phase99_database.c", 63
    ldax #__str_14
    sta _test_module3_profile__local_0
    stx _test_module3_profile__local_0+1
    lda _test_module3_profile__local_0
    ; [peephole-opt]     ldx _test_module3_profile__local_0+1
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
    .loc "src/test-resources/test_phase99_database.c", 66
    lda #0
    sta _test_module3_profile__local_2
    sta _test_module3_profile__local_2+1
@for_cond15:
    lda _test_module3_profile__local_2
    ldx _test_module3_profile__local_2+1
    cmp.16 .AX, #1024
    bcc @for_body16
    bra @for_end18
@for_body16:
    .loc "src/test-resources/test_phase99_database.c", 67
    lda #2
    ldx #0
    sta $24
    stx $25
    lda _test_module3_profile__local_2
    ldx _test_module3_profile__local_2+1
    lsl.16 .AX
    sta $26
    stx $27
    ; [peephole-opt]     lda $26
    ; [peephole-opt]     ldx $27
    pha
    phx
    lda _test_module3_profile__local_2
    ldx _test_module3_profile__local_2+1
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_vertices
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    .loc "src/test-resources/test_phase99_database.c", 68
    lda #255
    ldx #0
    sta $2A
    stx $2B
    lda _test_module3_profile__local_2
    ldx _test_module3_profile__local_2+1
    and $2A
    sta $2C
    stx $2D
    ; [peephole-opt]     lda $2C
    ; [peephole-opt]     ldx $2D
    pha
    lda _test_module3_profile__local_2
    ldx _test_module3_profile__local_2+1
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_normals
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
@for_inc17:
    .loc "src/test-resources/test_phase99_database.c", 66
    lda _test_module3_profile__local_2
    ldx _test_module3_profile__local_2+1
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    add.16 .AX, #1
    sta _test_module3_profile__local_2
    stx _test_module3_profile__local_2+1
    bra @for_cond15
@for_end18:
    .loc "src/test-resources/test_phase99_database.c", 72
    lda #42
    ldx #0
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_palette
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
    sta _test_module3_profile__local_15
    .loc "src/test-resources/test_phase99_database.c", 73
    ldax #__str_19
    sta _test_module3_profile__local_19
    stx _test_module3_profile__local_19+1
    ; [peephole-opt]     lda _test_module3_profile__local_15
    sta $28
    stx $29
    lda _test_module3_profile__local_19
    ; [peephole-opt]     ldx _test_module3_profile__local_19+1
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
    .frame_size 4
    endproc

; function _test_bank_assignments
; SAC inline storage: 0 bytes
    _test_bank_assignments__local_0: .word 0
    _test_bank_assignments__local_2: .word 0
    _test_bank_assignments__local_4: .word 0
    _test_bank_assignments__local_6: .word 0
    _test_bank_assignments__local_8: .word 0
    _test_bank_assignments__local_10: .word 0
    _test_bank_assignments__local_12: .word 0
    _test_bank_assignments__local_14: .word 0
    _test_bank_assignments__local_16: .word 0
    _test_bank_assignments__local_18: .word 0
    _test_bank_assignments__local_20: .word 0
    _test_bank_assignments__local_22: .word 0
    _test_bank_assignments__local_24: .word 0
    _test_bank_assignments__local_26: .word 0
    _test_bank_assignments__local_28: .word 0
    proc _test_bank_assignments
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "src/test-resources/test_phase99_database.c", 61

@entry:
    .loc "src/test-resources/test_phase99_database.c", 86
    ldax #__str_20
    sta _test_bank_assignments__local_0
    stx _test_bank_assignments__local_0+1
    lda _test_bank_assignments__local_0
    ; [peephole-opt]     ldx _test_bank_assignments__local_0+1
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
    .loc "src/test-resources/test_phase99_database.c", 87
    ldax #__str_21
    sta _test_bank_assignments__local_2
    stx _test_bank_assignments__local_2+1
    lda _test_bank_assignments__local_2
    ; [peephole-opt]     ldx _test_bank_assignments__local_2+1
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
    .loc "src/test-resources/test_phase99_database.c", 88
    ldax #__str_22
    sta _test_bank_assignments__local_4
    stx _test_bank_assignments__local_4+1
    lda _test_bank_assignments__local_4
    ; [peephole-opt]     ldx _test_bank_assignments__local_4+1
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
    .loc "src/test-resources/test_phase99_database.c", 89
    ldax #__str_23
    sta _test_bank_assignments__local_6
    stx _test_bank_assignments__local_6+1
    lda _test_bank_assignments__local_6
    ; [peephole-opt]     ldx _test_bank_assignments__local_6+1
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
    .loc "src/test-resources/test_phase99_database.c", 90
    ldax #__str_24
    sta _test_bank_assignments__local_8
    stx _test_bank_assignments__local_8+1
    lda _test_bank_assignments__local_8
    ; [peephole-opt]     ldx _test_bank_assignments__local_8+1
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
    .loc "src/test-resources/test_phase99_database.c", 91
    ldax #__str_25
    sta _test_bank_assignments__local_10
    stx _test_bank_assignments__local_10+1
    lda _test_bank_assignments__local_10
    ; [peephole-opt]     ldx _test_bank_assignments__local_10+1
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
    .loc "src/test-resources/test_phase99_database.c", 92
    ldax #__str_26
    sta _test_bank_assignments__local_12
    stx _test_bank_assignments__local_12+1
    lda _test_bank_assignments__local_12
    ; [peephole-opt]     ldx _test_bank_assignments__local_12+1
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
    .loc "src/test-resources/test_phase99_database.c", 93
    ldax #__str_27
    sta _test_bank_assignments__local_14
    stx _test_bank_assignments__local_14+1
    lda _test_bank_assignments__local_14
    ; [peephole-opt]     ldx _test_bank_assignments__local_14+1
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
    .loc "src/test-resources/test_phase99_database.c", 94
    ldax #__str_28
    sta _test_bank_assignments__local_16
    stx _test_bank_assignments__local_16+1
    lda _test_bank_assignments__local_16
    ; [peephole-opt]     ldx _test_bank_assignments__local_16+1
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
    .loc "src/test-resources/test_phase99_database.c", 95
    ldax #__str_29
    sta _test_bank_assignments__local_18
    stx _test_bank_assignments__local_18+1
    lda _test_bank_assignments__local_18
    ; [peephole-opt]     ldx _test_bank_assignments__local_18+1
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
    .loc "src/test-resources/test_phase99_database.c", 96
    ldax #__str_30
    sta _test_bank_assignments__local_20
    stx _test_bank_assignments__local_20+1
    lda _test_bank_assignments__local_20
    ; [peephole-opt]     ldx _test_bank_assignments__local_20+1
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
    .loc "src/test-resources/test_phase99_database.c", 97
    ldax #__str_31
    sta _test_bank_assignments__local_22
    stx _test_bank_assignments__local_22+1
    lda _test_bank_assignments__local_22
    ; [peephole-opt]     ldx _test_bank_assignments__local_22+1
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
    .loc "src/test-resources/test_phase99_database.c", 98
    ldax #__str_32
    sta _test_bank_assignments__local_24
    stx _test_bank_assignments__local_24+1
    lda _test_bank_assignments__local_24
    ; [peephole-opt]     ldx _test_bank_assignments__local_24+1
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
    .loc "src/test-resources/test_phase99_database.c", 99
    ldax #__str_33
    sta _test_bank_assignments__local_26
    stx _test_bank_assignments__local_26+1
    lda _test_bank_assignments__local_26
    ; [peephole-opt]     ldx _test_bank_assignments__local_26+1
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
    .loc "src/test-resources/test_phase99_database.c", 100
    ldax #__str_34
    sta _test_bank_assignments__local_28
    stx _test_bank_assignments__local_28+1
    lda _test_bank_assignments__local_28
    ; [peephole-opt]     ldx _test_bank_assignments__local_28+1
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
    .frame_size 0
    endproc

; function _main
; SAC inline storage: 8 bytes
    _main__local_0: .word 0
    _main__local_2: .word 0
    _main__local_4: .word 0
    _main__local_34: .word 0
    _main__local_36: .word 0
    _main__local_38: .word 0
    _main__local_57: .word 0
    _main__local_59: .word 0
    _main__local_61: .word 0
    _main__local_63: .word 0
    _main__local_76: .word 0
    _main__local_80: .word 0
    _main__local_82: .word 0
    _main__local_83: .word 0
    _main__local_84: .word 0
    _main__local_85: .word 0
    _main__local_86: .word 0
    _main__local_87: .word 0
    _main__local_88: .word 0
    _main__local_89: .word 0
    _main__local_90: .word 0
    _main__local_91: .word 0
    _main__local_92: .word 0
    _main__local_93: .word 0
    _main__local_94: .word 0
    _main__local_95: .word 0
    _main__local_96: .word 0
    _main__local_97: .word 0
    _main__local_98: .word 0
    _main__local_99: .word 0
    _main__local_100: .word 0
    _main__local_101: .word 0
    _main__local_102: .word 0
    _main__local_103: .word 0
    _main__local_104: .word 0
    _main__local_105: .word 0
    _main__local_106: .word 0
    _main__local_107: .word 0
    _main__local_108: .word 0
    _main__local_109: .word 0
    _main__local_110: .word 0
    _main__local_111: .word 0
    _main__local_112: .word 0
    _main__local_113: .word 0
    _main__local_114: .word 0
    _main__local_115: .word 0
    _main__local_116: .word 0
    _main__local_117: .word 0
    _main__local_118: .word 0
    _main__local_119: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "src/test-resources/test_phase99_database.c", 88
    .local @_l_color = 6
    .local @_l_i = 4
; .debug_var: __main @_l_color offset=6 size=2 type=int8 scope=local
; .debug_var: __main @_l_i offset=4 size=2 type=int16 scope=local

@entry:
    .loc "src/test-resources/test_phase99_database.c", 108
    ldax #__str_35
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
    .loc "src/test-resources/test_phase99_database.c", 110
    ldax #__str_36
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
    .loc "src/test-resources/test_phase99_database.c", 19
    ldax #__str_38
    sta _main__local_4
    stx _main__local_4+1
    lda _main__local_4
    ; [peephole-opt]     ldx _main__local_4+1
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
    .loc "src/test-resources/test_phase99_database.c", 22
    lda #0
    sta $20
    sta $21
@for_cond39:
    ; [peephole-opt]     lda $20
    ldx $21
    cmp.16 .AX, #256
    bcc @for_body40
    bra @for_end42
@for_body40:
    lda $20
    ldx $21
    sta $26
    .loc "src/test-resources/test_phase99_database.c", 23
    ; [peephole-opt]     lda $26
    ldx #0
    pha
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_palette
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
@for_inc41:
    .loc "src/test-resources/test_phase99_database.c", 22
    lda $20
    ldx $21
    sta $2A
    stx $2B
    ; [peephole-opt]     lda $2A
    clc
    adc #1
    sta $20
    lda $2B
    adc #0
    sta $21
    bra @for_cond39
@for_end42:
    .loc "src/test-resources/test_phase99_database.c", 26
    lda #0
    sta $20
    sta $21
@for_cond43:
    ; [peephole-opt]     lda $20
    ldx $21
    cmp.16 .AX, #64
    bcc @for_body44
    bra @for_end46
@for_body44:
    .loc "src/test-resources/test_phase99_database.c", 27
    lda #4
    ldx #0
    sta $26
    stx $27
    lda $20
    ldx $21
    lsl.16 .AX
    lsl.16 .AX
    sta $28
    stx $29
    ; [peephole-opt]     lda $28
    ; [peephole-opt]     ldx $29
    sta $2A
    lda $20
    ldx $21
    mul.16 .AX, #3
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_entity
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
    ; [peephole-opt]     lda $28
    ldy #0
    sta ($2C),y
    .loc "src/test-resources/test_phase99_database.c", 28
    lda #2
    ldx #0
    sta $30
    stx $31
    lda $20
    ldx $21
    lsl.16 .AX
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    ; [peephole-opt]     ldx $33
    sta $34
    lda $2C
    ldx $2D
    sta $36
    stx $37
    lda $2C
    ldx $2D
    add.16 .AX, #1
    sta $38
    stx $39
    ; [peephole-opt]     lda $32
    ldy #0
    sta ($38),y
    .loc "src/test-resources/test_phase99_database.c", 29
    lda #15
    ldx #0
    sta $3A
    stx $3B
    lda $20
    ldx $21
    and $3A
    sta $3C
    stx $3D
    ; [peephole-opt]     lda $3C
    ; [peephole-opt]     ldx $3D
    sta $3E
    lda $2C
    ldx $2D
    sta $40
    stx $41
    lda $2C
    ldx $2D
    add.16 .AX, #2
    sta $42
    stx $43
    ; [peephole-opt]     lda $3C
    ldy #0
    sta ($42),y
@for_inc45:
    .loc "src/test-resources/test_phase99_database.c", 26
    lda $20
    ldx $21
    sta $44
    stx $45
    ; [peephole-opt]     lda $44
    clc
    adc #1
    sta $20
    lda $45
    adc #0
    sta $21
    bra @for_cond43
@for_end46:
@inline_end37:
    .loc "src/test-resources/test_phase99_database.c", 112
    ldax #__str_47
    sta _main__local_34
    stx _main__local_34+1
    lda _main__local_34
    ; [peephole-opt]     ldx _main__local_34+1
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
    .loc "src/test-resources/test_phase99_database.c", 114
    ldax #__str_48
    sta _main__local_36
    stx _main__local_36+1
    lda _main__local_36
    ; [peephole-opt]     ldx _main__local_36+1
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
    .loc "src/test-resources/test_phase99_database.c", 43
    ldax #__str_50
    sta _main__local_38
    stx _main__local_38+1
    lda _main__local_38
    ; [peephole-opt]     ldx _main__local_38+1
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
    .loc "src/test-resources/test_phase99_database.c", 46
    lda #0
    sta $20
    sta $21
@for_cond51:
    ; [peephole-opt]     lda $20
    ldx $21
    cmp.16 .AX, #512
    bcc @for_body52
    bra @for_end54
@for_body52:
    .loc "src/test-resources/test_phase99_database.c", 47
    lda #255
    ldx #0
    sta $26
    stx $27
    lda $20
    ldx $21
    and $26
    sta $28
    stx $29
    ; [peephole-opt]     lda $28
    ; [peephole-opt]     ldx $29
    sta $2A
    ; [peephole-opt]     lda $28
    ; [peephole-opt]     ldx $29
    pha
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_audio_left
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "src/test-resources/test_phase99_database.c", 48
    lda $20
    clc
    adc #128
    sta $30
    lda $21
    adc #0
    sta $31
    lda #255
    ldx #0
    sta $32
    stx $33
    ; [peephole-opt]     lda $30
    ldx $31
    and $32
    sta $34
    stx $35
    ; [peephole-opt]     lda $34
    ; [peephole-opt]     ldx $35
    sta $36
    ; [peephole-opt]     lda $34
    ; [peephole-opt]     ldx $35
    pha
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_audio_right
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
@for_inc53:
    .loc "src/test-resources/test_phase99_database.c", 46
    lda $20
    ldx $21
    sta $3A
    stx $3B
    ; [peephole-opt]     lda $3A
    clc
    adc #1
    sta $20
    lda $3B
    adc #0
    sta $21
    bra @for_cond51
@for_end54:
    .loc "src/test-resources/test_phase99_database.c", 51
    lda #120
    ldx #86
    ldy #52
    ldz #18
    sta _synth_state
    stx _synth_state+1
    sty _synth_state+2
    stz _synth_state+3
@inline_end49:
    .loc "src/test-resources/test_phase99_database.c", 116
    ldax #__str_55
    sta _main__local_57
    stx _main__local_57+1
    lda _main__local_57
    ; [peephole-opt]     ldx _main__local_57+1
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
    .loc "src/test-resources/test_phase99_database.c", 118
    ldax #__str_56
    sta _main__local_59
    stx _main__local_59+1
    lda _main__local_59
    ; [peephole-opt]     ldx _main__local_59+1
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
    .loc "src/test-resources/test_phase99_database.c", 63
    ldax #__str_58
    sta _main__local_61
    stx _main__local_61+1
    lda _main__local_61
    ; [peephole-opt]     ldx _main__local_61+1
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
    .loc "src/test-resources/test_phase99_database.c", 66
    lda #0
    sta _main__local_63
    sta _main__local_63+1
@for_cond59:
    lda _main__local_63
    ldx _main__local_63+1
    cmp.16 .AX, #1024
    bcc @for_body60
    bra @for_end62
@for_body60:
    .loc "src/test-resources/test_phase99_database.c", 67
    lda #2
    ldx #0
    sta $24
    stx $25
    lda _main__local_63
    ldx _main__local_63+1
    lsl.16 .AX
    sta $26
    stx $27
    ; [peephole-opt]     lda $26
    ; [peephole-opt]     ldx $27
    pha
    phx
    lda _main__local_63
    ldx _main__local_63+1
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_vertices
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    .loc "src/test-resources/test_phase99_database.c", 68
    lda #255
    ldx #0
    sta $2A
    stx $2B
    lda _main__local_63
    ldx _main__local_63+1
    and $2A
    sta $2C
    stx $2D
    ; [peephole-opt]     lda $2C
    ; [peephole-opt]     ldx $2D
    sta $2E
    ; [peephole-opt]     lda $2C
    ; [peephole-opt]     ldx $2D
    pha
    lda _main__local_63
    ldx _main__local_63+1
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_normals
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
@for_inc61:
    .loc "src/test-resources/test_phase99_database.c", 66
    lda _main__local_63
    ldx _main__local_63+1
    sta $32
    stx $33
    ; [peephole-opt]     lda $32
    ; [peephole-opt]     ldx $33
    add.16 .AX, #1
    sta _main__local_63
    stx _main__local_63+1
    bra @for_cond59
@for_end62:
    .loc "src/test-resources/test_phase99_database.c", 72
    lda #42
    ldx #0
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_palette
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
    sta _main__local_76
    .loc "src/test-resources/test_phase99_database.c", 73
    ldax #__str_63
    sta _main__local_80
    stx _main__local_80+1
    ; [peephole-opt]     lda _main__local_76
    sta $28
    stx $29
    lda _main__local_80
    ; [peephole-opt]     ldx _main__local_80+1
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
    .frame_size 8
    endproc


    .segment "data"
__str_0:
    .text "Module 1: Graphics data loaded
"
    .byte 0
__str_9:
    .text "Module 2: Audio buffers loaded
"
    .byte 0
__str_14:
    .text "Module 3: Mesh data loaded
"
    .byte 0
__str_19:
    .text "Palette color at 42: %02X
"
    .byte 0
__str_20:
    .text "Bank Assignment Tests:
"
    .byte 0
__str_21:
    .text "  palette size: 256 bytes
"
    .byte 0
__str_22:
    .text "  entity size: 192 bytes (64 * 3)
"
    .byte 0
__str_23:
    .text "  Combined (Bank 0): 448 bytes
"
    .byte 0
__str_24:
    .text "
"
    .byte 0
__str_25:
    .text "  audio_left size: 512 bytes
"
    .byte 0
__str_26:
    .text "  audio_right size: 512 bytes
"
    .byte 0
__str_27:
    .text "  Combined (Bank 1): 1024 bytes
"
    .byte 0
__str_28:
    .text "
"
    .byte 0
__str_29:
    .text "  synth_state: 4 bytes
"
    .byte 0
__str_30:
    .text "  vertices: 2048 bytes
"
    .byte 0
__str_31:
    .text "  normals: 1024 bytes
"
    .byte 0
__str_32:
    .text "  Combined (Bank 2): 3076 bytes
"
    .byte 0
__str_33:
    .text "
"
    .byte 0
__str_34:
    .text "Total banks needed: 3
"
    .byte 0
__str_35:
    .text "=== Phase 99.1: Cross-Module Address Space Database ===

"
    .byte 0
__str_36:
    .text "Loading Module 1 (Graphics)...
"
    .byte 0
__str_38:
    .text "Module 1: Graphics data loaded
"
    .byte 0
__str_47:
    .text "
"
    .byte 0
__str_48:
    .text "Loading Module 2 (Audio)...
"
    .byte 0
__str_50:
    .text "Module 2: Audio buffers loaded
"
    .byte 0
__str_55:
    .text "
"
    .byte 0
__str_56:
    .text "Loading Module 3 (Mesh)...
"
    .byte 0
__str_58:
    .text "Module 3: Mesh data loaded
"
    .byte 0
__str_63:
    .text "Palette color at 42: %02X
"
    .byte 0
__str_64:
    .text "
"
    .byte 0
__str_65:
    .text "Analyzing Bank Assignments...
"
    .byte 0
__str_67:
    .text "Bank Assignment Tests:
"
    .byte 0
__str_68:
    .text "  palette size: 256 bytes
"
    .byte 0
__str_69:
    .text "  entity size: 192 bytes (64 * 3)
"
    .byte 0
__str_70:
    .text "  Combined (Bank 0): 448 bytes
"
    .byte 0
__str_71:
    .text "
"
    .byte 0
__str_72:
    .text "  audio_left size: 512 bytes
"
    .byte 0
__str_73:
    .text "  audio_right size: 512 bytes
"
    .byte 0
__str_74:
    .text "  Combined (Bank 1): 1024 bytes
"
    .byte 0
__str_75:
    .text "
"
    .byte 0
__str_76:
    .text "  synth_state: 4 bytes
"
    .byte 0
__str_77:
    .text "  vertices: 2048 bytes
"
    .byte 0
__str_78:
    .text "  normals: 1024 bytes
"
    .byte 0
__str_79:
    .text "  Combined (Bank 2): 3076 bytes
"
    .byte 0
__str_80:
    .text "
"
    .byte 0
__str_81:
    .text "Total banks needed: 3
"
    .byte 0
__str_82:
    .text "
"
    .byte 0
__str_83:
    .text "=== Phase 99.1 Tests Complete ===
"
    .byte 0

__zp_save_buf:
; [DEBUG] Phase 87 code reached, optimize=true

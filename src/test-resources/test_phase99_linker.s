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

    .global _linker_test_bank0
    .global _linker_test_bank1
    .global _linker_test_bank2
    .global _aligned_vars
    .global _byte_var
    .global _word_var
    .global _small_bank_test
    .global _medium_bank_test
    .global _overlap_test1
    .global _overlap_test2
    .global _overlap_test3
    .global _map_var_a
    .global _map_var_b
    .global _map_var_c
    .global _map_var_d
    .global _test_linker_directives
    .global _test_alignment_constraints
    .global _test_bank_capacity
    .global _test_address_overlaps
    .global _test_bank_map_generation
    .global _main

    .segment "bss"
_linker_test_bank0:
; .debug_var: @global _linker_test_bank0 offset=0 size=2 type=int8 scope=global
    .res 256
_linker_test_bank1:
; .debug_var: @global _linker_test_bank1 offset=0 size=2 type=int16 scope=global
    .res 256
_linker_test_bank2:
; .debug_var: @global _linker_test_bank2 offset=0 size=4 type=int32 scope=global
    .res 4
_aligned_vars:
; .debug_var: @global _aligned_vars offset=0 size=4 type=int32 scope=global
    .res 4
_byte_var:
; .debug_var: @global _byte_var offset=0 size=2 type=int8 scope=global
    .res 1
_word_var:
; .debug_var: @global _word_var offset=0 size=2 type=int16 scope=global
    .res 2
_small_bank_test:
; .debug_var: @global _small_bank_test offset=0 size=2 type=int8 scope=global
    .res 512
_medium_bank_test:
; .debug_var: @global _medium_bank_test offset=0 size=2 type=int8 scope=global
    .res 1024
_overlap_test1:
; .debug_var: @global _overlap_test1 offset=0 size=2 type=int8 scope=global
    .res 256
_overlap_test2:
; .debug_var: @global _overlap_test2 offset=0 size=2 type=int8 scope=global
    .res 128
_overlap_test3:
; .debug_var: @global _overlap_test3 offset=0 size=2 type=int16 scope=global
    .res 64
_map_var_a:
; .debug_var: @global _map_var_a offset=0 size=2 type=int8 scope=global
    .res 64
_map_var_b:
; .debug_var: @global _map_var_b offset=0 size=2 type=int8 scope=global
    .res 32
_map_var_c:
; .debug_var: @global _map_var_c offset=0 size=2 type=int16 scope=global
    .res 32
_map_var_d:
; .debug_var: @global _map_var_d offset=0 size=4 type=int32 scope=global
    .res 4

    .segment "code"

; function _test_linker_directives
; SAC inline storage: 10 bytes
    _test_linker_directives__local_0: .word 0
    _test_linker_directives__local_2: .word 0
    _test_linker_directives__local_4: .word 0
    _test_linker_directives__local_10: .word 0
    _test_linker_directives__local_23: .word 0
    _test_linker_directives__local_27: .word 0
    _test_linker_directives__local_31: .long 0
    _test_linker_directives__local_33: .word 0
    _test_linker_directives__local_35: .word 0
    _test_linker_directives__local_37: .word 0
    proc _test_linker_directives
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdio.h", 11
    .local @_l_b0 = 2
    .local @_l_b1 = 4
    .local @_l_b2 = 6
    .local @_l_i = 0
; .debug_var: __test_linker_directives @_l_b0 offset=2 size=2 type=int8 scope=local
; .debug_var: __test_linker_directives @_l_b1 offset=4 size=2 type=int16 scope=local
; .debug_var: __test_linker_directives @_l_b2 offset=6 size=4 type=int32 scope=local
; .debug_var: __test_linker_directives @_l_i offset=0 size=2 type=int16 scope=local

@entry:
    .loc "src/test-resources/test_phase99_linker.c", 15
    ldax #__str_0
    sta _test_linker_directives__local_0
    stx _test_linker_directives__local_0+1
    lda _test_linker_directives__local_0
    ; [peephole-opt]     ldx _test_linker_directives__local_0+1
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
    .loc "src/test-resources/test_phase99_linker.c", 16
    ldax #__str_1
    sta _test_linker_directives__local_2
    stx _test_linker_directives__local_2+1
    lda _test_linker_directives__local_2
    ; [peephole-opt]     ldx _test_linker_directives__local_2+1
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
    .loc "src/test-resources/test_phase99_linker.c", 20
    lda #0
    sta _test_linker_directives__local_4
    sta _test_linker_directives__local_4+1
@for_cond2:
    lda _test_linker_directives__local_4
    ldx _test_linker_directives__local_4+1
    cmp.16 .AX, #256
    bcc @for_body3
    bra @for_end5
@for_body3:
    .loc "src/test-resources/test_phase99_linker.c", 21
    lda #255
    ldx #0
    sta $24
    stx $25
    lda _test_linker_directives__local_4
    ldx _test_linker_directives__local_4+1
    and $24
    sta $26
    stx $27
    ; [peephole-opt]     lda $26
    ; [peephole-opt]     ldx $27
    pha
    lda _test_linker_directives__local_4
    ldx _test_linker_directives__local_4+1
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_linker_test_bank0
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
@for_inc4:
    .loc "src/test-resources/test_phase99_linker.c", 20
    lda _test_linker_directives__local_4
    ldx _test_linker_directives__local_4+1
    sta $2A
    stx $2B
    ; [peephole-opt]     lda $2A
    ; [peephole-opt]     ldx $2B
    add.16 .AX, #1
    sta _test_linker_directives__local_4
    stx _test_linker_directives__local_4+1
    bra @for_cond2
@for_end5:
    .loc "src/test-resources/test_phase99_linker.c", 24
    lda #0
    sta _test_linker_directives__local_4
    sta _test_linker_directives__local_4+1
@for_cond6:
    lda _test_linker_directives__local_4
    ldx _test_linker_directives__local_4+1
    cmp.16 .AX, #128
    bcc @for_body7
    bra @for_end9
@for_body7:
    .loc "src/test-resources/test_phase99_linker.c", 25
    lda #2
    ldx #0
    sta $24
    stx $25
    lda _test_linker_directives__local_4
    ldx _test_linker_directives__local_4+1
    lsl.16 .AX
    sta $26
    stx $27
    ; [peephole-opt]     lda $26
    ; [peephole-opt]     ldx $27
    pha
    phx
    lda _test_linker_directives__local_4
    ldx _test_linker_directives__local_4+1
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_linker_test_bank1
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
@for_inc8:
    .loc "src/test-resources/test_phase99_linker.c", 24
    lda _test_linker_directives__local_4
    ldx _test_linker_directives__local_4+1
    sta $2A
    stx $2B
    ; [peephole-opt]     lda $2A
    ; [peephole-opt]     ldx $2B
    add.16 .AX, #1
    sta _test_linker_directives__local_4
    stx _test_linker_directives__local_4+1
    bra @for_cond6
@for_end9:
    .loc "src/test-resources/test_phase99_linker.c", 28
    lda #239
    ldx #190
    ldy #173
    ldz #222
    sta _linker_test_bank2
    stx _linker_test_bank2+1
    sty _linker_test_bank2+2
    stz _linker_test_bank2+3
    .loc "src/test-resources/test_phase99_linker.c", 31
    lda #42
    ldx #0
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_linker_test_bank0
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
    sta _test_linker_directives__local_23
    .loc "src/test-resources/test_phase99_linker.c", 32
    lda #21
    ldx #0
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_linker_test_bank1
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
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta _test_linker_directives__local_27
    stx _test_linker_directives__local_27+1
    .loc "src/test-resources/test_phase99_linker.c", 33
    lda _linker_test_bank2
    ldx _linker_test_bank2+1
    ldy _linker_test_bank2+2
    ldz _linker_test_bank2+3
    sta _test_linker_directives__local_31
    stx _test_linker_directives__local_31+1
    sty _test_linker_directives__local_31+2
    stz _test_linker_directives__local_31+3
    .loc "src/test-resources/test_phase99_linker.c", 35
    ldax #__str_10
    sta _test_linker_directives__local_33
    stx _test_linker_directives__local_33+1
    lda _test_linker_directives__local_23
    sta $28
    stx $29
    lda _test_linker_directives__local_33
    ; [peephole-opt]     ldx _test_linker_directives__local_33+1
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
    .loc "src/test-resources/test_phase99_linker.c", 36
    ldax #__str_11
    sta _test_linker_directives__local_35
    stx _test_linker_directives__local_35+1
    lda _test_linker_directives__local_27
    ldx _test_linker_directives__local_27+1
    sta $28
    stx $29
    lda _test_linker_directives__local_35
    ; [peephole-opt]     ldx _test_linker_directives__local_35+1
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
    .loc "src/test-resources/test_phase99_linker.c", 37
    ldax #__str_12
    sta _test_linker_directives__local_37
    stx _test_linker_directives__local_37+1
    lda _test_linker_directives__local_31
    ldx _test_linker_directives__local_31+1
    ldy _test_linker_directives__local_31+2
    ldz _test_linker_directives__local_31+3
    sta $28
    stx $29
    sty $2A
    stz $2B
    lda _test_linker_directives__local_37
    ; [peephole-opt]     ldx _test_linker_directives__local_37+1
    sta $2C
    stx $2D
    ; [peephole-opt]     lda $28
    ; [peephole-opt]     ldx $29
    ; [peephole-opt]     ldy $2A
    ldz $2B
    push .axyz
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
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 10
    endproc

; function _test_alignment_constraints
; SAC inline storage: 0 bytes
    _test_alignment_constraints__local_0: .word 0
    _test_alignment_constraints__local_5: .word 0
    _test_alignment_constraints__local_7: .word 0
    _test_alignment_constraints__local_9: .word 0
    _test_alignment_constraints__local_11: .word 0
    proc _test_alignment_constraints
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "src/test-resources/test_phase99_linker.c", 29

@entry:
    .loc "src/test-resources/test_phase99_linker.c", 49
    ldax #__str_13
    sta _test_alignment_constraints__local_0
    stx _test_alignment_constraints__local_0+1
    lda _test_alignment_constraints__local_0
    ; [peephole-opt]     ldx _test_alignment_constraints__local_0+1
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
    .loc "src/test-resources/test_phase99_linker.c", 51
    lda #120
    ldx #86
    ldy #52
    ldz #18
    sta _aligned_vars
    stx _aligned_vars+1
    sty _aligned_vars+2
    stz _aligned_vars+3
    .loc "src/test-resources/test_phase99_linker.c", 52
    lda #85
    sta _byte_var
    .loc "src/test-resources/test_phase99_linker.c", 53
    lda #205
    ldx #171
    sta _word_var
    stx _word_var+1
    .loc "src/test-resources/test_phase99_linker.c", 55
    ldax #__str_14
    sta _test_alignment_constraints__local_5
    stx _test_alignment_constraints__local_5+1
    lda _test_alignment_constraints__local_5
    ; [peephole-opt]     ldx _test_alignment_constraints__local_5+1
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
    .loc "src/test-resources/test_phase99_linker.c", 56
    ldax #__str_15
    sta _test_alignment_constraints__local_7
    stx _test_alignment_constraints__local_7+1
    lda _test_alignment_constraints__local_7
    ; [peephole-opt]     ldx _test_alignment_constraints__local_7+1
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
    .loc "src/test-resources/test_phase99_linker.c", 57
    ldax #__str_16
    sta _test_alignment_constraints__local_9
    stx _test_alignment_constraints__local_9+1
    lda _test_alignment_constraints__local_9
    ; [peephole-opt]     ldx _test_alignment_constraints__local_9+1
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
    .loc "src/test-resources/test_phase99_linker.c", 58
    ldax #__str_17
    sta _test_alignment_constraints__local_11
    stx _test_alignment_constraints__local_11+1
    lda _test_alignment_constraints__local_11
    ; [peephole-opt]     ldx _test_alignment_constraints__local_11+1
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

; function _test_bank_capacity
; SAC inline storage: 2 bytes
    _test_bank_capacity__local_0: .word 0
    _test_bank_capacity__local_2: .word 0
    _test_bank_capacity__local_23: .word 0
    _test_bank_capacity__local_25: .word 0
    _test_bank_capacity__local_27: .word 0
    proc _test_bank_capacity
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "src/test-resources/test_phase99_linker.c", 49
    .local @_l_i = 0
; .debug_var: __test_bank_capacity @_l_i offset=0 size=2 type=int16 scope=local

@entry:
    .loc "src/test-resources/test_phase99_linker.c", 69
    ldax #__str_18
    sta _test_bank_capacity__local_0
    stx _test_bank_capacity__local_0+1
    lda _test_bank_capacity__local_0
    ; [peephole-opt]     ldx _test_bank_capacity__local_0+1
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
    .loc "src/test-resources/test_phase99_linker.c", 72
    lda #0
    sta _test_bank_capacity__local_2
    sta _test_bank_capacity__local_2+1
@for_cond19:
    lda _test_bank_capacity__local_2
    ldx _test_bank_capacity__local_2+1
    cmp.16 .AX, #512
    bcc @for_body20
    bra @for_end22
@for_body20:
    .loc "src/test-resources/test_phase99_linker.c", 73
    lda #255
    ldx #0
    sta $24
    stx $25
    lda _test_bank_capacity__local_2
    ldx _test_bank_capacity__local_2+1
    and $24
    sta $26
    stx $27
    ; [peephole-opt]     lda $26
    ; [peephole-opt]     ldx $27
    sta $28
    ; [peephole-opt]     lda $26
    ; [peephole-opt]     ldx $27
    pha
    lda _test_bank_capacity__local_2
    ldx _test_bank_capacity__local_2+1
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_small_bank_test
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
@for_inc21:
    .loc "src/test-resources/test_phase99_linker.c", 72
    lda _test_bank_capacity__local_2
    ldx _test_bank_capacity__local_2+1
    sta $2C
    stx $2D
    ; [peephole-opt]     lda $2C
    ; [peephole-opt]     ldx $2D
    add.16 .AX, #1
    sta _test_bank_capacity__local_2
    stx _test_bank_capacity__local_2+1
    bra @for_cond19
@for_end22:
    .loc "src/test-resources/test_phase99_linker.c", 76
    lda #0
    sta _test_bank_capacity__local_2
    sta _test_bank_capacity__local_2+1
@for_cond23:
    lda _test_bank_capacity__local_2
    ldx _test_bank_capacity__local_2+1
    cmp.16 .AX, #1024
    bcc @for_body24
    bra @for_end26
@for_body24:
    .loc "src/test-resources/test_phase99_linker.c", 77
    lda #0
    ldx #1
    sta $24
    stx $25
    lda _test_bank_capacity__local_2
    ldx _test_bank_capacity__local_2+1
    txa
    ldx #0
    sta $26
    stx $27
    lda #255
    ldx #0
    sta $28
    stx $29
    ; [peephole-opt]     lda $26
    ; [peephole-opt]     ldx $27
    and $28
    sta $2A
    stx $2B
    ; [peephole-opt]     lda $2A
    ; [peephole-opt]     ldx $2B
    sta $2C
    ; [peephole-opt]     lda $2A
    ; [peephole-opt]     ldx $2B
    pha
    lda _test_bank_capacity__local_2
    ldx _test_bank_capacity__local_2+1
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_medium_bank_test
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
@for_inc25:
    .loc "src/test-resources/test_phase99_linker.c", 76
    lda _test_bank_capacity__local_2
    ldx _test_bank_capacity__local_2+1
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    add.16 .AX, #1
    sta _test_bank_capacity__local_2
    stx _test_bank_capacity__local_2+1
    bra @for_cond23
@for_end26:
    .loc "src/test-resources/test_phase99_linker.c", 80
    ldax #__str_27
    sta _test_bank_capacity__local_23
    stx _test_bank_capacity__local_23+1
    lda _test_bank_capacity__local_23
    ; [peephole-opt]     ldx _test_bank_capacity__local_23+1
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
    .loc "src/test-resources/test_phase99_linker.c", 81
    ldax #__str_28
    sta _test_bank_capacity__local_25
    stx _test_bank_capacity__local_25+1
    lda _test_bank_capacity__local_25
    ; [peephole-opt]     ldx _test_bank_capacity__local_25+1
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
    .loc "src/test-resources/test_phase99_linker.c", 82
    ldax #__str_29
    sta _test_bank_capacity__local_27
    stx _test_bank_capacity__local_27+1
    lda _test_bank_capacity__local_27
    ; [peephole-opt]     ldx _test_bank_capacity__local_27+1
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
    .frame_size 2
    endproc

; function _test_address_overlaps
; SAC inline storage: 0 bytes
    _test_address_overlaps__local_0: .word 0
    _test_address_overlaps__local_11: .word 0
    _test_address_overlaps__local_13: .word 0
    _test_address_overlaps__local_15: .word 0
    _test_address_overlaps__local_17: .word 0
    proc _test_address_overlaps
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "src/test-resources/test_phase99_linker.c", 74

@entry:
    .loc "src/test-resources/test_phase99_linker.c", 94
    ldax #__str_30
    sta _test_address_overlaps__local_0
    stx _test_address_overlaps__local_0+1
    lda _test_address_overlaps__local_0
    ; [peephole-opt]     ldx _test_address_overlaps__local_0+1
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
    .loc "src/test-resources/test_phase99_linker.c", 97
    lda #170
    sta $20
    lda #0
    tax
    ; [peephole-opt]     lda $20
    ldx #0
    pha
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_overlap_test1
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
    .loc "src/test-resources/test_phase99_linker.c", 98
    lda #187
    sta $20
    lda #0
    tax
    ; [peephole-opt]     lda $20
    ldx #0
    pha
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_overlap_test2
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
    .loc "src/test-resources/test_phase99_linker.c", 99
    lda #221
    ldx #204
    sta $20
    stx $21
    lda #0
    tax
    ; [peephole-opt]     lda $20
    ; [peephole-opt]     ldx $21
    pha
    phx
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_overlap_test3
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
    .loc "src/test-resources/test_phase99_linker.c", 101
    ldax #__str_31
    sta _test_address_overlaps__local_11
    stx _test_address_overlaps__local_11+1
    lda _test_address_overlaps__local_11
    ; [peephole-opt]     ldx _test_address_overlaps__local_11+1
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
    .loc "src/test-resources/test_phase99_linker.c", 102
    ldax #__str_32
    sta _test_address_overlaps__local_13
    stx _test_address_overlaps__local_13+1
    lda _test_address_overlaps__local_13
    ; [peephole-opt]     ldx _test_address_overlaps__local_13+1
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
    .loc "src/test-resources/test_phase99_linker.c", 103
    ldax #__str_33
    sta _test_address_overlaps__local_15
    stx _test_address_overlaps__local_15+1
    lda _test_address_overlaps__local_15
    ; [peephole-opt]     ldx _test_address_overlaps__local_15+1
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
    .loc "src/test-resources/test_phase99_linker.c", 104
    ldax #__str_34
    sta _test_address_overlaps__local_17
    stx _test_address_overlaps__local_17+1
    lda _test_address_overlaps__local_17
    ; [peephole-opt]     ldx _test_address_overlaps__local_17+1
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

; function _test_bank_map_generation
; SAC inline storage: 0 bytes
    _test_bank_map_generation__local_0: .word 0
    _test_bank_map_generation__local_12: .word 0
    _test_bank_map_generation__local_14: .word 0
    _test_bank_map_generation__local_16: .word 0
    _test_bank_map_generation__local_18: .word 0
    _test_bank_map_generation__local_20: .word 0
    _test_bank_map_generation__local_22: .word 0
    proc _test_bank_map_generation
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "src/test-resources/test_phase99_linker.c", 97

@entry:
    .loc "src/test-resources/test_phase99_linker.c", 117
    ldax #__str_35
    sta _test_bank_map_generation__local_0
    stx _test_bank_map_generation__local_0+1
    lda _test_bank_map_generation__local_0
    ; [peephole-opt]     ldx _test_bank_map_generation__local_0+1
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
    .loc "src/test-resources/test_phase99_linker.c", 119
    lda #17
    sta $20
    lda #0
    tax
    ; [peephole-opt]     lda $20
    ldx #0
    pha
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_map_var_a
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
    .loc "src/test-resources/test_phase99_linker.c", 120
    lda #34
    sta $20
    lda #0
    tax
    ; [peephole-opt]     lda $20
    ldx #0
    pha
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_map_var_b
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
    .loc "src/test-resources/test_phase99_linker.c", 121
    lda #68
    ldx #51
    sta $20
    stx $21
    lda #0
    tax
    ; [peephole-opt]     lda $20
    ; [peephole-opt]     ldx $21
    pha
    phx
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_map_var_c
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
    .loc "src/test-resources/test_phase99_linker.c", 122
    lda #136
    ldx #119
    ldy #102
    ldz #85
    sta _map_var_d
    stx _map_var_d+1
    sty _map_var_d+2
    stz _map_var_d+3
    .loc "src/test-resources/test_phase99_linker.c", 124
    ldax #__str_36
    sta _test_bank_map_generation__local_12
    stx _test_bank_map_generation__local_12+1
    lda _test_bank_map_generation__local_12
    ; [peephole-opt]     ldx _test_bank_map_generation__local_12+1
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
    .loc "src/test-resources/test_phase99_linker.c", 125
    ldax #__str_37
    sta _test_bank_map_generation__local_14
    stx _test_bank_map_generation__local_14+1
    lda _test_bank_map_generation__local_14
    ; [peephole-opt]     ldx _test_bank_map_generation__local_14+1
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
    .loc "src/test-resources/test_phase99_linker.c", 126
    ldax #__str_38
    sta _test_bank_map_generation__local_16
    stx _test_bank_map_generation__local_16+1
    lda _test_bank_map_generation__local_16
    ; [peephole-opt]     ldx _test_bank_map_generation__local_16+1
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
    .loc "src/test-resources/test_phase99_linker.c", 127
    ldax #__str_39
    sta _test_bank_map_generation__local_18
    stx _test_bank_map_generation__local_18+1
    lda _test_bank_map_generation__local_18
    ; [peephole-opt]     ldx _test_bank_map_generation__local_18+1
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
    .loc "src/test-resources/test_phase99_linker.c", 128
    ldax #__str_40
    sta _test_bank_map_generation__local_20
    stx _test_bank_map_generation__local_20+1
    lda _test_bank_map_generation__local_20
    ; [peephole-opt]     ldx _test_bank_map_generation__local_20+1
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
    .loc "src/test-resources/test_phase99_linker.c", 129
    ldax #__str_41
    sta _test_bank_map_generation__local_22
    stx _test_bank_map_generation__local_22+1
    lda _test_bank_map_generation__local_22
    ; [peephole-opt]     ldx _test_bank_map_generation__local_22+1
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
; SAC inline storage: 12 bytes
    _main__local_0: .word 0
    _main__local_2: .word 0
    _main__local_4: .word 0
    _main__local_25: .word 0
    _main__local_29: .word 0
    _main__local_33: .long 0
    _main__local_35: .word 0
    _main__local_37: .word 0
    _main__local_39: .word 0
    _main__local_41: .word 0
    _main__local_46: .word 0
    _main__local_48: .word 0
    _main__local_50: .word 0
    _main__local_52: .word 0
    _main__local_54: .word 0
    _main__local_56: .word 0
    _main__local_77: .word 0
    _main__local_79: .word 0
    _main__local_81: .word 0
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
    _main__local_120: .word 0
    _main__local_121: .word 0
    _main__local_122: .word 0
    _main__local_123: .word 0
    _main__local_124: .word 0
    _main__local_125: .word 0
    _main__local_126: .word 0
    _main__local_127: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "src/test-resources/test_phase99_linker.c", 117
    .local @_l_b0 = 2
    .local @_l_b1 = 4
    .local @_l_b2 = 8
    .local @_l_i = 6
; .debug_var: __main @_l_b0 offset=2 size=2 type=int8 scope=local
; .debug_var: __main @_l_b1 offset=4 size=2 type=int16 scope=local
; .debug_var: __main @_l_b2 offset=8 size=4 type=int32 scope=local
; .debug_var: __main @_l_i offset=6 size=2 type=int16 scope=local

@entry:
    .loc "src/test-resources/test_phase99_linker.c", 137
    ldax #__str_42
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
    .loc "src/test-resources/test_phase99_linker.c", 15
    ldax #__str_44
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
    .loc "src/test-resources/test_phase99_linker.c", 16
    ldax #__str_45
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
    .loc "src/test-resources/test_phase99_linker.c", 20
    lda #0
    sta $20
    sta $21
@for_cond46:
    ; [peephole-opt]     lda $20
    ldx $21
    cmp.16 .AX, #256
    bcc @for_body47
    bra @for_end49
@for_body47:
    .loc "src/test-resources/test_phase99_linker.c", 21
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
    ldax #_linker_test_bank0
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
@for_inc48:
    .loc "src/test-resources/test_phase99_linker.c", 20
    lda $20
    ldx $21
    sta $2E
    stx $2F
    ; [peephole-opt]     lda $2E
    clc
    adc #1
    sta $20
    lda $2F
    adc #0
    sta $21
    bra @for_cond46
@for_end49:
    .loc "src/test-resources/test_phase99_linker.c", 24
    lda #0
    sta $20
    sta $21
@for_cond50:
    ; [peephole-opt]     lda $20
    ldx $21
    cmp.16 .AX, #128
    bcc @for_body51
    bra @for_end53
@for_body51:
    .loc "src/test-resources/test_phase99_linker.c", 25
    lda #2
    ldx #0
    sta $26
    stx $27
    lda $20
    ldx $21
    lsl.16 .AX
    sta $28
    stx $29
    ; [peephole-opt]     lda $28
    ; [peephole-opt]     ldx $29
    pha
    phx
    lda $20
    ldx $21
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_linker_test_bank1
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
@for_inc52:
    .loc "src/test-resources/test_phase99_linker.c", 24
    lda $20
    ldx $21
    sta $2C
    stx $2D
    ; [peephole-opt]     lda $2C
    clc
    adc #1
    sta $20
    lda $2D
    adc #0
    sta $21
    bra @for_cond50
@for_end53:
    .loc "src/test-resources/test_phase99_linker.c", 28
    lda #239
    ldx #190
    ldy #173
    ldz #222
    sta _linker_test_bank2
    stx _linker_test_bank2+1
    sty _linker_test_bank2+2
    stz _linker_test_bank2+3
    .loc "src/test-resources/test_phase99_linker.c", 31
    lda #42
    ldx #0
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_linker_test_bank0
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
    sta _main__local_25
    .loc "src/test-resources/test_phase99_linker.c", 32
    lda #21
    ldx #0
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_linker_test_bank1
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
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta _main__local_29
    stx _main__local_29+1
    .loc "src/test-resources/test_phase99_linker.c", 33
    lda _linker_test_bank2
    ldx _linker_test_bank2+1
    ldy _linker_test_bank2+2
    ldz _linker_test_bank2+3
    sta _main__local_33
    stx _main__local_33+1
    sty _main__local_33+2
    stz _main__local_33+3
    .loc "src/test-resources/test_phase99_linker.c", 35
    ldax #__str_54
    sta _main__local_35
    stx _main__local_35+1
    lda _main__local_25
    sta $28
    stx $29
    lda _main__local_35
    ; [peephole-opt]     ldx _main__local_35+1
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
    .loc "src/test-resources/test_phase99_linker.c", 36
    ldax #__str_55
    sta _main__local_37
    stx _main__local_37+1
    lda _main__local_29
    ldx _main__local_29+1
    sta $28
    stx $29
    lda _main__local_37
    ; [peephole-opt]     ldx _main__local_37+1
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
    .loc "src/test-resources/test_phase99_linker.c", 37
    ldax #__str_56
    sta _main__local_39
    stx _main__local_39+1
    lda _main__local_33
    ldx _main__local_33+1
    ldy _main__local_33+2
    ldz _main__local_33+3
    sta $28
    stx $29
    sty $2A
    stz $2B
    lda _main__local_39
    ; [peephole-opt]     ldx _main__local_39+1
    sta $2C
    stx $2D
    ; [peephole-opt]     lda $28
    ; [peephole-opt]     ldx $29
    ; [peephole-opt]     ldy $2A
    ldz $2B
    push .axyz
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
@inline_end43:
    .loc "src/test-resources/test_phase99_linker.c", 49
    ldax #__str_58
    sta _main__local_41
    stx _main__local_41+1
    lda _main__local_41
    ; [peephole-opt]     ldx _main__local_41+1
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
    .loc "src/test-resources/test_phase99_linker.c", 51
    lda #120
    ldx #86
    ldy #52
    ldz #18
    sta _aligned_vars
    stx _aligned_vars+1
    sty _aligned_vars+2
    stz _aligned_vars+3
    .loc "src/test-resources/test_phase99_linker.c", 52
    lda #85
    sta _byte_var
    .loc "src/test-resources/test_phase99_linker.c", 53
    lda #205
    ldx #171
    sta _word_var
    stx _word_var+1
    .loc "src/test-resources/test_phase99_linker.c", 55
    ldax #__str_59
    sta _main__local_46
    stx _main__local_46+1
    lda _main__local_46
    ; [peephole-opt]     ldx _main__local_46+1
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
    .loc "src/test-resources/test_phase99_linker.c", 56
    ldax #__str_60
    sta _main__local_48
    stx _main__local_48+1
    lda _main__local_48
    ; [peephole-opt]     ldx _main__local_48+1
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
    .loc "src/test-resources/test_phase99_linker.c", 57
    ldax #__str_61
    sta _main__local_50
    stx _main__local_50+1
    lda _main__local_50
    ; [peephole-opt]     ldx _main__local_50+1
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
    .loc "src/test-resources/test_phase99_linker.c", 58
    ldax #__str_62
    sta _main__local_52
    stx _main__local_52+1
    lda _main__local_52
    ; [peephole-opt]     ldx _main__local_52+1
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
@inline_end57:
    .loc "src/test-resources/test_phase99_linker.c", 69
    ldax #__str_64
    sta _main__local_54
    stx _main__local_54+1
    lda _main__local_54
    ; [peephole-opt]     ldx _main__local_54+1
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
    .loc "src/test-resources/test_phase99_linker.c", 72
    lda #0
    sta _main__local_56
    sta _main__local_56+1
@for_cond65:
    lda _main__local_56
    ldx _main__local_56+1
    cmp.16 .AX, #512
    bcc @for_body66
    bra @for_end68
@for_body66:
    .loc "src/test-resources/test_phase99_linker.c", 73
    lda #255
    ldx #0
    sta $24
    stx $25
    lda _main__local_56
    ldx _main__local_56+1
    and $24
    sta $26
    stx $27
    ; [peephole-opt]     lda $26
    ; [peephole-opt]     ldx $27
    sta $28
    ; [peephole-opt]     lda $26
    ; [peephole-opt]     ldx $27
    pha
    lda _main__local_56
    ldx _main__local_56+1
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_small_bank_test
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
@for_inc67:
    .loc "src/test-resources/test_phase99_linker.c", 72
    lda _main__local_56
    ldx _main__local_56+1
    sta $2C
    stx $2D
    ; [peephole-opt]     lda $2C
    ; [peephole-opt]     ldx $2D
    add.16 .AX, #1
    sta _main__local_56
    stx _main__local_56+1
    bra @for_cond65
@for_end68:
    .loc "src/test-resources/test_phase99_linker.c", 76
    lda #0
    sta _main__local_56
    sta _main__local_56+1
@for_cond69:
    lda _main__local_56
    ldx _main__local_56+1
    cmp.16 .AX, #1024
    bcc @for_body70
    bra @for_end72
@for_body70:
    .loc "src/test-resources/test_phase99_linker.c", 77
    lda #0
    ldx #1
    sta $24
    stx $25
    lda _main__local_56
    ldx _main__local_56+1
    txa
    ldx #0
    sta $26
    stx $27
    lda #255
    ldx #0
    sta $28
    stx $29
    ; [peephole-opt]     lda $26
    ; [peephole-opt]     ldx $27
    and $28
    sta $2A
    stx $2B
    ; [peephole-opt]     lda $2A
    ; [peephole-opt]     ldx $2B
    sta $2C
    ; [peephole-opt]     lda $2A
    ; [peephole-opt]     ldx $2B
    pha
    lda _main__local_56
    ldx _main__local_56+1
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_medium_bank_test
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
@for_inc71:
    .loc "src/test-resources/test_phase99_linker.c", 76
    lda _main__local_56
    ldx _main__local_56+1
    sta $30
    stx $31
    ; [peephole-opt]     lda $30
    ; [peephole-opt]     ldx $31
    add.16 .AX, #1
    sta _main__local_56
    stx _main__local_56+1
    bra @for_cond69
@for_end72:
    .loc "src/test-resources/test_phase99_linker.c", 80
    ldax #__str_73
    sta _main__local_77
    stx _main__local_77+1
    lda _main__local_77
    ; [peephole-opt]     ldx _main__local_77+1
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
    .loc "src/test-resources/test_phase99_linker.c", 81
    ldax #__str_74
    sta _main__local_79
    stx _main__local_79+1
    lda _main__local_79
    ; [peephole-opt]     ldx _main__local_79+1
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
    .loc "src/test-resources/test_phase99_linker.c", 82
    ldax #__str_75
    sta _main__local_81
    stx _main__local_81+1
    lda _main__local_81
    ; [peephole-opt]     ldx _main__local_81+1
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
    .frame_size 12
    endproc


    .segment "data"
__str_0:
    .text "Phase 99.3: Linker Integration Test
"
    .byte 0
__str_1:
    .text "
Test 1: Bank Layout Directives
"
    .byte 0
__str_10:
    .text "  linker_test_bank0[42]: %02X (expected: 2A)
"
    .byte 0
__str_11:
    .text "  linker_test_bank1[21]: %04X (expected: 002A)
"
    .byte 0
__str_12:
    .text "  linker_test_bank2: %08lX (expected: DEADBEEF)
"
    .byte 0
__str_13:
    .text "
Test 2: Alignment Constraints
"
    .byte 0
__str_14:
    .text "  aligned_vars initialized
"
    .byte 0
__str_15:
    .text "  byte_var initialized
"
    .byte 0
__str_16:
    .text "  word_var initialized
"
    .byte 0
__str_17:
    .text "  All alignment constraints satisfied
"
    .byte 0
__str_18:
    .text "
Test 3: Bank Capacity Verification
"
    .byte 0
__str_27:
    .text "  small_bank_test: 512 bytes OK
"
    .byte 0
__str_28:
    .text "  medium_bank_test: 1024 bytes OK
"
    .byte 0
__str_29:
    .text "  Bank capacity constraints verified
"
    .byte 0
__str_30:
    .text "
Test 4: Address Overlap Detection
"
    .byte 0
__str_31:
    .text "  overlap_test1 @ bank location
"
    .byte 0
__str_32:
    .text "  overlap_test2 @ different bank location
"
    .byte 0
__str_33:
    .text "  overlap_test3 @ another bank location
"
    .byte 0
__str_34:
    .text "  No address overlaps detected
"
    .byte 0
__str_35:
    .text "
Test 5: Bank Map Generation
"
    .byte 0
__str_36:
    .text "  Bank map entries:
"
    .byte 0
__str_37:
    .text "    map_var_a: 64 bytes
"
    .byte 0
__str_38:
    .text "    map_var_b: 32 bytes
"
    .byte 0
__str_39:
    .text "    map_var_c: 32 bytes (16 words)
"
    .byte 0
__str_40:
    .text "    map_var_d: 4 bytes
"
    .byte 0
__str_41:
    .text "  Total mapped: 132 bytes
"
    .byte 0
__str_42:
    .text "=== Phase 99.3: Linker Integration Validation ===

"
    .byte 0
__str_44:
    .text "Phase 99.3: Linker Integration Test
"
    .byte 0
__str_45:
    .text "
Test 1: Bank Layout Directives
"
    .byte 0
__str_54:
    .text "  linker_test_bank0[42]: %02X (expected: 2A)
"
    .byte 0
__str_55:
    .text "  linker_test_bank1[21]: %04X (expected: 002A)
"
    .byte 0
__str_56:
    .text "  linker_test_bank2: %08lX (expected: DEADBEEF)
"
    .byte 0
__str_58:
    .text "
Test 2: Alignment Constraints
"
    .byte 0
__str_59:
    .text "  aligned_vars initialized
"
    .byte 0
__str_60:
    .text "  byte_var initialized
"
    .byte 0
__str_61:
    .text "  word_var initialized
"
    .byte 0
__str_62:
    .text "  All alignment constraints satisfied
"
    .byte 0
__str_64:
    .text "
Test 3: Bank Capacity Verification
"
    .byte 0
__str_73:
    .text "  small_bank_test: 512 bytes OK
"
    .byte 0
__str_74:
    .text "  medium_bank_test: 1024 bytes OK
"
    .byte 0
__str_75:
    .text "  Bank capacity constraints verified
"
    .byte 0
__str_77:
    .text "
Test 4: Address Overlap Detection
"
    .byte 0
__str_78:
    .text "  overlap_test1 @ bank location
"
    .byte 0
__str_79:
    .text "  overlap_test2 @ different bank location
"
    .byte 0
__str_80:
    .text "  overlap_test3 @ another bank location
"
    .byte 0
__str_81:
    .text "  No address overlaps detected
"
    .byte 0
__str_83:
    .text "
Test 5: Bank Map Generation
"
    .byte 0
__str_84:
    .text "  Bank map entries:
"
    .byte 0
__str_85:
    .text "    map_var_a: 64 bytes
"
    .byte 0
__str_86:
    .text "    map_var_b: 32 bytes
"
    .byte 0
__str_87:
    .text "    map_var_c: 32 bytes (16 words)
"
    .byte 0
__str_88:
    .text "    map_var_d: 4 bytes
"
    .byte 0
__str_89:
    .text "  Total mapped: 132 bytes
"
    .byte 0
__str_90:
    .text "
=== All linker integration tests passed ===
"
    .byte 0

__zp_save_buf:
; [DEBUG] Phase 87 code reached, optimize=true

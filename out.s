* = $2000
    __sp_base = $0101
    jsr _main
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
_bytes:
    .byte 16
    .byte 32
    .byte 48
    .byte 64
_words:
    .word 100
    .word 200
    .word 300
_partial:
    .word 11
    .word 22
    .res 6
_zeros:
    .byte 0
    .res 2


; function _main
    proc _main
    .var _fp = 0
    .loc "src/test-resources/test_array_init.c", 18
; frame: 7 bytes (frame-allocated vRegs only)
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
    .local __vr0 = 0
    .local __vr8 = 3
    .local @_l_local_bytes = 0
    .local @_l_local_words = 3
; .debug_var: __main @_l_local_bytes offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_local_words offset=3 size=2 type=int16 scope=local

@entry:
    .loc "src/test-resources/test_array_init.c", 20
    lda $08
    ldx $08+1
    sta $20
    stx $21
    lda #170
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    pha
    struct_elem.16 __zp_scratch, $20, #0
    pla
    ldy #0
    sta (__zp_scratch),y
    lda #187
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    pha
    struct_elem.16 __zp_scratch, $20, #1
    pla
    ldy #0
    sta (__zp_scratch),y
    lda #204
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    pha
    struct_elem.16 __zp_scratch, $20, #2
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "src/test-resources/test_array_init.c", 21
    leax.fp 3
    sta $20
    stx $21
    lda #232
    ldx #3
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
    lda #208
    ldx #7
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
    .loc "src/test-resources/test_array_init.c", 24
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_bytes
    add.16 .AX, __zp_scratch3
    sta __zp_scratch
    stx __zp_scratch+1
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
    lda $22
    ldx $23
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
    .loc "src/test-resources/test_array_init.c", 25
    lda #3
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_bytes
    add.16 .AX, __zp_scratch3
    sta __zp_scratch
    stx __zp_scratch+1
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
    lda $22
    ldx $23
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
    .loc "src/test-resources/test_array_init.c", 28
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_words
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
    sta $24
    stx $25
    lda $24
    ldx $25
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
    .loc "src/test-resources/test_array_init.c", 29
    lda #2
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_words
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
    sta $24
    stx $25
    lda $24
    ldx $25
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
    .loc "src/test-resources/test_array_init.c", 32
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_partial
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
    sta $24
    stx $25
    lda $24
    ldx $25
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
    .loc "src/test-resources/test_array_init.c", 33
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_partial
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
    sta $24
    stx $25
    lda $24
    ldx $25
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
    .loc "src/test-resources/test_array_init.c", 36
    lda #2
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_partial
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
    sta $24
    stx $25
    lda $24
    ldx $25
    sta $20
    lda _result
    ldx _result+1
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
    .loc "src/test-resources/test_array_init.c", 37
    lda #4
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_partial
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
    sta $24
    stx $25
    lda $24
    ldx $25
    sta $20
    lda _result
    ldx _result+1
    sta $22
    stx $23
    lda #7
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
    .loc "src/test-resources/test_array_init.c", 40
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_zeros
    add.16 .AX, __zp_scratch3
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $24
    lda _result
    ldx _result+1
    sta $20
    stx $21
    lda #8
    ldx #0
    sta $22
    stx $23
    lda $24
    ldx #0
    pha
    lda $22
    ldx $23
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
    .loc "src/test-resources/test_array_init.c", 41
    lda #2
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_zeros
    add.16 .AX, __zp_scratch3
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $24
    lda _result
    ldx _result+1
    sta $20
    stx $21
    lda #9
    ldx #0
    sta $22
    stx $23
    lda $24
    ldx #0
    pha
    lda $22
    ldx $23
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
    .loc "src/test-resources/test_array_init.c", 44
    lda $08
    ldx $08+1
    sta $20
    stx $21
    lda #0
    sta $22
    sta $23
    lda $22
    ldx $23
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
    add.16 .AX, __zp_scratch3
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $26
    lda _result
    ldx _result+1
    sta $20
    stx $21
    lda #10
    ldx #0
    sta $22
    stx $23
    lda $26
    ldx #0
    pha
    lda $22
    ldx $23
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
    .loc "src/test-resources/test_array_init.c", 45
    lda $08
    ldx $08+1
    sta $20
    stx $21
    lda #2
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
    add.16 .AX, __zp_scratch3
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $26
    lda _result
    ldx _result+1
    sta $20
    stx $21
    lda #11
    ldx #0
    sta $22
    stx $23
    lda $26
    ldx #0
    pha
    lda $22
    ldx $23
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
    .loc "src/test-resources/test_array_init.c", 48
    leax.fp 3
    sta $20
    stx $21
    lda #0
    sta $22
    sta $23
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
    lda _result
    ldx _result+1
    sta $22
    stx $23
    lda #12
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
    .loc "src/test-resources/test_array_init.c", 49
    leax.fp 3
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
    lda _result
    ldx _result+1
    sta $22
    stx $23
    lda #13
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
    .loc "src/test-resources/test_array_init.c", 52
    lda #255
    sta $20
    lda _result
    ldx _result+1
    sta $22
    stx $23
    lda #14
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
    .func_flags stack_call, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z
    endproc


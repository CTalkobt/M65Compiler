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

    .global _result
    .global _bytes
    .global _words
    .global _partial
    .global _zeros
    .global _main

    .segment "data"
    .byte 0
_result:
; .debug_var: @global _result offset=0 size=2 type=ptr scope=global
    .word 16384
_bytes:
; .debug_var: @global _bytes offset=0 size=2 type=int8 scope=global
    .byte 16
    .byte 32
    .byte 48
    .byte 64
_words:
; .debug_var: @global _words offset=0 size=2 type=int16 scope=global
    .word 100
    .word 200
    .word 300
_partial:
; .debug_var: @global _partial offset=0 size=2 type=int16 scope=global
    .word 11
    .word 22
    .res 6
_zeros:
; .debug_var: @global _zeros offset=0 size=2 type=int8 scope=global
    .byte 0
    .res 2

    .segment "code"

; function _main
; SAC inline storage: 7 bytes
    _main__local_0: .word 0
    _main__local_8: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_array_init.c", 18
    .local @_l_local_bytes = 0
    .local @_l_local_words = 3
; .debug_var: __main @_l_local_bytes offset=0 size=2 type=int8 scope=local
; .debug_var: __main @_l_local_words offset=3 size=2 type=int16 scope=local

@entry:
    .loc "test_array_init.c", 20
    leax.local 0
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
    .loc "test_array_init.c", 21
    leax.local 3
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
    .loc "test_array_init.c", 24
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_bytes
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
    .loc "test_array_init.c", 25
    lda #3
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_bytes
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
    .loc "test_array_init.c", 28
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_words
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
    .loc "test_array_init.c", 29
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
    .loc "test_array_init.c", 32
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_partial
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
    .loc "test_array_init.c", 33
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
    .loc "test_array_init.c", 36
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
    .loc "test_array_init.c", 37
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
    .loc "test_array_init.c", 40
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_zeros
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
    .loc "test_array_init.c", 41
    lda #2
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_zeros
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
    .loc "test_array_init.c", 44
    leax.local 0
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
    .loc "test_array_init.c", 45
    leax.local 0
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
    .loc "test_array_init.c", 48
    leax.local 3
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
    .loc "test_array_init.c", 49
    leax.local 3
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
    .loc "test_array_init.c", 52
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
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z
    .frame_size 7
    endproc


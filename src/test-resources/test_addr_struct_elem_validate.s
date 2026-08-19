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

    .global _output
    .global _test_array
    .global _global_data
    .global _main

    .segment "data"
    .byte 0
_output:
; .debug_var: @global _output offset=0 size=2 type=ptr scope=global
    .word 16384
_test_array:
; .debug_var: @global _test_array offset=0 size=2 type=int8 scope=global
    .byte 17
    .byte 34
    .byte 51
    .byte 68
    .byte 85
    .byte 102
_global_data:
; .debug_var: @global _global_data offset=0 size=2 type=int8 scope=global
    .byte 170
    .byte 187
    .byte 204
    .byte 221
    .byte 238
    .byte 255
    .byte 18
    .byte 52
    .byte 86
    .byte 120
    .byte 154
    .byte 188
    .byte 222
    .byte 240
    .byte 17
    .byte 34

    .segment "code"

; function _main
; SAC inline storage: 6 bytes
    _main__local_0: .word 0
    _main__local_2: .word 0
    _main__local_4: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/string.h", 14
    .local @_l_errors = 2
    .local @_l_result = 0
    .local @_l_test_num = 4
; .debug_var: __main @_l_errors offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_result offset=0 size=2 type=ptr scope=local
; .debug_var: __main @_l_test_num offset=4 size=2 type=int16 scope=local

@entry:
    .loc "test_addr_struct_elem_validate.c", 18
    lda _output
    ldx _output+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_0
    stx _main__local_0+1
    .loc "test_addr_struct_elem_validate.c", 19
    lda #0
    sta _main__local_2
    sta _main__local_2+1
    .loc "test_addr_struct_elem_validate.c", 20
    lda #0
    sta _main__local_4
    sta _main__local_4+1
    .loc "test_addr_struct_elem_validate.c", 24
    lda _main__local_4
    ldx _main__local_4+1
    sta $20
    stx $21
    lda $20
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta _main__local_4
    stx _main__local_4+1
    .loc "test_addr_struct_elem_validate.c", 25
    lda #0
    sta $20
    sta $21
    lda _main__local_0
    ldx _main__local_0+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda __zp_scratch2
    ldx __zp_scratch2+1
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
    lda #51
    sta $20
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    cmp.16 .AX, $24
    bne @if_then0
    bra @if_end2
@if_then0:
    lda _main__local_4
    ldx _main__local_4+1
    sta $20
    .loc "test_addr_struct_elem_validate.c", 26
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #32
    ldx #0
    sta $24
    stx $25
    lda _main__local_4
    ldx _main__local_4+1
    add.16 .AX, $24
    sta $26
    stx $27
    lda $20
    ldx #0
    pha
    lda $26
    ldx $27
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
    .loc "test_addr_struct_elem_validate.c", 27
    lda _main__local_2
    ldx _main__local_2+1
    sta $20
    stx $21
    lda $20
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta _main__local_2
    stx _main__local_2+1
@if_end2:
    .loc "test_addr_struct_elem_validate.c", 32
    lda _main__local_4
    ldx _main__local_4+1
    sta $20
    stx $21
    lda $20
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta _main__local_4
    stx _main__local_4+1
    .loc "test_addr_struct_elem_validate.c", 33
    lda #1
    ldx #0
    sta $20
    stx $21
    lda _main__local_0
    ldx _main__local_0+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda __zp_scratch2
    ldx __zp_scratch2+1
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
    lda #34
    sta $20
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    cmp.16 .AX, $24
    bne @if_then3
    bra @if_end5
@if_then3:
    lda _main__local_4
    ldx _main__local_4+1
    sta $20
    .loc "test_addr_struct_elem_validate.c", 34
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #32
    ldx #0
    sta $24
    stx $25
    lda _main__local_4
    ldx _main__local_4+1
    add.16 .AX, $24
    sta $26
    stx $27
    lda $20
    ldx #0
    pha
    lda $26
    ldx $27
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
    .loc "test_addr_struct_elem_validate.c", 35
    lda _main__local_2
    ldx _main__local_2+1
    sta $20
    stx $21
    lda $20
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta _main__local_2
    stx _main__local_2+1
@if_end5:
    .loc "test_addr_struct_elem_validate.c", 40
    lda _main__local_4
    ldx _main__local_4+1
    sta $20
    stx $21
    lda $20
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta _main__local_4
    stx _main__local_4+1
    .loc "test_addr_struct_elem_validate.c", 41
    lda #2
    ldx #0
    sta $20
    stx $21
    lda _main__local_0
    ldx _main__local_0+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda __zp_scratch2
    ldx __zp_scratch2+1
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
    lda #68
    sta $20
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    cmp.16 .AX, $24
    bne @if_then6
    bra @if_end8
@if_then6:
    lda _main__local_4
    ldx _main__local_4+1
    sta $20
    .loc "test_addr_struct_elem_validate.c", 42
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #32
    ldx #0
    sta $24
    stx $25
    lda _main__local_4
    ldx _main__local_4+1
    add.16 .AX, $24
    sta $26
    stx $27
    lda $20
    ldx #0
    pha
    lda $26
    ldx $27
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
    .loc "test_addr_struct_elem_validate.c", 43
    lda _main__local_2
    ldx _main__local_2+1
    sta $20
    stx $21
    lda $20
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta _main__local_2
    stx _main__local_2+1
@if_end8:
    .loc "test_addr_struct_elem_validate.c", 48
    lda _main__local_4
    ldx _main__local_4+1
    sta $20
    stx $21
    lda $20
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta _main__local_4
    stx _main__local_4+1
    .loc "test_addr_struct_elem_validate.c", 49
    lda #3
    ldx #0
    sta $20
    stx $21
    lda _main__local_0
    ldx _main__local_0+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda __zp_scratch2
    ldx __zp_scratch2+1
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
    lda #17
    sta $20
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    cmp.16 .AX, $24
    bne @if_then9
    bra @if_end11
@if_then9:
    lda _main__local_4
    ldx _main__local_4+1
    sta $20
    .loc "test_addr_struct_elem_validate.c", 50
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #32
    ldx #0
    sta $24
    stx $25
    lda _main__local_4
    ldx _main__local_4+1
    add.16 .AX, $24
    sta $26
    stx $27
    lda $20
    ldx #0
    pha
    lda $26
    ldx $27
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
    .loc "test_addr_struct_elem_validate.c", 51
    lda _main__local_2
    ldx _main__local_2+1
    sta $20
    stx $21
    lda $20
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta _main__local_2
    stx _main__local_2+1
@if_end11:
    .loc "test_addr_struct_elem_validate.c", 56
    lda _main__local_4
    ldx _main__local_4+1
    sta $20
    stx $21
    lda $20
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta _main__local_4
    stx _main__local_4+1
    .loc "test_addr_struct_elem_validate.c", 57
    lda #4
    ldx #0
    sta $20
    stx $21
    lda _main__local_0
    ldx _main__local_0+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda __zp_scratch2
    ldx __zp_scratch2+1
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
    lda #85
    sta $20
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    cmp.16 .AX, $24
    bne @if_then12
    bra @if_end14
@if_then12:
    lda _main__local_4
    ldx _main__local_4+1
    sta $20
    .loc "test_addr_struct_elem_validate.c", 58
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #32
    ldx #0
    sta $24
    stx $25
    lda _main__local_4
    ldx _main__local_4+1
    add.16 .AX, $24
    sta $26
    stx $27
    lda $20
    ldx #0
    pha
    lda $26
    ldx $27
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
    .loc "test_addr_struct_elem_validate.c", 59
    lda _main__local_2
    ldx _main__local_2+1
    sta $20
    stx $21
    lda $20
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta _main__local_2
    stx _main__local_2+1
@if_end14:
    .loc "test_addr_struct_elem_validate.c", 64
    lda _main__local_4
    ldx _main__local_4+1
    sta $20
    stx $21
    lda $20
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta _main__local_4
    stx _main__local_4+1
    .loc "test_addr_struct_elem_validate.c", 65
    lda #5
    ldx #0
    sta $20
    stx $21
    lda _main__local_0
    ldx _main__local_0+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda __zp_scratch2
    ldx __zp_scratch2+1
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
    lda #51
    sta $20
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    cmp.16 .AX, $24
    bne @if_then15
    bra @if_end17
@if_then15:
    lda _main__local_4
    ldx _main__local_4+1
    sta $20
    .loc "test_addr_struct_elem_validate.c", 66
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #32
    ldx #0
    sta $24
    stx $25
    lda _main__local_4
    ldx _main__local_4+1
    add.16 .AX, $24
    sta $26
    stx $27
    lda $20
    ldx #0
    pha
    lda $26
    ldx $27
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
    .loc "test_addr_struct_elem_validate.c", 67
    lda _main__local_2
    ldx _main__local_2+1
    sta $20
    stx $21
    lda $20
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta _main__local_2
    stx _main__local_2+1
@if_end17:
    .loc "test_addr_struct_elem_validate.c", 72
    lda _main__local_4
    ldx _main__local_4+1
    sta $20
    stx $21
    lda $20
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta _main__local_4
    stx _main__local_4+1
    .loc "test_addr_struct_elem_validate.c", 73
    lda #6
    ldx #0
    sta $20
    stx $21
    lda _main__local_0
    ldx _main__local_0+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda __zp_scratch2
    ldx __zp_scratch2+1
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
    lda #68
    sta $20
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    cmp.16 .AX, $24
    bne @if_then18
    bra @if_end20
@if_then18:
    lda _main__local_4
    ldx _main__local_4+1
    sta $20
    .loc "test_addr_struct_elem_validate.c", 74
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #32
    ldx #0
    sta $24
    stx $25
    lda _main__local_4
    ldx _main__local_4+1
    add.16 .AX, $24
    sta $26
    stx $27
    lda $20
    ldx #0
    pha
    lda $26
    ldx $27
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
    .loc "test_addr_struct_elem_validate.c", 75
    lda _main__local_2
    ldx _main__local_2+1
    sta $20
    stx $21
    lda $20
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta _main__local_2
    stx _main__local_2+1
@if_end20:
    .loc "test_addr_struct_elem_validate.c", 80
    lda _main__local_4
    ldx _main__local_4+1
    sta $20
    stx $21
    lda $20
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta _main__local_4
    stx _main__local_4+1
    .loc "test_addr_struct_elem_validate.c", 81
    lda #7
    ldx #0
    sta $20
    stx $21
    lda _main__local_0
    ldx _main__local_0+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda __zp_scratch2
    ldx __zp_scratch2+1
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
    lda #34
    sta $20
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    cmp.16 .AX, $24
    bne @if_then21
    bra @if_end23
@if_then21:
    lda _main__local_4
    ldx _main__local_4+1
    sta $20
    .loc "test_addr_struct_elem_validate.c", 82
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #32
    ldx #0
    sta $24
    stx $25
    lda _main__local_4
    ldx _main__local_4+1
    add.16 .AX, $24
    sta $26
    stx $27
    lda $20
    ldx #0
    pha
    lda $26
    ldx $27
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
    .loc "test_addr_struct_elem_validate.c", 83
    lda _main__local_2
    ldx _main__local_2+1
    sta $20
    stx $21
    lda $20
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta _main__local_2
    stx _main__local_2+1
@if_end23:
    .loc "test_addr_struct_elem_validate.c", 88
    lda _main__local_4
    ldx _main__local_4+1
    sta $20
    stx $21
    lda $20
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta _main__local_4
    stx _main__local_4+1
    .loc "test_addr_struct_elem_validate.c", 89
    lda #8
    ldx #0
    sta $20
    stx $21
    lda _main__local_0
    ldx _main__local_0+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda __zp_scratch2
    ldx __zp_scratch2+1
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
    lda #204
    sta $20
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    cmp.16 .AX, $24
    bne @if_then24
    bra @if_end26
@if_then24:
    lda _main__local_4
    ldx _main__local_4+1
    sta $20
    .loc "test_addr_struct_elem_validate.c", 90
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #32
    ldx #0
    sta $24
    stx $25
    lda _main__local_4
    ldx _main__local_4+1
    add.16 .AX, $24
    sta $26
    stx $27
    lda $20
    ldx #0
    pha
    lda $26
    ldx $27
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
    .loc "test_addr_struct_elem_validate.c", 91
    lda _main__local_2
    ldx _main__local_2+1
    sta $20
    stx $21
    lda $20
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta _main__local_2
    stx _main__local_2+1
@if_end26:
    .loc "test_addr_struct_elem_validate.c", 96
    lda _main__local_4
    ldx _main__local_4+1
    sta $20
    stx $21
    lda $20
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta _main__local_4
    stx _main__local_4+1
    .loc "test_addr_struct_elem_validate.c", 97
    lda #9
    ldx #0
    sta $20
    stx $21
    lda _main__local_0
    ldx _main__local_0+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda __zp_scratch2
    ldx __zp_scratch2+1
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
    lda #17
    sta $20
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    cmp.16 .AX, $24
    bne @if_then27
    bra @if_end29
@if_then27:
    lda _main__local_4
    ldx _main__local_4+1
    sta $20
    .loc "test_addr_struct_elem_validate.c", 98
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #32
    ldx #0
    sta $24
    stx $25
    lda _main__local_4
    ldx _main__local_4+1
    add.16 .AX, $24
    sta $26
    stx $27
    lda $20
    ldx #0
    pha
    lda $26
    ldx $27
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
    .loc "test_addr_struct_elem_validate.c", 99
    lda _main__local_2
    ldx _main__local_2+1
    sta $20
    stx $21
    lda $20
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta _main__local_2
    stx _main__local_2+1
@if_end29:
    .loc "test_addr_struct_elem_validate.c", 104
    lda _main__local_4
    ldx _main__local_4+1
    sta $20
    stx $21
    lda $20
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta _main__local_4
    stx _main__local_4+1
    .loc "test_addr_struct_elem_validate.c", 105
    lda #10
    ldx #0
    sta $20
    stx $21
    lda _main__local_0
    ldx _main__local_0+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda __zp_scratch2
    ldx __zp_scratch2+1
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
    lda #51
    sta $20
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    cmp.16 .AX, $24
    bne @if_then30
    bra @if_end32
@if_then30:
    lda _main__local_4
    ldx _main__local_4+1
    sta $20
    .loc "test_addr_struct_elem_validate.c", 106
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #32
    ldx #0
    sta $24
    stx $25
    lda _main__local_4
    ldx _main__local_4+1
    add.16 .AX, $24
    sta $26
    stx $27
    lda $20
    ldx #0
    pha
    lda $26
    ldx $27
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
    .loc "test_addr_struct_elem_validate.c", 107
    lda _main__local_2
    ldx _main__local_2+1
    sta $20
    stx $21
    lda $20
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta _main__local_2
    stx _main__local_2+1
@if_end32:
    .loc "test_addr_struct_elem_validate.c", 112
    lda _main__local_4
    ldx _main__local_4+1
    sta $20
    stx $21
    lda $20
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta _main__local_4
    stx _main__local_4+1
    .loc "test_addr_struct_elem_validate.c", 113
    lda #11
    ldx #0
    sta $20
    stx $21
    lda _main__local_0
    ldx _main__local_0+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda __zp_scratch2
    ldx __zp_scratch2+1
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
    lda #238
    sta $20
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    cmp.16 .AX, $24
    bne @if_then33
    bra @if_end35
@if_then33:
    lda _main__local_4
    ldx _main__local_4+1
    sta $20
    .loc "test_addr_struct_elem_validate.c", 114
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #32
    ldx #0
    sta $24
    stx $25
    lda _main__local_4
    ldx _main__local_4+1
    add.16 .AX, $24
    sta $26
    stx $27
    lda $20
    ldx #0
    pha
    lda $26
    ldx $27
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
    .loc "test_addr_struct_elem_validate.c", 115
    lda _main__local_2
    ldx _main__local_2+1
    sta $20
    stx $21
    lda $20
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    lda $22
    ldx $23
    sta _main__local_2
    stx _main__local_2+1
@if_end35:
    lda _main__local_2
    ldx _main__local_2+1
    sta $20
    stx $21
    .loc "test_addr_struct_elem_validate.c", 119
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc


__zp_save_buf:

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
    .global _byte_array
    .global _struct_array
    .global _test_count
    .global _errors
    .global _check
    .global _main

    .segment "data"
    .byte 0
_output:
; .debug_var: @global _output offset=0 size=2 type=ptr scope=global
    .word 16384
_byte_array:
; .debug_var: @global _byte_array offset=0 size=2 type=int8 scope=global
    .byte 17
    .byte 34
    .byte 51
    .byte 68
    .byte 85
    .byte 102
    .byte 119
    .byte 136
_struct_array:
; .debug_var: @global _struct_array offset=0 size=2 type=int8 scope=global
    .byte 170
    .byte 187
    .byte 204
    .byte 221
    .byte 238
    .byte 255
_test_count:
; .debug_var: @global _test_count offset=0 size=2 type=int16 scope=global
    .word 0
_errors:
; .debug_var: @global _errors offset=0 size=2 type=int16 scope=global
    .word 0

    .segment "code"

; function _check
; SAC inline storage: 6 bytes
    .global _check__param_actual
    _check__param_actual: .word 0
    .global _check__param_expected
    _check__param_expected: .word 0
    .global _check__param_test_name
    _check__param_test_name: .word 0
    _check__local_0: .word 0
    _check__local_1: .word 0
    _check__local_2: .word 0
    proc _check, W#@_p_actual, W#@_p_expected, W#@_p_test_name
    .sac
    .var _fp = 0
    .loc "test_simop_validation.c", 17
    .var @_p_actual = 2
    .var @_p_expected = 4
    .var @_p_test_name = 6
; .debug_var: __check @_p_actual offset=2 size=2 type=int16 scope=parameter
; .debug_var: __check @_p_expected offset=4 size=2 type=int16 scope=parameter
; .debug_var: __check @_p_test_name offset=6 size=2 type=ptr scope=parameter

@entry:
    .loc "test_simop_validation.c", 18
    lda _test_count
    ldx _test_count+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda _output
    ldx _output+1
    sta $20
    stx $21
    lda #32
    ldx #0
    sta $24
    stx $25
    lda _test_count
    ldx _test_count+1
    sta $26
    stx $27
    lda $24
    clc
    adc $26
    sta $28
    lda $25
    adc $26+1
    sta $29
    lda $22
    ldx #0
    pha
    lda $28
    ldx $29
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
    lda _check__param_expected
    ldx _check__param_expected+1
    sta $20
    .loc "test_simop_validation.c", 19
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #48
    ldx #0
    sta $24
    stx $25
    lda _test_count
    ldx _test_count+1
    sta $26
    stx $27
    lda $24
    clc
    adc $26
    sta $28
    lda $25
    adc $26+1
    sta $29
    lda $20
    ldx #0
    pha
    lda $28
    ldx $29
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
    lda _check__param_actual
    ldx _check__param_actual+1
    sta $20
    .loc "test_simop_validation.c", 20
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #64
    ldx #0
    sta $24
    stx $25
    lda _test_count
    ldx _test_count+1
    sta $26
    stx $27
    lda $24
    clc
    adc $26
    sta $28
    lda $25
    adc $26+1
    sta $29
    lda $20
    ldx #0
    pha
    lda $28
    ldx $29
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
    .loc "test_simop_validation.c", 22
    lda _check__param_expected
    ldx _check__param_expected+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _check__param_actual
    ldx _check__param_actual+1
    cmp.16 .AX, __zp_scratch2
    bne @if_then0
    bra @if_else1
@if_then0:
    .loc "test_simop_validation.c", 23
    lda _errors
    ldx _errors+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    clc
    adc #1
    sta $20
    lda $23
    adc #0
    sta $21
    lda $20
    ldx $21
    sta _errors
    stx _errors+1
    .loc "test_simop_validation.c", 24
    lda #255
    sta $20
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #80
    ldx #0
    sta $24
    stx $25
    lda _test_count
    ldx _test_count+1
    sta $26
    stx $27
    lda $24
    clc
    adc $26
    sta $28
    lda $25
    adc $26+1
    sta $29
    lda $20
    ldx #0
    pha
    lda $28
    ldx $29
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
    bra @if_end2
@if_else1:
    .loc "test_simop_validation.c", 26
    lda #0
    sta $20
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #80
    ldx #0
    sta $24
    stx $25
    lda _test_count
    ldx _test_count+1
    sta $26
    stx $27
    lda $24
    clc
    adc $26
    sta $28
    lda $25
    adc $26+1
    sta $29
    lda $20
    ldx #0
    pha
    lda $28
    ldx $29
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
@if_end2:
    .loc "test_simop_validation.c", 29
    lda _test_count
    ldx _test_count+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    clc
    adc #1
    sta $20
    lda $23
    adc #0
    sta $21
    lda $20
    ldx $21
    sta _test_count
    stx _test_count+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _main
; SAC inline storage: 2 bytes
    _main__local_0: .word 0
    _main__local_481: .word 0
    _main__local_482: .word 0
    _main__local_483: .word 0
    _main__local_484: .word 0
    _main__local_485: .word 0
    _main__local_486: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_simop_validation.c", 32
    .local @_l_i = 0
; .debug_var: __main @_l_i offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_simop_validation.c", 35
    lda #0
    sta _main__local_0
    sta _main__local_0+1
@for_cond3:
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, #96
    bcc @for_body4
    bra @for_end6
@for_body4:
    .loc "test_simop_validation.c", 36
    lda #0
    sta $24
    lda _output
    ldx _output+1
    sta $26
    stx $27
    lda $24
    ldx #0
    pha
    lda _main__local_0
    ldx _main__local_0+1
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
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
@for_inc5:
    .loc "test_simop_validation.c", 35
    lda _main__local_0
    ldx _main__local_0+1
    sta $2A
    stx $2B
    lda $2A
    clc
    adc #1
    sta $2C
    lda $2B
    adc #0
    sta $2D
    lda $2C
    ldx $2D
    sta _main__local_0
    stx _main__local_0+1
    bra @for_cond3
@for_end6:
    .loc "test_simop_validation.c", 39
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta _test_count
    stx _test_count+1
    .loc "test_simop_validation.c", 40
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta _errors
    stx _errors+1
    .loc "test_simop_validation.c", 43
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_byte_array
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
    ldx #0
    sta $20
    stx $21
    lda $24
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $26
    ldx $27
    sta $24
    stx $25
    lda $20
    ldx $21
    sta $24
    stx $25
    .loc "test_simop_validation.c", 18
    lda _test_count
    ldx _test_count+1
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #32
    ldx #0
    sta $28
    stx $29
    lda _test_count
    ldx _test_count+1
    sta $2A
    stx $2B
    lda $28
    clc
    adc $2A
    sta $2C
    lda $29
    adc $2A+1
    sta $2D
    lda $24
    ldx #0
    pha
    lda $2C
    ldx $2D
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
    lda $20
    ldx $21
    sta $22
    .loc "test_simop_validation.c", 19
    lda _output
    ldx _output+1
    sta $24
    stx $25
    lda #48
    ldx #0
    sta $28
    stx $29
    lda _test_count
    ldx _test_count+1
    sta $2A
    stx $2B
    lda $28
    clc
    adc $2A
    sta $2C
    lda $29
    adc $2A+1
    sta $2D
    lda $22
    ldx #0
    pha
    lda $2C
    ldx $2D
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $24
    ldx $24+1
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
    lda $26
    ldx $27
    sta $22
    .loc "test_simop_validation.c", 20
    lda _output
    ldx _output+1
    sta $24
    stx $25
    lda #64
    ldx #0
    sta $28
    stx $29
    lda _test_count
    ldx _test_count+1
    sta $2A
    stx $2B
    lda $28
    clc
    adc $2A
    sta $2C
    lda $29
    adc $2A+1
    sta $2D
    lda $22
    ldx #0
    pha
    lda $2C
    ldx $2D
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $24
    ldx $24+1
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
    .loc "test_simop_validation.c", 22
    lda $26
    ldx $27
    cmp.16 .AX, #17
    bne @if_then9
    bra @if_else10
@if_then9:
    .loc "test_simop_validation.c", 23
    lda _errors
    ldx _errors+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    clc
    adc #1
    sta $20
    lda $23
    adc #0
    sta $21
    lda $20
    ldx $21
    sta _errors
    stx _errors+1
    .loc "test_simop_validation.c", 24
    lda #255
    sta $20
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #80
    ldx #0
    sta $24
    stx $25
    lda _test_count
    ldx _test_count+1
    sta $26
    stx $27
    lda $24
    clc
    adc $26
    sta $28
    lda $25
    adc $26+1
    sta $29
    lda $20
    ldx #0
    pha
    lda $28
    ldx $29
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
    bra @if_end11
@if_else10:
    .loc "test_simop_validation.c", 26
    lda #0
    sta $20
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #80
    ldx #0
    sta $24
    stx $25
    lda _test_count
    ldx _test_count+1
    sta $26
    stx $27
    lda $24
    clc
    adc $26
    sta $28
    lda $25
    adc $26+1
    sta $29
    lda $20
    ldx #0
    pha
    lda $28
    ldx $29
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
@if_end11:
    .loc "test_simop_validation.c", 29
    lda _test_count
    ldx _test_count+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    clc
    adc #1
    sta $20
    lda $23
    adc #0
    sta $21
    lda $20
    ldx $21
    sta _test_count
    stx _test_count+1
@inline_end8:
    .loc "test_simop_validation.c", 44
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_byte_array
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
    ldx #0
    sta $20
    stx $21
    lda $24
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $26
    ldx $27
    sta $24
    stx $25
    lda $20
    ldx $21
    sta $24
    stx $25
    .loc "test_simop_validation.c", 18
    lda _test_count
    ldx _test_count+1
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #32
    ldx #0
    sta $28
    stx $29
    lda _test_count
    ldx _test_count+1
    sta $2A
    stx $2B
    lda $28
    clc
    adc $2A
    sta $2C
    lda $29
    adc $2A+1
    sta $2D
    lda $24
    ldx #0
    pha
    lda $2C
    ldx $2D
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
    lda $20
    ldx $21
    sta $22
    .loc "test_simop_validation.c", 19
    lda _output
    ldx _output+1
    sta $24
    stx $25
    lda #48
    ldx #0
    sta $28
    stx $29
    lda _test_count
    ldx _test_count+1
    sta $2A
    stx $2B
    lda $28
    clc
    adc $2A
    sta $2C
    lda $29
    adc $2A+1
    sta $2D
    lda $22
    ldx #0
    pha
    lda $2C
    ldx $2D
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $24
    ldx $24+1
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
    lda $26
    ldx $27
    sta $22
    .loc "test_simop_validation.c", 20
    lda _output
    ldx _output+1
    sta $24
    stx $25
    lda #64
    ldx #0
    sta $28
    stx $29
    lda _test_count
    ldx _test_count+1
    sta $2A
    stx $2B
    lda $28
    clc
    adc $2A
    sta $2C
    lda $29
    adc $2A+1
    sta $2D
    lda $22
    ldx #0
    pha
    lda $2C
    ldx $2D
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $24
    ldx $24+1
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
    .loc "test_simop_validation.c", 22
    lda $26
    ldx $27
    cmp.16 .AX, #34
    bne @if_then14
    bra @if_else15
@if_then14:
    .loc "test_simop_validation.c", 23
    lda _errors
    ldx _errors+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    clc
    adc #1
    sta $20
    lda $23
    adc #0
    sta $21
    lda $20
    ldx $21
    sta _errors
    stx _errors+1
    .loc "test_simop_validation.c", 24
    lda #255
    sta $20
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #80
    ldx #0
    sta $24
    stx $25
    lda _test_count
    ldx _test_count+1
    sta $26
    stx $27
    lda $24
    clc
    adc $26
    sta $28
    lda $25
    adc $26+1
    sta $29
    lda $20
    ldx #0
    pha
    lda $28
    ldx $29
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
    bra @if_end16
@if_else15:
    .loc "test_simop_validation.c", 26
    lda #0
    sta $20
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #80
    ldx #0
    sta $24
    stx $25
    lda _test_count
    ldx _test_count+1
    sta $26
    stx $27
    lda $24
    clc
    adc $26
    sta $28
    lda $25
    adc $26+1
    sta $29
    lda $20
    ldx #0
    pha
    lda $28
    ldx $29
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
@if_end16:
    .loc "test_simop_validation.c", 29
    lda _test_count
    ldx _test_count+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    clc
    adc #1
    sta $20
    lda $23
    adc #0
    sta $21
    lda $20
    ldx $21
    sta _test_count
    stx _test_count+1
@inline_end13:
    .loc "test_simop_validation.c", 45
    lda #2
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_byte_array
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
    ldx #0
    sta $20
    stx $21
    lda $24
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $26
    ldx $27
    sta $24
    stx $25
    lda $20
    ldx $21
    sta $24
    stx $25
    .loc "test_simop_validation.c", 18
    lda _test_count
    ldx _test_count+1
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #32
    ldx #0
    sta $28
    stx $29
    lda _test_count
    ldx _test_count+1
    sta $2A
    stx $2B
    lda $28
    clc
    adc $2A
    sta $2C
    lda $29
    adc $2A+1
    sta $2D
    lda $24
    ldx #0
    pha
    lda $2C
    ldx $2D
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
    lda $20
    ldx $21
    sta $22
    .loc "test_simop_validation.c", 19
    lda _output
    ldx _output+1
    sta $24
    stx $25
    lda #48
    ldx #0
    sta $28
    stx $29
    lda _test_count
    ldx _test_count+1
    sta $2A
    stx $2B
    lda $28
    clc
    adc $2A
    sta $2C
    lda $29
    adc $2A+1
    sta $2D
    lda $22
    ldx #0
    pha
    lda $2C
    ldx $2D
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $24
    ldx $24+1
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
    lda $26
    ldx $27
    sta $22
    .loc "test_simop_validation.c", 20
    lda _output
    ldx _output+1
    sta $24
    stx $25
    lda #64
    ldx #0
    sta $28
    stx $29
    lda _test_count
    ldx _test_count+1
    sta $2A
    stx $2B
    lda $28
    clc
    adc $2A
    sta $2C
    lda $29
    adc $2A+1
    sta $2D
    lda $22
    ldx #0
    pha
    lda $2C
    ldx $2D
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $24
    ldx $24+1
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
    .loc "test_simop_validation.c", 22
    lda $26
    ldx $27
    cmp.16 .AX, #51
    bne @if_then19
    bra @if_else20
@if_then19:
    .loc "test_simop_validation.c", 23
    lda _errors
    ldx _errors+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    clc
    adc #1
    sta $20
    lda $23
    adc #0
    sta $21
    lda $20
    ldx $21
    sta _errors
    stx _errors+1
    .loc "test_simop_validation.c", 24
    lda #255
    sta $20
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #80
    ldx #0
    sta $24
    stx $25
    lda _test_count
    ldx _test_count+1
    sta $26
    stx $27
    lda $24
    clc
    adc $26
    sta $28
    lda $25
    adc $26+1
    sta $29
    lda $20
    ldx #0
    pha
    lda $28
    ldx $29
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
    bra @if_end21
@if_else20:
    .loc "test_simop_validation.c", 26
    lda #0
    sta $20
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #80
    ldx #0
    sta $24
    stx $25
    lda _test_count
    ldx _test_count+1
    sta $26
    stx $27
    lda $24
    clc
    adc $26
    sta $28
    lda $25
    adc $26+1
    sta $29
    lda $20
    ldx #0
    pha
    lda $28
    ldx $29
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
@if_end21:
    .loc "test_simop_validation.c", 29
    lda _test_count
    ldx _test_count+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    clc
    adc #1
    sta $20
    lda $23
    adc #0
    sta $21
    lda $20
    ldx $21
    sta _test_count
    stx _test_count+1
@inline_end18:
    .loc "test_simop_validation.c", 46
    lda #3
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_byte_array
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
    ldx #0
    sta $20
    stx $21
    lda $24
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $26
    ldx $27
    sta $24
    stx $25
    lda $20
    ldx $21
    sta $24
    stx $25
    .loc "test_simop_validation.c", 18
    lda _test_count
    ldx _test_count+1
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #32
    ldx #0
    sta $28
    stx $29
    lda _test_count
    ldx _test_count+1
    sta $2A
    stx $2B
    lda $28
    clc
    adc $2A
    sta $2C
    lda $29
    adc $2A+1
    sta $2D
    lda $24
    ldx #0
    pha
    lda $2C
    ldx $2D
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
    lda $20
    ldx $21
    sta $22
    .loc "test_simop_validation.c", 19
    lda _output
    ldx _output+1
    sta $24
    stx $25
    lda #48
    ldx #0
    sta $28
    stx $29
    lda _test_count
    ldx _test_count+1
    sta $2A
    stx $2B
    lda $28
    clc
    adc $2A
    sta $2C
    lda $29
    adc $2A+1
    sta $2D
    lda $22
    ldx #0
    pha
    lda $2C
    ldx $2D
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $24
    ldx $24+1
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
    lda $26
    ldx $27
    sta $22
    .loc "test_simop_validation.c", 20
    lda _output
    ldx _output+1
    sta $24
    stx $25
    lda #64
    ldx #0
    sta $28
    stx $29
    lda _test_count
    ldx _test_count+1
    sta $2A
    stx $2B
    lda $28
    clc
    adc $2A
    sta $2C
    lda $29
    adc $2A+1
    sta $2D
    lda $22
    ldx #0
    pha
    lda $2C
    ldx $2D
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $24
    ldx $24+1
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
    .loc "test_simop_validation.c", 22
    lda $26
    ldx $27
    cmp.16 .AX, #68
    bne @if_then24
    bra @if_else25
@if_then24:
    .loc "test_simop_validation.c", 23
    lda _errors
    ldx _errors+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    clc
    adc #1
    sta $20
    lda $23
    adc #0
    sta $21
    lda $20
    ldx $21
    sta _errors
    stx _errors+1
    .loc "test_simop_validation.c", 24
    lda #255
    sta $20
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #80
    ldx #0
    sta $24
    stx $25
    lda _test_count
    ldx _test_count+1
    sta $26
    stx $27
    lda $24
    clc
    adc $26
    sta $28
    lda $25
    adc $26+1
    sta $29
    lda $20
    ldx #0
    pha
    lda $28
    ldx $29
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
    bra @if_end26
@if_else25:
    .loc "test_simop_validation.c", 26
    lda #0
    sta $20
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #80
    ldx #0
    sta $24
    stx $25
    lda _test_count
    ldx _test_count+1
    sta $26
    stx $27
    lda $24
    clc
    adc $26
    sta $28
    lda $25
    adc $26+1
    sta $29
    lda $20
    ldx #0
    pha
    lda $28
    ldx $29
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
@if_end26:
    .loc "test_simop_validation.c", 29
    lda _test_count
    ldx _test_count+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    clc
    adc #1
    sta $20
    lda $23
    adc #0
    sta $21
    lda $20
    ldx $21
    sta _test_count
    stx _test_count+1
@inline_end23:
    .loc "test_simop_validation.c", 47
    lda #4
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_byte_array
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
    ldx #0
    sta $20
    stx $21
    lda $24
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $26
    ldx $27
    sta $24
    stx $25
    lda $20
    ldx $21
    sta $24
    stx $25
    .loc "test_simop_validation.c", 18
    lda _test_count
    ldx _test_count+1
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #32
    ldx #0
    sta $28
    stx $29
    lda _test_count
    ldx _test_count+1
    sta $2A
    stx $2B
    lda $28
    clc
    adc $2A
    sta $2C
    lda $29
    adc $2A+1
    sta $2D
    lda $24
    ldx #0
    pha
    lda $2C
    ldx $2D
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
    lda $20
    ldx $21
    sta $22
    .loc "test_simop_validation.c", 19
    lda _output
    ldx _output+1
    sta $24
    stx $25
    lda #48
    ldx #0
    sta $28
    stx $29
    lda _test_count
    ldx _test_count+1
    sta $2A
    stx $2B
    lda $28
    clc
    adc $2A
    sta $2C
    lda $29
    adc $2A+1
    sta $2D
    lda $22
    ldx #0
    pha
    lda $2C
    ldx $2D
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $24
    ldx $24+1
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
    lda $26
    ldx $27
    sta $22
    .loc "test_simop_validation.c", 20
    lda _output
    ldx _output+1
    sta $24
    stx $25
    lda #64
    ldx #0
    sta $28
    stx $29
    lda _test_count
    ldx _test_count+1
    sta $2A
    stx $2B
    lda $28
    clc
    adc $2A
    sta $2C
    lda $29
    adc $2A+1
    sta $2D
    lda $22
    ldx #0
    pha
    lda $2C
    ldx $2D
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $24
    ldx $24+1
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
    .loc "test_simop_validation.c", 22
    lda $26
    ldx $27
    cmp.16 .AX, #85
    bne @if_then29
    bra @if_else30
@if_then29:
    .loc "test_simop_validation.c", 23
    lda _errors
    ldx _errors+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    clc
    adc #1
    sta $20
    lda $23
    adc #0
    sta $21
    lda $20
    ldx $21
    sta _errors
    stx _errors+1
    .loc "test_simop_validation.c", 24
    lda #255
    sta $20
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #80
    ldx #0
    sta $24
    stx $25
    lda _test_count
    ldx _test_count+1
    sta $26
    stx $27
    lda $24
    clc
    adc $26
    sta $28
    lda $25
    adc $26+1
    sta $29
    lda $20
    ldx #0
    pha
    lda $28
    ldx $29
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
    bra @if_end31
@if_else30:
    .loc "test_simop_validation.c", 26
    lda #0
    sta $20
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #80
    ldx #0
    sta $24
    stx $25
    lda _test_count
    ldx _test_count+1
    sta $26
    stx $27
    lda $24
    clc
    adc $26
    sta $28
    lda $25
    adc $26+1
    sta $29
    lda $20
    ldx #0
    pha
    lda $28
    ldx $29
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
@if_end31:
    .loc "test_simop_validation.c", 29
    lda _test_count
    ldx _test_count+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    clc
    adc #1
    sta $20
    lda $23
    adc #0
    sta $21
    lda $20
    ldx $21
    sta _test_count
    stx _test_count+1
@inline_end28:
    .loc "test_simop_validation.c", 48
    lda #5
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_byte_array
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
    lda #102
    ldx #0
    sta $20
    stx $21
    lda $24
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $26
    ldx $27
    sta $24
    stx $25
    lda $20
    ldx $21
    sta $24
    stx $25
    .loc "test_simop_validation.c", 18
    lda _test_count
    ldx _test_count+1
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #32
    ldx #0
    sta $28
    stx $29
    lda _test_count
    ldx _test_count+1
    sta $2A
    stx $2B
    lda $28
    clc
    adc $2A
    sta $2C
    lda $29
    adc $2A+1
    sta $2D
    lda $24
    ldx #0
    pha
    lda $2C
    ldx $2D
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
    lda $20
    ldx $21
    sta $22
    .loc "test_simop_validation.c", 19
    lda _output
    ldx _output+1
    sta $24
    stx $25
    lda #48
    ldx #0
    sta $28
    stx $29
    lda _test_count
    ldx _test_count+1
    sta $2A
    stx $2B
    lda $28
    clc
    adc $2A
    sta $2C
    lda $29
    adc $2A+1
    sta $2D
    lda $22
    ldx #0
    pha
    lda $2C
    ldx $2D
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $24
    ldx $24+1
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
    lda $26
    ldx $27
    sta $22
    .loc "test_simop_validation.c", 20
    lda _output
    ldx _output+1
    sta $24
    stx $25
    lda #64
    ldx #0
    sta $28
    stx $29
    lda _test_count
    ldx _test_count+1
    sta $2A
    stx $2B
    lda $28
    clc
    adc $2A
    sta $2C
    lda $29
    adc $2A+1
    sta $2D
    lda $22
    ldx #0
    pha
    lda $2C
    ldx $2D
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $24
    ldx $24+1
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
    .loc "test_simop_validation.c", 22
    lda $26
    ldx $27
    cmp.16 .AX, #102
    bne @if_then34
    bra @if_else35
@if_then34:
    .loc "test_simop_validation.c", 23
    lda _errors
    ldx _errors+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    clc
    adc #1
    sta $20
    lda $23
    adc #0
    sta $21
    lda $20
    ldx $21
    sta _errors
    stx _errors+1
    .loc "test_simop_validation.c", 24
    lda #255
    sta $20
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #80
    ldx #0
    sta $24
    stx $25
    lda _test_count
    ldx _test_count+1
    sta $26
    stx $27
    lda $24
    clc
    adc $26
    sta $28
    lda $25
    adc $26+1
    sta $29
    lda $20
    ldx #0
    pha
    lda $28
    ldx $29
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
    bra @if_end36
@if_else35:
    .loc "test_simop_validation.c", 26
    lda #0
    sta $20
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #80
    ldx #0
    sta $24
    stx $25
    lda _test_count
    ldx _test_count+1
    sta $26
    stx $27
    lda $24
    clc
    adc $26
    sta $28
    lda $25
    adc $26+1
    sta $29
    lda $20
    ldx #0
    pha
    lda $28
    ldx $29
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
@if_end36:
    .loc "test_simop_validation.c", 29
    lda _test_count
    ldx _test_count+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    clc
    adc #1
    sta $20
    lda $23
    adc #0
    sta $21
    lda $20
    ldx $21
    sta _test_count
    stx _test_count+1
@inline_end33:
    .loc "test_simop_validation.c", 51
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_struct_array
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
    lda #170
    ldx #0
    sta $20
    stx $21
    lda $24
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $26
    ldx $27
    sta $24
    stx $25
    lda $20
    ldx $21
    sta $24
    stx $25
    .loc "test_simop_validation.c", 18
    lda _test_count
    ldx _test_count+1
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #32
    ldx #0
    sta $28
    stx $29
    lda _test_count
    ldx _test_count+1
    sta $2A
    stx $2B
    lda $28
    clc
    adc $2A
    sta $2C
    lda $29
    adc $2A+1
    sta $2D
    lda $24
    ldx #0
    pha
    lda $2C
    ldx $2D
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
    lda $20
    ldx $21
    sta $22
    .loc "test_simop_validation.c", 19
    lda _output
    ldx _output+1
    sta $24
    stx $25
    lda #48
    ldx #0
    sta $28
    stx $29
    lda _test_count
    ldx _test_count+1
    sta $2A
    stx $2B
    lda $28
    clc
    adc $2A
    sta $2C
    lda $29
    adc $2A+1
    sta $2D
    lda $22
    ldx #0
    pha
    lda $2C
    ldx $2D
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $24
    ldx $24+1
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
    lda $26
    ldx $27
    sta $22
    .loc "test_simop_validation.c", 20
    lda _output
    ldx _output+1
    sta $24
    stx $25
    lda #64
    ldx #0
    sta $28
    stx $29
    lda _test_count
    ldx _test_count+1
    sta $2A
    stx $2B
    lda $28
    clc
    adc $2A
    sta $2C
    lda $29
    adc $2A+1
    sta $2D
    lda $22
    ldx #0
    pha
    lda $2C
    ldx $2D
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $24
    ldx $24+1
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
    .loc "test_simop_validation.c", 22
    lda $26
    ldx $27
    cmp.16 .AX, #170
    bne @if_then39
    bra @if_else40
@if_then39:
    .loc "test_simop_validation.c", 23
    lda _errors
    ldx _errors+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    clc
    adc #1
    sta $20
    lda $23
    adc #0
    sta $21
    lda $20
    ldx $21
    sta _errors
    stx _errors+1
    .loc "test_simop_validation.c", 24
    lda #255
    sta $20
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #80
    ldx #0
    sta $24
    stx $25
    lda _test_count
    ldx _test_count+1
    sta $26
    stx $27
    lda $24
    clc
    adc $26
    sta $28
    lda $25
    adc $26+1
    sta $29
    lda $20
    ldx #0
    pha
    lda $28
    ldx $29
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
    bra @if_end41
@if_else40:
    .loc "test_simop_validation.c", 26
    lda #0
    sta $20
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #80
    ldx #0
    sta $24
    stx $25
    lda _test_count
    ldx _test_count+1
    sta $26
    stx $27
    lda $24
    clc
    adc $26
    sta $28
    lda $25
    adc $26+1
    sta $29
    lda $20
    ldx #0
    pha
    lda $28
    ldx $29
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
@if_end41:
    .loc "test_simop_validation.c", 29
    lda _test_count
    ldx _test_count+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    clc
    adc #1
    sta $20
    lda $23
    adc #0
    sta $21
    lda $20
    ldx $21
    sta _test_count
    stx _test_count+1
@inline_end38:
    .loc "test_simop_validation.c", 52
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_struct_array
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
    lda #187
    ldx #0
    sta $20
    stx $21
    lda $24
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $26
    ldx $27
    sta $24
    stx $25
    lda $20
    ldx $21
    sta $24
    stx $25
    .loc "test_simop_validation.c", 18
    lda _test_count
    ldx _test_count+1
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #32
    ldx #0
    sta $28
    stx $29
    lda _test_count
    ldx _test_count+1
    sta $2A
    stx $2B
    lda $28
    clc
    adc $2A
    sta $2C
    lda $29
    adc $2A+1
    sta $2D
    lda $24
    ldx #0
    pha
    lda $2C
    ldx $2D
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
    lda $20
    ldx $21
    sta $22
    .loc "test_simop_validation.c", 19
    lda _output
    ldx _output+1
    sta $24
    stx $25
    lda #48
    ldx #0
    sta $28
    stx $29
    lda _test_count
    ldx _test_count+1
    sta $2A
    stx $2B
    lda $28
    clc
    adc $2A
    sta $2C
    lda $29
    adc $2A+1
    sta $2D
    lda $22
    ldx #0
    pha
    lda $2C
    ldx $2D
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $24
    ldx $24+1
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
    lda $26
    ldx $27
    sta $22
    .loc "test_simop_validation.c", 20
    lda _output
    ldx _output+1
    sta $24
    stx $25
    lda #64
    ldx #0
    sta $28
    stx $29
    lda _test_count
    ldx _test_count+1
    sta $2A
    stx $2B
    lda $28
    clc
    adc $2A
    sta $2C
    lda $29
    adc $2A+1
    sta $2D
    lda $22
    ldx #0
    pha
    lda $2C
    ldx $2D
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $24
    ldx $24+1
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
    .loc "test_simop_validation.c", 22
    lda $26
    ldx $27
    cmp.16 .AX, #187
    bne @if_then44
    bra @if_else45
@if_then44:
    .loc "test_simop_validation.c", 23
    lda _errors
    ldx _errors+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    clc
    adc #1
    sta $20
    lda $23
    adc #0
    sta $21
    lda $20
    ldx $21
    sta _errors
    stx _errors+1
    .loc "test_simop_validation.c", 24
    lda #255
    sta $20
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #80
    ldx #0
    sta $24
    stx $25
    lda _test_count
    ldx _test_count+1
    sta $26
    stx $27
    lda $24
    clc
    adc $26
    sta $28
    lda $25
    adc $26+1
    sta $29
    lda $20
    ldx #0
    pha
    lda $28
    ldx $29
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
    bra @if_end46
@if_else45:
    .loc "test_simop_validation.c", 26
    lda #0
    sta $20
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #80
    ldx #0
    sta $24
    stx $25
    lda _test_count
    ldx _test_count+1
    sta $26
    stx $27
    lda $24
    clc
    adc $26
    sta $28
    lda $25
    adc $26+1
    sta $29
    lda $20
    ldx #0
    pha
    lda $28
    ldx $29
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
@if_end46:
    .loc "test_simop_validation.c", 29
    lda _test_count
    ldx _test_count+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    clc
    adc #1
    sta $20
    lda $23
    adc #0
    sta $21
    lda $20
    ldx $21
    sta _test_count
    stx _test_count+1
@inline_end43:
    .loc "test_simop_validation.c", 53
    lda #2
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_struct_array
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
    ldx #0
    sta $20
    stx $21
    lda $24
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $26
    ldx $27
    sta $24
    stx $25
    lda $20
    ldx $21
    sta $24
    stx $25
    .loc "test_simop_validation.c", 18
    lda _test_count
    ldx _test_count+1
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #32
    ldx #0
    sta $28
    stx $29
    lda _test_count
    ldx _test_count+1
    sta $2A
    stx $2B
    lda $28
    clc
    adc $2A
    sta $2C
    lda $29
    adc $2A+1
    sta $2D
    lda $24
    ldx #0
    pha
    lda $2C
    ldx $2D
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
    lda $20
    ldx $21
    sta $22
    .loc "test_simop_validation.c", 19
    lda _output
    ldx _output+1
    sta $24
    stx $25
    lda #48
    ldx #0
    sta $28
    stx $29
    lda _test_count
    ldx _test_count+1
    sta $2A
    stx $2B
    lda $28
    clc
    adc $2A
    sta $2C
    lda $29
    adc $2A+1
    sta $2D
    lda $22
    ldx #0
    pha
    lda $2C
    ldx $2D
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $24
    ldx $24+1
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
    lda $26
    ldx $27
    sta $22
    .loc "test_simop_validation.c", 20
    lda _output
    ldx _output+1
    sta $24
    stx $25
    lda #64
    ldx #0
    sta $28
    stx $29
    lda _test_count
    ldx _test_count+1
    sta $2A
    stx $2B
    lda $28
    clc
    adc $2A
    sta $2C
    lda $29
    adc $2A+1
    sta $2D
    lda $22
    ldx #0
    pha
    lda $2C
    ldx $2D
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $24
    ldx $24+1
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
    .loc "test_simop_validation.c", 22
    lda $26
    ldx $27
    cmp.16 .AX, #204
    bne @if_then49
    bra @if_else50
@if_then49:
    .loc "test_simop_validation.c", 23
    lda _errors
    ldx _errors+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    clc
    adc #1
    sta $20
    lda $23
    adc #0
    sta $21
    lda $20
    ldx $21
    sta _errors
    stx _errors+1
    .loc "test_simop_validation.c", 24
    lda #255
    sta $20
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #80
    ldx #0
    sta $24
    stx $25
    lda _test_count
    ldx _test_count+1
    sta $26
    stx $27
    lda $24
    clc
    adc $26
    sta $28
    lda $25
    adc $26+1
    sta $29
    lda $20
    ldx #0
    pha
    lda $28
    ldx $29
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
    bra @if_end51
@if_else50:
    .loc "test_simop_validation.c", 26
    lda #0
    sta $20
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #80
    ldx #0
    sta $24
    stx $25
    lda _test_count
    ldx _test_count+1
    sta $26
    stx $27
    lda $24
    clc
    adc $26
    sta $28
    lda $25
    adc $26+1
    sta $29
    lda $20
    ldx #0
    pha
    lda $28
    ldx $29
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
@if_end51:
    .loc "test_simop_validation.c", 29
    lda _test_count
    ldx _test_count+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    clc
    adc #1
    sta $20
    lda $23
    adc #0
    sta $21
    lda $20
    ldx $21
    sta _test_count
    stx _test_count+1
@inline_end48:
    .loc "test_simop_validation.c", 54
    lda #3
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_struct_array
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
    lda #221
    ldx #0
    sta $20
    stx $21
    lda $24
    ldx #0
    ldx #0
    sta $26
    stx $27
    lda $26
    ldx $27
    sta $24
    stx $25
    lda $20
    ldx $21
    sta $24
    stx $25
    .loc "test_simop_validation.c", 18
    lda _test_count
    ldx _test_count+1
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #32
    ldx #0
    sta $28
    stx $29
    lda _test_count
    ldx _test_count+1
    sta $2A
    stx $2B
    lda $28
    clc
    adc $2A
    sta $2C
    lda $29
    adc $2A+1
    sta $2D
    lda $24
    ldx #0
    pha
    lda $2C
    ldx $2D
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
    lda $20
    ldx $21
    sta $22
    .loc "test_simop_validation.c", 19
    lda _output
    ldx _output+1
    sta $24
    stx $25
    lda #48
    ldx #0
    sta $28
    stx $29
    lda _test_count
    ldx _test_count+1
    sta $2A
    stx $2B
    lda $28
    clc
    adc $2A
    sta $2C
    lda $29
    adc $2A+1
    sta $2D
    lda $22
    ldx #0
    pha
    lda $2C
    ldx $2D
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $24
    ldx $24+1
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
    lda $26
    ldx $27
    sta $22
    .loc "test_simop_validation.c", 20
    lda _output
    ldx _output+1
    sta $24
    stx $25
    lda #64
    ldx #0
    sta $28
    stx $29
    lda _test_count
    ldx _test_count+1
    sta $2A
    stx $2B
    lda $28
    clc
    adc $2A
    sta $2C
    lda $29
    adc $2A+1
    sta $2D
    lda $22
    ldx #0
    pha
    lda $2C
    ldx $2D
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $24
    ldx $24+1
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
    .loc "test_simop_validation.c", 22
    lda $26
    ldx $27
    cmp.16 .AX, #221
    bne @if_then54
    bra @if_else55
@if_then54:
    .loc "test_simop_validation.c", 23
    lda _errors
    ldx _errors+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    clc
    adc #1
    sta $20
    lda $23
    adc #0
    sta $21
    lda $20
    ldx $21
    sta _errors
    stx _errors+1
    .loc "test_simop_validation.c", 24
    lda #255
    sta $20
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #80
    ldx #0
    sta $24
    stx $25
    lda _test_count
    ldx _test_count+1
    sta $26
    stx $27
    lda $24
    clc
    adc $26
    sta $28
    lda $25
    adc $26+1
    sta $29
    lda $20
    ldx #0
    pha
    lda $28
    ldx $29
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
    bra @if_end56
@if_else55:
    .loc "test_simop_validation.c", 26
    lda #0
    sta $20
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #80
    ldx #0
    sta $24
    stx $25
    lda _test_count
    ldx _test_count+1
    sta $26
    stx $27
    lda $24
    clc
    adc $26
    sta $28
    lda $25
    adc $26+1
    sta $29
    lda $20
    ldx #0
    pha
    lda $28
    ldx $29
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
@if_end56:
    .loc "test_simop_validation.c", 29
    lda _test_count
    ldx _test_count+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda $22
    clc
    adc #1
    sta $20
    lda $23
    adc #0
    sta $21
    lda $20
    ldx $21
    sta _test_count
    stx _test_count+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc


    .segment "data"
__str_7:
    .text "array[0]"
    .byte 0
__str_12:
    .text "array[1]"
    .byte 0
__str_17:
    .text "array[2]"
    .byte 0
__str_22:
    .text "array[3]"
    .byte 0
__str_27:
    .text "array[4]"
    .byte 0
__str_32:
    .text "array[5]"
    .byte 0
__str_37:
    .text "struct[0]"
    .byte 0
__str_42:
    .text "struct[1]"
    .byte 0
__str_47:
    .text "struct[2]"
    .byte 0
__str_52:
    .text "struct[3]"
    .byte 0

__zp_save_buf:

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
    .global _get_array_value
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
    .byte 119
    .byte 136

    .segment "code"

; function _get_array_value
; SAC inline storage: 2 bytes
    .global _get_array_value__param_index
    _get_array_value__param_index: .word 0
    _get_array_value__local_0: .word 0
    proc _get_array_value, W#@_p_index
    .sac
    .var _fp = 0
    .loc "test_simop_runtime.c", 8
    .var @_p_index = 2
; .debug_var: __get_array_value @_p_index offset=2 size=2 type=int16 scope=parameter

@entry:
    .loc "test_simop_runtime.c", 9
    lda _get_array_value__param_index
    ldx _get_array_value__param_index+1
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_test_array
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
    sta $22
    lda $22
    ldx #0
    sxt.8
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z
    .frame_size 2
    endproc

; function _main
; SAC inline storage: 2 bytes
    _main__local_0: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_simop_runtime.c", 12
    .local @_l_i = 0
; .debug_var: __main @_l_i offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_simop_runtime.c", 17
    lda #0
    sta _main__local_0
    sta _main__local_0+1
@for_cond0:
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, #8
    bcc @for_body1
    bra @for_end3
@for_body1:
    .loc "test_simop_runtime.c", 18
    lda _main__local_0
    ldx _main__local_0+1
    sta $24
    stx $25
    .loc "test_simop_runtime.c", 9
    lda _main__local_0
    ldx _main__local_0+1
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_test_array
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
    sta $28
    lda $28
    ldx #0
    sta $2A
    stx $2B
    lda $2A
    ldx $2B
    sta $2C
    stx $2D
@inline_end4:
    lda $2C
    ldx $2D
    sta $2E
    .loc "test_simop_runtime.c", 18
    lda _output
    ldx _output+1
    sta $30
    stx $31
    lda $2E
    ldx #0
    pha
    lda _main__local_0
    ldx _main__local_0+1
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
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
@for_inc2:
    .loc "test_simop_runtime.c", 17
    lda _main__local_0
    ldx _main__local_0+1
    sta $34
    stx $35
    lda $34
    clc
    adc #1
    sta $36
    lda $35
    adc #0
    sta $37
    lda $36
    ldx $37
    sta _main__local_0
    stx _main__local_0+1
    bra @for_cond0
@for_end3:
    .loc "test_simop_runtime.c", 22
    lda #170
    sta $20
    lda _output
    ldx _output+1
    sta $22
    stx $23
    lda #8
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
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc


__zp_save_buf:

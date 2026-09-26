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

    .extern _memset
    .extern _strlen

    .global _buf
    .global _main

    .segment "bss"
_buf:
; .debug_var: @global _buf offset=0 size=2 type=int8 scope=global
    .res 10

    .segment "code"

; function _main
; SAC inline storage: 0 bytes
    _main__local_0: .word 0
    _main__local_1: .word 0
    _main__local_3: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/string.h", 5

@entry:
    .loc "test_include_flag.c", 10
    lda #10
    ldx #0
    push .ax
    lda #0
    ldx #0
    push .ax
    ldax #_buf
    push .ax
    jsr _memset
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    .loc "test_include_flag.c", 11
    ldax #__str_0
    sta _main__local_3
    stx _main__local_3+1
    lda _main__local_3
    ldx _main__local_3+1
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    jsr _strlen
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda #0
    sta $20
    sta $21
    lda $22
    ldx #0
    pha
    lda $20
    ldx $21
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_buf
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
    .loc "test_include_flag.c", 12
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 0
    endproc


    .segment "data"
__str_0:
    .text "hello"
    .byte 0

__zp_save_buf:

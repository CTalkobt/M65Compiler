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
    .global _a
    .global _b
    .global _c
    .global _d
    .global _main

    .segment "data"
    .byte 0
_result:
; .debug_var: @global _result offset=0 size=2 type=ptr scope=global
    .word 16384
_a:
; .debug_var: @global _a offset=0 size=2 type=int16 scope=global
    .word 3
_b:
; .debug_var: @global _b offset=0 size=2 type=int16 scope=global
    .word 5
_c:
; .debug_var: @global _c offset=0 size=2 type=int16 scope=global
    .word 2
_d:
; .debug_var: @global _d offset=0 size=2 type=int16 scope=global
    .word 4

    .segment "code"

; function _main
; SAC inline storage: 0 bytes
    _main__local_3: .word 0
    _main__local_10: .word 0
    _main__local_19: .word 0
    _main__local_30: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_expr_reentrant.c", 12

@entry:
    .loc "test_expr_reentrant.c", 14
    lda _a
    ldx _a+1
    sta $20
    stx $21
    lda _c
    ldx _c+1
    sta $22
    stx $23
    lda $20
    ldx $21
    mul.s16 .AX, $22
    sta $24
    stx $25
    lda _result
    ldx _result+1
    sta $20
    stx $21
    lda #0
    sta $22
    sta $23
    lda $24
    ldx $25
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
    .loc "test_expr_reentrant.c", 17
    lda _a
    ldx _a+1
    sta $20
    stx $21
    lda _b
    ldx _b+1
    sta $22
    stx $23
    lda $20
    ldx $21
    clc
    adc $22
    sta $24
    stx $25
    lda _result
    ldx _result+1
    sta $20
    stx $21
    lda #1
    ldx #0
    sta $22
    stx $23
    lda $24
    ldx $25
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
    .loc "test_expr_reentrant.c", 20
    lda _a
    ldx _a+1
    sta $20
    stx $21
    lda _b
    ldx _b+1
    sta $22
    stx $23
    lda $20
    ldx $21
    mul.s16 .AX, $22
    sta $24
    stx $25
    lda _c
    ldx _c+1
    sta $20
    stx $21
    lda $24
    ldx $25
    clc
    adc $20
    sta $22
    stx $23
    lda _result
    ldx _result+1
    sta $20
    stx $21
    lda #2
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    pha
    lda $24
    ldx $25
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
    .loc "test_expr_reentrant.c", 25
    lda _a
    ldx _a+1
    sta $20
    stx $21
    lda _b
    ldx _b+1
    sta $22
    stx $23
    lda $20
    ldx $21
    mul.s16 .AX, $22
    sta $24
    stx $25
    lda _c
    ldx _c+1
    sta $20
    stx $21
    lda _d
    ldx _d+1
    sta $22
    stx $23
    lda $20
    ldx $21
    mul.s16 .AX, $22
    sta $26
    stx $27
    lda $24
    ldx $25
    clc
    adc $26
    sta $20
    stx $21
    lda _result
    ldx _result+1
    sta $22
    stx $23
    lda #3
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx $21
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
    .loc "test_expr_reentrant.c", 28
    lda #170
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
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 0
    endproc


__zp_save_buf:

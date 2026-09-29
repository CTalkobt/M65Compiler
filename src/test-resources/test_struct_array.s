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
    .global _pts
    .global _main

    .segment "data"
    .byte 0
_result:
; .debug_var: @global _result offset=0 size=2 type=ptr scope=global
    .word 16384
_pts:
; .debug_var: @global _pts offset=0 size=4 type=int32 scope=global
    .dword 0
    .res 12

    .segment "code"

; function _main
; SAC inline storage: 2 bytes
    _main__local_0: .word 0
    _main__local_7: .word 0
    _main__local_12: .word 0
    _main__local_18: .word 0
    _main__local_26: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_struct_array.c", 10
    .local @_l_i = 0
; .debug_var: __main @_l_i offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_struct_array.c", 14
    lda #0
    sta _main__local_0
    sta _main__local_0+1
@for_cond0:
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, #4
    bcc @for_body1
    bra @for_end3
@for_body1:
    .loc "test_struct_array.c", 15
    lda #10
    ldx #0
    sta $24
    stx $25
    lda _main__local_0
    ldx _main__local_0+1
    mul.16 .AX, $24
    sta $26
    stx $27
    lda _main__local_0
    ldx _main__local_0+1
    mul.16 .AX, #4
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_pts
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $28
    stx $28+1
    lda $26
    ldx $27
    ldy #0
    sta ($28),y
    txa
    iny
    sta ($28),y
    .loc "test_struct_array.c", 16
    lda #10
    ldx #0
    sta $2A
    stx $2B
    lda _main__local_0
    ldx _main__local_0+1
    mul.16 .AX, $2A
    sta $2C
    stx $2D
    lda $2C
    clc
    adc #1
    sta $30
    lda $2D
    adc #0
    sta $31
    lda $28
    ldx $29
    add.16 .AX, #2
    sta $32
    stx $33
    lda $30
    ldx $31
    ldy #0
    sta ($32),y
    txa
    iny
    sta ($32),y
@for_inc2:
    .loc "test_struct_array.c", 14
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
    .loc "test_struct_array.c", 20
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    mul.16 .AX, #4
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_pts
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda _result
    ldx _result+1
    sta $20
    stx $21
    lda #0
    sta $24
    sta $25
    lda $22
    ldx #0
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
    .loc "test_struct_array.c", 21
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    mul.16 .AX, #4
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_pts
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda _result
    ldx _result+1
    sta $20
    stx $21
    lda #1
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx #0
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
    .loc "test_struct_array.c", 22
    lda #2
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    mul.16 .AX, #4
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_pts
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
    lda $22
    ldx $23
    add.16 .AX, #2
    sta $20
    stx $21
    ldy #0
    lda ($20),y
    pha
    iny
    lda ($20),y
    tax
    pla
    sta $22
    stx $23
    lda $22
    ldx $23
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
    .loc "test_struct_array.c", 23
    lda #3
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    mul.16 .AX, #4
    sta __zp_scratch3
    stx __zp_scratch3+1
    ldax #_pts
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta $22
    stx $22+1
    lda $22
    ldx $23
    add.16 .AX, #2
    sta $20
    stx $21
    ldy #0
    lda ($20),y
    pha
    iny
    lda ($20),y
    tax
    pla
    sta $22
    stx $23
    lda $22
    ldx $23
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
    .loc "test_struct_array.c", 24
    lda #16
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda _result
    ldx _result+1
    sta $20
    stx $21
    lda #4
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx #0
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
    .loc "test_struct_array.c", 25
    lda #170
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
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc


__zp_save_buf:

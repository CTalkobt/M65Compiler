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
    .global _main

    .segment "data"
    .byte 0
_output:
; .debug_var: @global _output offset=0 size=2 type=ptr scope=global
    .word 16384

    .segment "code"

; function _main
; SAC inline storage: 16 bytes
    _main__local_0: .word 0
    _main__local_8: .word 0
    _main__local_15: .word 0
    _main__local_35: .word 0
    _main__local_43: .word 0
    _main__local_56: .word 0
    _main__local_64: .word 0
    _main__local_71: .word 0
    _main__local_78: .word 0
    _main__local_85: .word 0
    _main__local_89: .word 0
    _main__local_93: .word 0
    _main__local_99: .word 0
    _main__local_109: .word 0
    _main__local_113: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_array_trace.c", 5
    .local @_l_arr = 10
    .local @_l_hi = 2
    .local @_l_i = 6
    .local @_l_lo = 0
    .local @_l_v = 8
    .local @_l_val = 4
; .debug_var: __main @_l_arr offset=10 size=2 type=int16 scope=local
; .debug_var: __main @_l_hi offset=2 size=2 type=int8 scope=local
; .debug_var: __main @_l_i offset=6 size=2 type=int16 scope=local
; .debug_var: __main @_l_lo offset=0 size=2 type=int8 scope=local
; .debug_var: __main @_l_v offset=8 size=2 type=int16 scope=local
; .debug_var: __main @_l_val offset=4 size=2 type=int16 scope=local

@entry:
    .loc "test_array_trace.c", 7
    leax.local 10
    sta $20
    stx $21
    lda #100
    ldx #0
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
    lda #200
    ldx #0
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
    lda #44
    ldx #1
    sta $22
    stx $23
    lda $22
    ldx $23
    pha
    phx
    struct_elem.16 __zp_scratch, $20, #4
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    .loc "test_array_trace.c", 10
    leax.local 10
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
    lda #255
    ldx #0
    sta $20
    stx $21
    lda $26
    ldx $27
    and.16 .AX, $20
    sta $22
    stx $23
    lda $22
    ldx $23
    sta _main__local_8
    .loc "test_array_trace.c", 11
    leax.local 10
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
    txa
    ldx #0
    sta $20
    stx $21
    lda #255
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    and.16 .AX, $22
    sta $24
    stx $25
    lda $24
    ldx $25
    sta _main__local_15
    .loc "test_array_trace.c", 12
    lda _output
    ldx _output+1
    sta $20
    stx $21
    lda #0
    sta $22
    sta $23
    lda _main__local_8
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
    .loc "test_array_trace.c", 13
    lda _output
    ldx _output+1
    sta $20
    stx $21
    lda #1
    ldx #0
    sta $22
    stx $23
    lda _main__local_15
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
    .loc "test_array_trace.c", 15
    leax.local 10
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
    lda #255
    ldx #0
    sta $20
    stx $21
    lda $26
    ldx $27
    and $20
    sta $22
    stx $23
    lda $22
    ldx $23
    sta _main__local_8
    .loc "test_array_trace.c", 16
    leax.local 10
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
    txa
    ldx #0
    sta $20
    stx $21
    lda #255
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    and $22
    sta $24
    stx $25
    lda $24
    ldx $25
    sta _main__local_15
    .loc "test_array_trace.c", 17
    lda _output
    ldx _output+1
    sta $20
    stx $21
    lda #2
    ldx #0
    sta $22
    stx $23
    lda _main__local_8
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
    .loc "test_array_trace.c", 18
    lda _output
    ldx _output+1
    sta $20
    stx $21
    lda #3
    ldx #0
    sta $22
    stx $23
    lda _main__local_15
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
    .loc "test_array_trace.c", 20
    leax.local 10
    sta $20
    stx $21
    lda #2
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
    lda #255
    ldx #0
    sta $20
    stx $21
    lda $26
    ldx $27
    and $20
    sta $22
    stx $23
    lda $22
    ldx $23
    sta _main__local_8
    .loc "test_array_trace.c", 21
    leax.local 10
    sta $20
    stx $21
    lda #2
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
    txa
    ldx #0
    sta $20
    stx $21
    lda #255
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    and $22
    sta $24
    stx $25
    lda $24
    ldx $25
    sta _main__local_15
    .loc "test_array_trace.c", 22
    lda _output
    ldx _output+1
    sta $20
    stx $21
    lda #4
    ldx #0
    sta $22
    stx $23
    lda _main__local_8
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
    .loc "test_array_trace.c", 23
    lda _output
    ldx _output+1
    sta $20
    stx $21
    lda #5
    ldx #0
    sta $22
    stx $23
    lda _main__local_15
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
    .loc "test_array_trace.c", 26
    leax.local 10
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
    sta _main__local_71
    stx _main__local_71+1
    .loc "test_array_trace.c", 27
    lda #255
    ldx #0
    sta $20
    stx $21
    lda _main__local_71
    ldx _main__local_71+1
    and $20
    sta $22
    stx $23
    lda _output
    ldx _output+1
    sta $20
    stx $21
    lda #6
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
    .loc "test_array_trace.c", 28
    lda _main__local_71
    ldx _main__local_71+1
    txa
    ldx #0
    sta $20
    stx $21
    lda #255
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx $21
    and $22
    sta $24
    stx $25
    lda _output
    ldx _output+1
    sta $20
    stx $21
    lda #7
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
    .loc "test_array_trace.c", 32
    lda #0
    sta _main__local_89
    sta _main__local_89+1
@for_cond0_ph:
    .loc "test_array_trace.c", 33
    leax.local 10
    sta $20
    stx $21
@for_cond0:
    .loc "test_array_trace.c", 32
    lda _main__local_89
    ldx _main__local_89+1
    cmp.16 .AX, #3
    bcc @for_body1
    bra @for_end3
@for_body1:
    .loc "test_array_trace.c", 33
    lda _main__local_89
    ldx _main__local_89+1
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
    sta $28
    stx $29
    lda $28
    ldx $29
    sta _main__local_93
    stx _main__local_93+1
    .loc "test_array_trace.c", 34
    lda #255
    ldx #0
    sta $2A
    stx $2B
    lda _main__local_93
    ldx _main__local_93+1
    and $2A
    sta $2C
    stx $2D
    lda _output
    ldx _output+1
    sta $2E
    stx $2F
    lda #8
    ldx #0
    sta $30
    stx $31
    lda #2
    ldx #0
    sta $32
    stx $33
    lda _main__local_89
    ldx _main__local_89+1
    lsl.16 .AX
    sta $34
    stx $35
    lda $30
    clc
    adc $34
    sta $36
    lda $31
    adc $34+1
    sta $37
    lda $2C
    ldx $2D
    pha
    lda $36
    ldx $37
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $2E
    ldx $2E+1
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
    .loc "test_array_trace.c", 35
    lda _main__local_93
    ldx _main__local_93+1
    txa
    ldx #0
    sta $3A
    stx $3B
    lda #255
    ldx #0
    sta $3C
    stx $3D
    lda $3A
    ldx $3B
    and $3C
    sta $3E
    stx $3F
    lda _output
    ldx _output+1
    sta $40
    stx $41
    lda #8
    ldx #0
    sta $42
    stx $43
    lda #2
    ldx #0
    sta $44
    stx $45
    lda $42
    clc
    adc $34
    sta $46
    lda $43
    adc $34+1
    sta $47
    lda $46
    clc
    adc #1
    sta $4A
    lda $47
    adc #0
    sta $4B
    lda $3E
    ldx $3F
    pha
    lda $4A
    ldx $4B
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $40
    ldx $40+1
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
    .loc "test_array_trace.c", 32
    lda _main__local_89
    ldx _main__local_89+1
    sta $4E
    stx $4F
    lda $4E
    clc
    adc #1
    sta $50
    lda $4F
    adc #0
    sta $51
    lda $50
    ldx $51
    sta _main__local_89
    stx _main__local_89+1
    bra @for_cond0
@for_end3:
    .loc "test_array_trace.c", 39
    lda #170
    sta $20
    lda _output
    ldx _output+1
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
    .loc "test_array_trace.c", 41
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 16
    endproc


__zp_save_buf:

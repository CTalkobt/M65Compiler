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

    .global _shr
    .global _shl
    .global _shr16
    .global _main

    .segment "code"

; function _shr

    ; Static buffer for struct return from _shr
    _shr__struct_buf:
    .byte 0, 0, 0, 0

; SAC inline storage: 6 bytes
    .global _shr__param_v
    _shr__param_v: .long 0
    .global _shr__param_n
    _shr__param_n: .word 0
    _shr__local_0: .long 0
    _shr__local_1: .word 0
    _shr__local_4: .word 0
    proc _shr, D#@_p_v, W#@_p_n
    .sac
    .var _fp = 0
    .loc "test_shift_edge.c", 3
    .var @_p_v = 2
    .var @_p_n = 6
; .debug_var: __shr @_p_v offset=2 size=4 type=int32 scope=parameter
; .debug_var: __shr @_p_n offset=6 size=2 type=int16 scope=parameter

@entry:
    .loc "test_shift_edge.c", 5
    lda _shr__param_n
    ldx _shr__param_n+1
    stx __zp_scratch
    ora __zp_scratch
    beq @if_then0
    bra @if_end2
@if_then0:
    lda _shr__param_v
    ldx _shr__param_v+1
    ldy _shr__param_v+2
    ldz _shr__param_v+3
    bra @__return
@if_end2:
    .loc "test_shift_edge.c", 6
    lda _shr__param_n
    ldx _shr__param_n+1
    cmp.16 .AX, #1
    beq @if_then3
    bra @if_end5
@if_then3:
    lda _shr__param_v
    ldx _shr__param_v+1
    ldy _shr__param_v+2
    ldz _shr__param_v+3
    lsr.32 .AXYZ
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    bra @__return
@if_end5:
    .loc "test_shift_edge.c", 7
    lda _shr__param_n
    ldx _shr__param_n+1
    cmp.16 .AX, #8
    beq @if_then6
    bra @if_end8
@if_then6:
    lda _shr__param_v
    ldx _shr__param_v+1
    ldy _shr__param_v+2
    ldz _shr__param_v+3
    txa
    pha
    tya
    tax
    tza
    tay
    ldz #0
    pla
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    bra @__return
@if_end8:
    .loc "test_shift_edge.c", 8
    lda _shr__param_n
    ldx _shr__param_n+1
    cmp.16 .AX, #16
    beq @if_then9
    bra @if_end11
@if_then9:
    lda _shr__param_v
    ldx _shr__param_v+1
    ldy _shr__param_v+2
    ldz _shr__param_v+3
    tza
    tax
    tya
    ldy #0
    ldz #0
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    bra @__return
@if_end11:
    .loc "test_shift_edge.c", 9
    lda _shr__param_n
    ldx _shr__param_n+1
    cmp.16 .AX, #24
    beq @if_then12
    bra @if_end14
@if_then12:
    lda _shr__param_v
    ldx _shr__param_v+1
    ldy _shr__param_v+2
    ldz _shr__param_v+3
    tza
    ldx #0
    ldy #0
    ldz #0
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    bra @__return
@if_end14:
    .loc "test_shift_edge.c", 10
    lda _shr__param_n
    ldx _shr__param_n+1
    cmp.16 .AX, #31
    beq @if_then15
    bra @if_end17
@if_then15:
    lda _shr__param_v
    ldx _shr__param_v+1
    ldy _shr__param_v+2
    ldz _shr__param_v+3
    lsr.32 .AXYZ
    lsr.32 .AXYZ
    lsr.32 .AXYZ
    lsr.32 .AXYZ
    lsr.32 .AXYZ
    lsr.32 .AXYZ
    lsr.32 .AXYZ
    lsr.32 .AXYZ
    lsr.32 .AXYZ
    lsr.32 .AXYZ
    lsr.32 .AXYZ
    lsr.32 .AXYZ
    lsr.32 .AXYZ
    lsr.32 .AXYZ
    lsr.32 .AXYZ
    lsr.32 .AXYZ
    lsr.32 .AXYZ
    lsr.32 .AXYZ
    lsr.32 .AXYZ
    lsr.32 .AXYZ
    lsr.32 .AXYZ
    lsr.32 .AXYZ
    lsr.32 .AXYZ
    lsr.32 .AXYZ
    lsr.32 .AXYZ
    lsr.32 .AXYZ
    lsr.32 .AXYZ
    lsr.32 .AXYZ
    lsr.32 .AXYZ
    lsr.32 .AXYZ
    lsr.32 .AXYZ
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    bra @__return
@if_end17:
    lda _shr__param_n
    ldx _shr__param_n+1
    ldy #0
    ldz #0
    sta $20
    stx $21
    sty $22
    stz $23
    .loc "test_shift_edge.c", 11
    lda _shr__param_v
    ldx _shr__param_v+1
    ldy _shr__param_v+2
    ldz _shr__param_v+3
    lda $20
    sta __zp_scratch4
    lda _shr__param_v
    ldx _shr__param_v+1
    ldy _shr__param_v+2
    ldz _shr__param_v+3
    lda __zp_scratch4
    beq @__shr_done_1
    sta __zp_scratch4
@__shr_loop_0:
    lsr.32 .AXYZ
    dec __zp_scratch4
    bne @__shr_loop_0
@__shr_done_1:
    sta $24
    stx $25
    sty $26
    stz $27
    lda $24
    ldx $25
    ldy $26
    ldz $27
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _shl

    ; Static buffer for struct return from _shl
    _shl__struct_buf:
    .byte 0, 0, 0, 0

; SAC inline storage: 6 bytes
    .global _shl__param_v
    _shl__param_v: .long 0
    .global _shl__param_n
    _shl__param_n: .word 0
    _shl__local_0: .long 0
    _shl__local_1: .word 0
    _shl__local_4: .word 0
    proc _shl, D#@_p_v, W#@_p_n
    .sac
    .var _fp = 0
    .loc "test_shift_edge.c", 14
    .var @_p_v = 2
    .var @_p_n = 6
; .debug_var: __shl @_p_v offset=2 size=4 type=int32 scope=parameter
; .debug_var: __shl @_p_n offset=6 size=2 type=int16 scope=parameter

@entry:
    .loc "test_shift_edge.c", 15
    lda _shl__param_n
    ldx _shl__param_n+1
    stx __zp_scratch
    ora __zp_scratch
    beq @if_then18
    bra @if_end20
@if_then18:
    lda _shl__param_v
    ldx _shl__param_v+1
    ldy _shl__param_v+2
    ldz _shl__param_v+3
    bra @__return
@if_end20:
    .loc "test_shift_edge.c", 16
    lda _shl__param_n
    ldx _shl__param_n+1
    cmp.16 .AX, #1
    beq @if_then21
    bra @if_end23
@if_then21:
    lda _shl__param_v
    ldx _shl__param_v+1
    ldy _shl__param_v+2
    ldz _shl__param_v+3
    lsl.32 .AXYZ
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    bra @__return
@if_end23:
    .loc "test_shift_edge.c", 17
    lda _shl__param_n
    ldx _shl__param_n+1
    cmp.16 .AX, #8
    beq @if_then24
    bra @if_end26
@if_then24:
    lda _shl__param_v
    ldx _shl__param_v+1
    ldy _shl__param_v+2
    ldz _shl__param_v+3
    pha
    tya
    taz
    txa
    tay
    pla
    tax
    lda #0
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    bra @__return
@if_end26:
    .loc "test_shift_edge.c", 18
    lda _shl__param_n
    ldx _shl__param_n+1
    cmp.16 .AX, #16
    beq @if_then27
    bra @if_end29
@if_then27:
    lda _shl__param_v
    ldx _shl__param_v+1
    ldy _shl__param_v+2
    ldz _shl__param_v+3
    tay
    txa
    taz
    lda #0
    ldx #0
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    bra @__return
@if_end29:
    .loc "test_shift_edge.c", 19
    lda _shl__param_n
    ldx _shl__param_n+1
    cmp.16 .AX, #24
    beq @if_then30
    bra @if_end32
@if_then30:
    lda _shl__param_v
    ldx _shl__param_v+1
    ldy _shl__param_v+2
    ldz _shl__param_v+3
    taz
    lda #0
    ldx #0
    ldy #0
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    bra @__return
@if_end32:
    lda _shl__param_n
    ldx _shl__param_n+1
    ldy #0
    ldz #0
    sta $20
    stx $21
    sty $22
    stz $23
    .loc "test_shift_edge.c", 20
    lda _shl__param_v
    ldx _shl__param_v+1
    ldy _shl__param_v+2
    ldz _shl__param_v+3
    lda $20
    sta __zp_scratch4
    lda _shl__param_v
    ldx _shl__param_v+1
    ldy _shl__param_v+2
    ldz _shl__param_v+3
    lda __zp_scratch4
    beq @__shl_done_3
    sta __zp_scratch4
@__shl_loop_2:
    lsl.32 .AXYZ
    dec __zp_scratch4
    bne @__shl_loop_2
@__shl_done_3:
    sta $24
    stx $25
    sty $26
    stz $27
    lda $24
    ldx $25
    ldy $26
    ldz $27
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _shr16
; SAC inline storage: 4 bytes
    .global _shr16__param_v
    _shr16__param_v: .word 0
    .global _shr16__param_n
    _shr16__param_n: .word 0
    _shr16__local_0: .word 0
    _shr16__local_1: .word 0
    _shr16__local_4: .word 0
    proc _shr16, W#@_p_v, W#@_p_n
    .sac
    .var _fp = 0
    .loc "test_shift_edge.c", 24
    .var @_p_v = 2
    .var @_p_n = 4
; .debug_var: __shr16 @_p_v offset=2 size=2 type=int16 scope=parameter
; .debug_var: __shr16 @_p_n offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_shift_edge.c", 25
    lda _shr16__param_n
    ldx _shr16__param_n+1
    stx __zp_scratch
    ora __zp_scratch
    beq @if_then33
    bra @if_end35
@if_then33:
    lda _shr16__param_v
    ldx _shr16__param_v+1
    bra @__return
@if_end35:
    .loc "test_shift_edge.c", 26
    lda _shr16__param_n
    ldx _shr16__param_n+1
    cmp.16 .AX, #8
    beq @if_then36
    bra @if_end38
@if_then36:
    lda _shr16__param_v
    ldx _shr16__param_v+1
    txa
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    bra @__return
@if_end38:
    .loc "test_shift_edge.c", 27
    lda _shr16__param_v
    ldx _shr16__param_v+1
    lda _shr16__param_n
    ldx _shr16__param_n+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    ldy __zp_scratch2
    beq @__shr_done_5
@__shr_loop_4:
    lsr.16 .AX
    dey
    bne @__shr_loop_4
@__shr_done_5:
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _main
; SAC inline storage: 24 bytes
    _main__local_0: .long 0
    _main__local_2: .word 0
    _main__local_7: .word 0
    _main__local_12: .word 0
    _main__local_17: .long 0
    _main__local_20: .word 0
    _main__local_25: .long 0
    _main__local_28: .word 0
    _main__local_33: .long 0
    _main__local_36: .word 0
    _main__local_42: .word 0
    _main__local_43: .long 0
    _main__local_49: .word 0
    _main__local_50: .long 0
    _main__local_56: .word 0
    _main__local_57: .long 0
    _main__local_62: .long 0
    _main__local_64: .word 0
    _main__local_65: .long 0
    _main__local_67: .word 0
    _main__local_72: .long 0
    _main__local_74: .word 0
    _main__local_75: .long 0
    _main__local_77: .word 0
    _main__local_84: .word 0
    _main__local_85: .word 0
    _main__local_86: .word 0
    _main__local_89: .word 0
    _main__local_98: .word 0
    _main__local_99: .word 0
    _main__local_100: .word 0
    _main__local_103: .word 0
    _main__local_112: .word 0
    _main__local_113: .word 0
    _main__local_114: .word 0
    _main__local_117: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_shift_edge.c", 30
    .local @_l_big = 4
    .local @_l_huge = 8
    .local @_l_s16 = 16
    .local @_l_s24 = 20
    .local @_l_top = 12
    .local @_l_v = 0
; .debug_var: __main @_l_big offset=4 size=4 type=int32 scope=local
; .debug_var: __main @_l_huge offset=8 size=4 type=int32 scope=local
; .debug_var: __main @_l_s16 offset=16 size=4 type=int32 scope=local
; .debug_var: __main @_l_s24 offset=20 size=4 type=int32 scope=local
; .debug_var: __main @_l_top offset=12 size=4 type=int32 scope=local
; .debug_var: __main @_l_v offset=0 size=4 type=int32 scope=local

@entry:
    .loc "test_shift_edge.c", 31
    lda #0
    sta _main__local_0
    lda #1
    sta _main__local_0+1
    .loc "test_shift_edge.c", 36
    lda #0
    tax
    sta _main__local_2
    stx _main__local_2+1
    lda #0
    ldx #0
    sta _shr__param_n
    stx _shr__param_n+1
    lda _main__local_0
    ldx _main__local_0+1
    ldy _main__local_0+2
    ldz _main__local_0+3
    sta _shr__param_v
    stx _shr__param_v+1
    sty _shr__param_v+2
    stz _shr__param_v+3
    jsr _shr
    sta $20
    stx $21
    sty $22
    stz $23
    lda #0
    ldx #1
    sta $24
    stx $25
    lda $24
    ldx $25
    ldy #0
    ldz #0
    sta $26
    stx $27
    sty $28
    stz $29
    lda $20
    ldx $21
    ldy $22
    ldz $23
    cmp.32 .AXYZ, $26
    bne @if_then39
    bra @if_end41
@if_then39:
    lda #1
    ldx #0
    bra @__return
@if_end41:
    .loc "test_shift_edge.c", 39
    lda #1
    ldx #0
    sta _main__local_7
    stx _main__local_7+1
    lda #1
    ldx #0
    sta _shr__param_n
    stx _shr__param_n+1
    lda _main__local_0
    ldx _main__local_0+1
    ldy _main__local_0+2
    ldz _main__local_0+3
    sta _shr__param_v
    stx _shr__param_v+1
    sty _shr__param_v+2
    stz _shr__param_v+3
    jsr _shr
    sta $20
    stx $21
    sty $22
    stz $23
    lda #128
    ldx #0
    sta $24
    stx $25
    lda $24
    ldx $25
    ldy #0
    ldz #0
    sta $26
    stx $27
    sty $28
    stz $29
    lda $20
    ldx $21
    ldy $22
    ldz $23
    cmp.32 .AXYZ, $26
    bne @if_then42
    bra @if_end44
@if_then42:
    lda #2
    ldx #0
    bra @__return
@if_end44:
    .loc "test_shift_edge.c", 42
    lda #8
    ldx #0
    sta _main__local_12
    stx _main__local_12+1
    lda #8
    ldx #0
    sta _shr__param_n
    stx _shr__param_n+1
    lda _main__local_0
    ldx _main__local_0+1
    ldy _main__local_0+2
    ldz _main__local_0+3
    sta _shr__param_v
    stx _shr__param_v+1
    sty _shr__param_v+2
    stz _shr__param_v+3
    jsr _shr
    sta $20
    stx $21
    sty $22
    stz $23
    lda #1
    ldx #0
    sta $24
    stx $25
    lda $24
    ldx $25
    ldy #0
    ldz #0
    sta $26
    stx $27
    sty $28
    stz $29
    lda $20
    ldx $21
    ldy $22
    ldz $23
    cmp.32 .AXYZ, $26
    bne @if_then45
    bra @if_end47
@if_then45:
    lda #3
    ldx #0
    bra @__return
@if_end47:
    .loc "test_shift_edge.c", 46
    lda #1
    sta _main__local_17
    lda #0
    sta _main__local_17+1
    .loc "test_shift_edge.c", 47
    lda _main__local_17
    ldx _main__local_17+1
    ldy _main__local_17+2
    ldz _main__local_17+3
    tay
    txa
    taz
    lda #0
    ldx #0
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    sta _main__local_17
    stx _main__local_17+1
    sty _main__local_17+2
    stz _main__local_17+3
    .loc "test_shift_edge.c", 48
    lda #16
    ldx #0
    sta _main__local_20
    stx _main__local_20+1
    lda #16
    ldx #0
    sta _shr__param_n
    stx _shr__param_n+1
    lda _main__local_17
    ldx _main__local_17+1
    ldy _main__local_17+2
    ldz _main__local_17+3
    sta _shr__param_v
    stx _shr__param_v+1
    sty _shr__param_v+2
    stz _shr__param_v+3
    jsr _shr
    sta $20
    stx $21
    sty $22
    stz $23
    lda #1
    ldx #0
    sta $24
    stx $25
    lda $24
    ldx $25
    ldy #0
    ldz #0
    sta $26
    stx $27
    sty $28
    stz $29
    lda $20
    ldx $21
    ldy $22
    ldz $23
    cmp.32 .AXYZ, $26
    bne @if_then48
    bra @if_end50
@if_then48:
    lda #4
    ldx #0
    bra @__return
@if_end50:
    .loc "test_shift_edge.c", 51
    lda #1
    sta _main__local_25
    lda #0
    sta _main__local_25+1
    .loc "test_shift_edge.c", 52
    lda _main__local_25
    ldx _main__local_25+1
    ldy _main__local_25+2
    ldz _main__local_25+3
    taz
    lda #0
    ldx #0
    ldy #0
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    sta _main__local_25
    stx _main__local_25+1
    sty _main__local_25+2
    stz _main__local_25+3
    .loc "test_shift_edge.c", 53
    lda #24
    ldx #0
    sta _main__local_28
    stx _main__local_28+1
    lda #24
    ldx #0
    sta _shr__param_n
    stx _shr__param_n+1
    lda _main__local_25
    ldx _main__local_25+1
    ldy _main__local_25+2
    ldz _main__local_25+3
    sta _shr__param_v
    stx _shr__param_v+1
    sty _shr__param_v+2
    stz _shr__param_v+3
    jsr _shr
    sta $20
    stx $21
    sty $22
    stz $23
    lda #1
    ldx #0
    sta $24
    stx $25
    lda $24
    ldx $25
    ldy #0
    ldz #0
    sta $26
    stx $27
    sty $28
    stz $29
    lda $20
    ldx $21
    ldy $22
    ldz $23
    cmp.32 .AXYZ, $26
    bne @if_then51
    bra @if_end53
@if_then51:
    lda #5
    ldx #0
    bra @__return
@if_end53:
    .loc "test_shift_edge.c", 56
    lda #1
    sta _main__local_33
    lda #0
    sta _main__local_33+1
    .loc "test_shift_edge.c", 57
    lda _main__local_33
    ldx _main__local_33+1
    ldy _main__local_33+2
    ldz _main__local_33+3
    lsl.32 .AXYZ
    lsl.32 .AXYZ
    lsl.32 .AXYZ
    lsl.32 .AXYZ
    lsl.32 .AXYZ
    lsl.32 .AXYZ
    lsl.32 .AXYZ
    lsl.32 .AXYZ
    lsl.32 .AXYZ
    lsl.32 .AXYZ
    lsl.32 .AXYZ
    lsl.32 .AXYZ
    lsl.32 .AXYZ
    lsl.32 .AXYZ
    lsl.32 .AXYZ
    lsl.32 .AXYZ
    lsl.32 .AXYZ
    lsl.32 .AXYZ
    lsl.32 .AXYZ
    lsl.32 .AXYZ
    lsl.32 .AXYZ
    lsl.32 .AXYZ
    lsl.32 .AXYZ
    lsl.32 .AXYZ
    lsl.32 .AXYZ
    lsl.32 .AXYZ
    lsl.32 .AXYZ
    lsl.32 .AXYZ
    lsl.32 .AXYZ
    lsl.32 .AXYZ
    lsl.32 .AXYZ
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    sta _main__local_33
    stx _main__local_33+1
    sty _main__local_33+2
    stz _main__local_33+3
    .loc "test_shift_edge.c", 58
    lda #31
    ldx #0
    sta _main__local_36
    stx _main__local_36+1
    lda #31
    ldx #0
    sta _shr__param_n
    stx _shr__param_n+1
    lda _main__local_33
    ldx _main__local_33+1
    ldy _main__local_33+2
    ldz _main__local_33+3
    sta _shr__param_v
    stx _shr__param_v+1
    sty _shr__param_v+2
    stz _shr__param_v+3
    jsr _shr
    sta $20
    stx $21
    sty $22
    stz $23
    lda #1
    ldx #0
    sta $24
    stx $25
    lda $24
    ldx $25
    ldy #0
    ldz #0
    sta $26
    stx $27
    sty $28
    stz $29
    lda $20
    ldx $21
    ldy $22
    ldz $23
    cmp.32 .AXYZ, $26
    bne @if_then54
    bra @if_end56
@if_then54:
    lda #6
    ldx #0
    bra @__return
@if_end56:
    .loc "test_shift_edge.c", 63
    lda #1
    ldx #0
    sta $20
    stx $21
    lda #0
    tax
    sta _main__local_42
    stx _main__local_42+1
    lda $20
    ldx $21
    ldy #0
    ldz #0
    sta _main__local_43
    stx _main__local_43+1
    sty _main__local_43+2
    stz _main__local_43+3
    lda #0
    ldx #0
    sta _shl__param_n
    stx _shl__param_n+1
    lda _main__local_43
    ldx _main__local_43+1
    ldy _main__local_43+2
    ldz _main__local_43+3
    sta _shl__param_v
    stx _shl__param_v+1
    sty _shl__param_v+2
    stz _shl__param_v+3
    jsr _shl
    sta $20
    stx $21
    sty $22
    stz $23
    lda #1
    ldx #0
    sta $24
    stx $25
    lda $24
    ldx $25
    ldy #0
    ldz #0
    sta $26
    stx $27
    sty $28
    stz $29
    lda $20
    ldx $21
    ldy $22
    ldz $23
    cmp.32 .AXYZ, $26
    bne @if_then57
    bra @if_end59
@if_then57:
    lda #7
    ldx #0
    bra @__return
@if_end59:
    .loc "test_shift_edge.c", 66
    lda #1
    ldx #0
    sta $20
    stx $21
    lda #1
    ldx #0
    sta _main__local_49
    stx _main__local_49+1
    lda $20
    ldx $21
    ldy #0
    ldz #0
    sta _main__local_50
    stx _main__local_50+1
    sty _main__local_50+2
    stz _main__local_50+3
    lda #1
    ldx #0
    sta _shl__param_n
    stx _shl__param_n+1
    lda _main__local_50
    ldx _main__local_50+1
    ldy _main__local_50+2
    ldz _main__local_50+3
    sta _shl__param_v
    stx _shl__param_v+1
    sty _shl__param_v+2
    stz _shl__param_v+3
    jsr _shl
    sta $20
    stx $21
    sty $22
    stz $23
    lda #2
    ldx #0
    sta $24
    stx $25
    lda $24
    ldx $25
    ldy #0
    ldz #0
    sta $26
    stx $27
    sty $28
    stz $29
    lda $20
    ldx $21
    ldy $22
    ldz $23
    cmp.32 .AXYZ, $26
    bne @if_then60
    bra @if_end62
@if_then60:
    lda #8
    ldx #0
    bra @__return
@if_end62:
    .loc "test_shift_edge.c", 69
    lda #1
    ldx #0
    sta $20
    stx $21
    lda #8
    ldx #0
    sta _main__local_56
    stx _main__local_56+1
    lda $20
    ldx $21
    ldy #0
    ldz #0
    sta _main__local_57
    stx _main__local_57+1
    sty _main__local_57+2
    stz _main__local_57+3
    lda #8
    ldx #0
    sta _shl__param_n
    stx _shl__param_n+1
    lda _main__local_57
    ldx _main__local_57+1
    ldy _main__local_57+2
    ldz _main__local_57+3
    sta _shl__param_v
    stx _shl__param_v+1
    sty _shl__param_v+2
    stz _shl__param_v+3
    jsr _shl
    sta $20
    stx $21
    sty $22
    stz $23
    lda #0
    ldx #1
    sta $24
    stx $25
    lda $24
    ldx $25
    ldy #0
    ldz #0
    sta $26
    stx $27
    sty $28
    stz $29
    lda $20
    ldx $21
    ldy $22
    ldz $23
    cmp.32 .AXYZ, $26
    bne @if_then63
    bra @if_end65
@if_then63:
    lda #9
    ldx #0
    bra @__return
@if_end65:
    .loc "test_shift_edge.c", 72
    lda #1
    ldx #0
    sta $20
    stx $21
    lda #16
    ldx #0
    sta _main__local_64
    stx _main__local_64+1
    lda $20
    ldx $21
    ldy #0
    ldz #0
    sta _main__local_65
    stx _main__local_65+1
    sty _main__local_65+2
    stz _main__local_65+3
    lda #16
    ldx #0
    sta _shl__param_n
    stx _shl__param_n+1
    lda _main__local_65
    ldx _main__local_65+1
    ldy _main__local_65+2
    ldz _main__local_65+3
    sta _shl__param_v
    stx _shl__param_v+1
    sty _shl__param_v+2
    stz _shl__param_v+3
    jsr _shl
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    sta _main__local_62
    stx _main__local_62+1
    sty _main__local_62+2
    stz _main__local_62+3
    .loc "test_shift_edge.c", 73
    lda #16
    ldx #0
    sta _main__local_67
    stx _main__local_67+1
    lda #16
    ldx #0
    sta _shr__param_n
    stx _shr__param_n+1
    lda _main__local_62
    ldx _main__local_62+1
    ldy _main__local_62+2
    ldz _main__local_62+3
    sta _shr__param_v
    stx _shr__param_v+1
    sty _shr__param_v+2
    stz _shr__param_v+3
    jsr _shr
    sta $20
    stx $21
    sty $22
    stz $23
    lda #1
    ldx #0
    sta $24
    stx $25
    lda $24
    ldx $25
    ldy #0
    ldz #0
    sta $26
    stx $27
    sty $28
    stz $29
    lda $20
    ldx $21
    ldy $22
    ldz $23
    cmp.32 .AXYZ, $26
    bne @if_then66
    bra @if_end68
@if_then66:
    lda #10
    ldx #0
    bra @__return
@if_end68:
    .loc "test_shift_edge.c", 76
    lda #1
    ldx #0
    sta $20
    stx $21
    lda #24
    ldx #0
    sta _main__local_74
    stx _main__local_74+1
    lda $20
    ldx $21
    ldy #0
    ldz #0
    sta _main__local_75
    stx _main__local_75+1
    sty _main__local_75+2
    stz _main__local_75+3
    lda #24
    ldx #0
    sta _shl__param_n
    stx _shl__param_n+1
    lda _main__local_75
    ldx _main__local_75+1
    ldy _main__local_75+2
    ldz _main__local_75+3
    sta _shl__param_v
    stx _shl__param_v+1
    sty _shl__param_v+2
    stz _shl__param_v+3
    jsr _shl
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    sta _main__local_72
    stx _main__local_72+1
    sty _main__local_72+2
    stz _main__local_72+3
    .loc "test_shift_edge.c", 77
    lda #24
    ldx #0
    sta _main__local_77
    stx _main__local_77+1
    lda #24
    ldx #0
    sta _shr__param_n
    stx _shr__param_n+1
    lda _main__local_72
    ldx _main__local_72+1
    ldy _main__local_72+2
    ldz _main__local_72+3
    sta _shr__param_v
    stx _shr__param_v+1
    sty _shr__param_v+2
    stz _shr__param_v+3
    jsr _shr
    sta $20
    stx $21
    sty $22
    stz $23
    lda #1
    ldx #0
    sta $24
    stx $25
    lda $24
    ldx $25
    ldy #0
    ldz #0
    sta $26
    stx $27
    sty $28
    stz $29
    lda $20
    ldx $21
    ldy $22
    ldz $23
    cmp.32 .AXYZ, $26
    bne @if_then69
    bra @if_end71
@if_then69:
    lda #11
    ldx #0
    bra @__return
@if_end71:
    .loc "test_shift_edge.c", 82
    lda #0
    ldx #1
    sta $20
    stx $21
    lda #0
    sta $22
    sta $23
    .loc "test_shift_edge.c", 25
    lda $22
    ora $23
    beq @if_then76
    bra @if_end78
@if_then76:
    bra @inline_end75
@if_end78:
    .loc "test_shift_edge.c", 26
    lda $22
    ldx $23
    cmp.16 .AX, #8
    beq @if_then80
    bra @if_end82
@if_then80:
    lda $20
    ldx $21
    txa
    ldx #0
    sta $24
    stx $25
    bra @inline_end75
@if_end82:
    .loc "test_shift_edge.c", 27
    lda $20
    ldx $21
    ldy $22
    beq @__shr_done_7
@__shr_loop_6:
    lsr.16 .AX
    dey
    bne @__shr_loop_6
@__shr_done_7:
    sta $24
    stx $25
@inline_end75:
    .loc "test_shift_edge.c", 82
    lda $24
    ldx $25
    cmp.16 .AX, #256
    bne @if_then72
    bra @if_end74
@if_then72:
    lda #12
    ldx #0
    bra @__return
@if_end74:
    .loc "test_shift_edge.c", 85
    lda #0
    ldx #1
    sta $20
    stx $21
    lda #8
    ldx #0
    sta $22
    stx $23
    .loc "test_shift_edge.c", 25
    lda $22
    ora $23
    beq @if_then89
    bra @if_end91
@if_then89:
    bra @inline_end88
@if_end91:
    .loc "test_shift_edge.c", 26
    lda $22
    ldx $23
    cmp.16 .AX, #8
    beq @if_then93
    bra @if_end95
@if_then93:
    lda $20
    ldx $21
    txa
    ldx #0
    sta $24
    stx $25
    bra @inline_end88
@if_end95:
    .loc "test_shift_edge.c", 27
    lda $20
    ldx $21
    ldy $22
    beq @__shr_done_9
@__shr_loop_8:
    lsr.16 .AX
    dey
    bne @__shr_loop_8
@__shr_done_9:
    sta $24
    stx $25
@inline_end88:
    .loc "test_shift_edge.c", 85
    lda $24
    ldx $25
    cmp.16 .AX, #1
    bne @if_then85
    bra @if_end87
@if_then85:
    lda #13
    ldx #0
    bra @__return
@if_end87:
    .loc "test_shift_edge.c", 88
    lda #52
    ldx #18
    sta $20
    stx $21
    lda #4
    ldx #0
    sta $22
    stx $23
    .loc "test_shift_edge.c", 25
    lda $22
    ora $23
    beq @if_then102
    bra @if_end104
@if_then102:
    bra @inline_end101
@if_end104:
    .loc "test_shift_edge.c", 26
    lda $22
    ldx $23
    cmp.16 .AX, #8
    beq @if_then106
    bra @if_end108
@if_then106:
    lda $20
    ldx $21
    txa
    ldx #0
    sta $24
    stx $25
    bra @inline_end101
@if_end108:
    .loc "test_shift_edge.c", 27
    lda $20
    ldx $21
    ldy $22
    beq @__shr_done_11
@__shr_loop_10:
    lsr.16 .AX
    dey
    bne @__shr_loop_10
@__shr_done_11:
    sta $24
    stx $25
@inline_end101:
    .loc "test_shift_edge.c", 88
    lda $24
    ldx $25
    cmp.16 .AX, #291
    bne @if_then98
    bra @if_end100
@if_then98:
    lda #14
    ldx #0
    bra @__return
@if_end100:
    .loc "test_shift_edge.c", 90
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 24
    endproc


__zp_save_buf:

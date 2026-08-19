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

    .global _add
    .global _main

    .segment "code"

; function _add
; SAC inline storage: 4 bytes
    .global _add__param_a
    _add__param_a: .word 0
    .global _add__param_b
    _add__param_b: .word 0
    _add__local_0: .word 0
    _add__local_1: .word 0
    proc _add, W#@_p_a, W#@_p_b
    .sac
    .var _fp = 0
    .loc "test_register.c", 4
    .var @_p_a = 2
    .var @_p_b = 4
; .debug_var: __add @_p_a offset=2 size=2 type=int16 scope=parameter
; .debug_var: __add @_p_b offset=4 size=2 type=int16 scope=parameter

@entry:
    .loc "test_register.c", 5
    lda _add__param_b
    ldx _add__param_b+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _add__param_a
    ldx _add__param_a+1
    add.16 .AX, __zp_scratch2
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
; SAC inline storage: 14 bytes
    _main__local_28: .word 0
    _main__local_32: .word 0
    _main__local_42: .word 0
    _main__local_101: .word 0
    _main__local_112: .word 0
    _main__local_116: .word 0
    _main__local_120: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_register.c", 8
    .var @_l_big = $30
    .var @_l_c = $24
    .var @_l_ch = $32
    .var @_l_counter = $34
    .var @_l_dc = $36
    .var @_l_fi = $3C
    .local @_l_fsum = 6
    .var @_l_i = $20
    .var @_l_inner = $3A
    .var @_l_outer = $38
    .var @_l_rb = $3E
    .var @_l_rd = $40
    .var @_l_reg_b = $28
    .local @_l_result = 4
    .local @_l_sa = 8
    .local @_l_sc = 10
    .var @_l_self = $42
    .local @_l_stack_a = 0
    .local @_l_stack_c = 2
    .var @_l_sum = $26
    .local @_l_total = 12
    .var @_l_x = $2A
    .var @_l_y = $2C
    .var @_l_z = $2E
; .debug_var: __main @_l_fsum offset=6 size=2 type=int16 scope=local
; .debug_var: __main @_l_result offset=4 size=2 type=int16 scope=local
; .debug_var: __main @_l_sa offset=8 size=2 type=int16 scope=local
; .debug_var: __main @_l_sc offset=10 size=2 type=int16 scope=local
; .debug_var: __main @_l_stack_a offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_stack_c offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_total offset=12 size=2 type=int16 scope=local

@entry:
    .loc "test_register.c", 10
    lda #10
    sta $20
    ldx #0
    stx $21
    .loc "test_register.c", 11
    lda $20
    ldx $21
    cmp.16 .AX, #10
    bne @if_then0
    bra @if_end2
@if_then0:
    lda #1
    ldx #0
    bra @__return
@if_end2:
    .loc "test_register.c", 14
    lda #42
    sta $24
    ldx #0
    stx $25
    .loc "test_register.c", 15
    lda #42
    sta $22
    lda $24
    ldx #0
    ldx #0
    sta $44
    stx $45
    lda $22
    ldx #0
    ldx #0
    sta $46
    stx $47
    lda $44
    ldx $45
    cmp.16 .AX, $46
    bne @if_then3
    bra @if_end5
@if_then3:
    lda #2
    ldx #0
    bra @__return
@if_end5:
    .loc "test_register.c", 18
    lda #99
    sta $20
    ldx #0
    stx $21
    .loc "test_register.c", 19
    lda $20
    ldx $21
    cmp.16 .AX, #99
    bne @if_then6
    bra @if_end8
@if_then6:
    lda #3
    ldx #0
    bra @__return
@if_end8:
    .loc "test_register.c", 22
    lda #7
    sta $24
    .loc "test_register.c", 23
    lda #7
    sta $22
    lda $24
    ldx #0
    ldx #0
    sta $44
    stx $45
    lda $22
    ldx #0
    ldx #0
    sta $46
    stx $47
    lda $44
    ldx $45
    cmp.16 .AX, $46
    bne @if_then9
    bra @if_end11
@if_then9:
    lda #4
    ldx #0
    bra @__return
@if_end11:
    .loc "test_register.c", 26
    lda #0
    sta $26
    sta $27
    .loc "test_register.c", 27
    lda #0
    sta $20
    sta $21
@for_cond12:
    lda $20
    ldx $21
    cmp.16 .AX, #5
    bcc @for_body13
    bra @for_end15
@for_body13:
    .loc "test_register.c", 28
    lda $26
    clc
    adc $20
    sta $26
    lda $27
    adc $20+1
    sta $27
@for_inc14:
    .loc "test_register.c", 27
    inc $20
    bne *+4
    inc $21
    bra @for_cond12
@for_end15:
    .loc "test_register.c", 30
    lda $26
    ldx $27
    cmp.16 .AX, #10
    bne @if_then16
    bra @if_end18
@if_then16:
    lda #5
    ldx #0
    bra @__return
@if_end18:
    .loc "test_register.c", 33
    lda #100
    sta _main__local_28
    lda #0
    sta _main__local_28+1
    .loc "test_register.c", 34
    lda #200
    sta $28
    ldx #0
    stx $29
    .loc "test_register.c", 35
    lda #44
    sta _main__local_32
    lda #1
    sta _main__local_32+1
    .loc "test_register.c", 36
    lda _main__local_28
    ldx _main__local_28+1
    add.16 .AX, $28
    sta $22
    stx $23
    lda _main__local_32
    ldx _main__local_32+1
    add.16 .AX, $22
    sta $44
    stx $45
    lda $44
    ldx $45
    cmp.16 .AX, #600
    bne @if_then19
    bra @if_end21
@if_then19:
    lda #6
    ldx #0
    bra @__return
@if_end21:
    .loc "test_register.c", 39
    lda #15
    sta $2A
    ldx #0
    stx $2B
    .loc "test_register.c", 40
    lda #25
    sta $2C
    ldx #0
    stx $2D
    .loc "test_register.c", 41
    lda $2A
    ldx $2B
    sta $22
    stx $23
    lda $2C
    ldx $2D
    sta $22
    stx $23
    .loc "test_register.c", 5
    lda $2A
    clc
    adc $2C
    sta $22
    lda $2B
    adc $2C+1
    sta $23
    lda $22
    ldx $23
    sta $44
    stx $45
@inline_end22:
    .loc "test_register.c", 41
    lda $44
    ldx $45
    sta _main__local_42
    stx _main__local_42+1
    .loc "test_register.c", 42
    lda _main__local_42
    ldx _main__local_42+1
    cmp.16 .AX, #40
    bne @if_then24
    bra @if_end26
@if_then24:
    lda #7
    ldx #0
    bra @__return
@if_end26:
    .loc "test_register.c", 45
    lda #3
    ldx #0
    sta $22
    stx $23
    lda #4
    ldx #0
    sta $44
    stx $45
    lda $22
    ldx $23
    sta $46
    stx $47
    lda $44
    ldx $45
    sta $46
    stx $47
    .loc "test_register.c", 5
    lda $22
    clc
    adc #4
    sta $46
    lda $23
    adc #0
    sta $47
    lda $46
    ldx $47
    sta $22
    stx $23
@inline_end27:
    .loc "test_register.c", 45
    lda $22
    ldx $23
    sta $2E
    stx $2F
    .loc "test_register.c", 46
    lda $2E
    ldx $2F
    cmp.16 .AX, #7
    bne @if_then29
    bra @if_end31
@if_then29:
    lda #8
    ldx #0
    bra @__return
@if_end31:
    .loc "test_register.c", 49
    lda #232
    sta $30
    ldx #3
    stx $31
    .loc "test_register.c", 50
    lda $30
    ldx $31
    cmp.16 .AX, #1000
    bne @if_then32
    bra @if_end34
@if_then32:
    lda #9
    ldx #0
    bra @__return
@if_end34:
    .loc "test_register.c", 53
    lda #255
    sta $32
    ldx #0
    stx $33
    .loc "test_register.c", 54
    lda #255
    sta $22
    lda $32
    ldx #0
    ldx #0
    sta $44
    stx $45
    lda $22
    ldx #0
    ldx #0
    sta $46
    stx $47
    lda $44
    ldx $45
    cmp.16 .AX, $46
    bne @if_then35
    bra @if_end37
@if_then35:
    lda #10
    ldx #0
    bra @__return
@if_end37:
    .loc "test_register.c", 57
    lda #0
    sta $34
    sta $35
    .loc "test_register.c", 58
    inc $34
    bne *+4
    inc $35
    .loc "test_register.c", 59
    inc $34
    bne *+4
    inc $35
    .loc "test_register.c", 60
    inc $34
    bne *+4
    inc $35
    .loc "test_register.c", 61
    lda $34
    ldx $35
    cmp.16 .AX, #3
    bne @if_then38
    bra @if_end40
@if_then38:
    lda #11
    ldx #0
    bra @__return
@if_end40:
    .loc "test_register.c", 64
    lda #10
    sta $36
    ldx #0
    stx $37
    .loc "test_register.c", 65
    lda #1
    sta $22
    lda $36
    ldx #0
    ldx #0
    sta $44
    stx $45
    lda $22
    ldx #0
    ldx #0
    sta $46
    stx $47
    lda $44
    sec
    sbc $46
    sta $22
    lda $45
    sbc $46+1
    sta $23
    lda $22
    ldx $23
    sta $46
    lda $46
    ldx #0
    sta $36
    .loc "test_register.c", 66
    lda #1
    sta $22
    lda $44
    ldx $45
    sta $46
    stx $47
    lda $22
    ldx #0
    ldx #0
    sta $46
    stx $47
    lda $44
    sec
    sbc $46
    sta $22
    lda $45
    sbc $46+1
    sta $23
    lda $22
    ldx $23
    sta $46
    lda $46
    ldx #0
    sta $36
    .loc "test_register.c", 67
    lda #8
    sta $22
    lda $44
    ldx $45
    sta $46
    stx $47
    lda $22
    ldx #0
    ldx #0
    sta $46
    stx $47
    lda $44
    ldx $45
    cmp.16 .AX, $46
    bne @if_then41
    bra @if_end43
@if_then41:
    lda #12
    ldx #0
    bra @__return
@if_end43:
    .loc "test_register.c", 70
    lda #1
    sta $38
    ldx #0
    stx $39
    .loc "test_register.c", 72
    lda #2
    sta $3A
    ldx #0
    stx $3B
    .loc "test_register.c", 73
    lda $38
    clc
    adc $3A
    sta $38
    lda $39
    adc $3A+1
    sta $39
    .loc "test_register.c", 75
    lda $38
    ldx $39
    cmp.16 .AX, #3
    bne @if_then44
    bra @if_end46
@if_then44:
    lda #13
    ldx #0
    bra @__return
@if_end46:
    .loc "test_register.c", 78
    lda #0
    sta _main__local_101
    sta _main__local_101+1
    .loc "test_register.c", 79
    lda #1
    sta $3C
    ldx #0
    stx $3D
@for_cond47:
    lda $3C
    ldx $3D
    cmp.16 .AX, #5
    bcc @for_body48
    beq @for_body48
    bra @for_end50
@for_body48:
    .loc "test_register.c", 80
    lda _main__local_101
    ldx _main__local_101+1
    add.16 .AX, $3C
    sta $46
    stx $47
    sta _main__local_101
    stx _main__local_101+1
@for_inc49:
    .loc "test_register.c", 79
    inc $3C
    bne *+4
    inc $3D
    bra @for_cond47
@for_end50:
    .loc "test_register.c", 82
    lda _main__local_101
    ldx _main__local_101+1
    cmp.16 .AX, #15
    bne @if_then51
    bra @if_end53
@if_then51:
    lda #14
    ldx #0
    bra @__return
@if_end53:
    .loc "test_register.c", 85
    lda #10
    sta _main__local_112
    lda #0
    sta _main__local_112+1
    .loc "test_register.c", 86
    lda #20
    sta $3E
    ldx #0
    stx $3F
    .loc "test_register.c", 87
    lda #30
    sta _main__local_116
    lda #0
    sta _main__local_116+1
    .loc "test_register.c", 88
    lda #40
    sta $40
    ldx #0
    stx $41
    .loc "test_register.c", 89
    lda _main__local_112
    ldx _main__local_112+1
    add.16 .AX, $3E
    sta $22
    stx $23
    lda _main__local_116
    ldx _main__local_116+1
    add.16 .AX, $22
    sta $44
    stx $45
    lda $44
    clc
    adc $40
    sta $22
    lda $45
    adc $40+1
    sta $23
    lda $22
    ldx $23
    sta _main__local_120
    stx _main__local_120+1
    .loc "test_register.c", 90
    lda _main__local_120
    ldx _main__local_120+1
    cmp.16 .AX, #100
    bne @if_then54
    bra @if_end56
@if_then54:
    lda #15
    ldx #0
    bra @__return
@if_end56:
    .loc "test_register.c", 93
    lda #42
    sta $42
    ldx #0
    stx $43
    .loc "test_register.c", 94
    lda $42
    ldx $43
    sta $42
    stx $43
    .loc "test_register.c", 95
    lda $42
    ldx $43
    cmp.16 .AX, #42
    bne @if_then57
    bra @if_end59
@if_then57:
    lda #16
    ldx #0
    bra @__return
@if_end59:
    .loc "test_register.c", 97
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 14
    endproc


__zp_save_buf:

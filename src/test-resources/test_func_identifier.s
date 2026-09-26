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

    .global _check_func_name
    .global _verify_main
    .global _test_file_macro
    .global _test_line_macro
    .global _main

    .segment "code"

; function _check_func_name
; SAC inline storage: 4 bytes
    _check_func_name__local_0: .word 0
    _check_func_name__local_37: .word 0
    proc _check_func_name
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_func_identifier.c", 5
    .local @_l_len = 2
    .local @_l_name = 0
; .debug_var: __check_func_name @_l_len offset=2 size=2 type=int16 scope=local
; .debug_var: __check_func_name @_l_name offset=0 size=2 type=ptr scope=local

@entry:
    .loc "test_func_identifier.c", 6
    ldax #__str_0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _check_func_name__local_0
    stx _check_func_name__local_0+1
    .loc "test_func_identifier.c", 8
    lda #0
    sta $20
    sta $21
    lda _check_func_name__local_0
    ldx _check_func_name__local_0+1
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
    lda #99
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
    bne @if_then1
    bra @if_end3
@if_then1:
    lda #1
    ldx #0
    bra @__return
@if_end3:
    .loc "test_func_identifier.c", 9
    lda #1
    ldx #0
    sta $20
    stx $21
    lda _check_func_name__local_0
    ldx _check_func_name__local_0+1
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
    lda #104
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
    bne @if_then4
    bra @if_end6
@if_then4:
    lda #2
    ldx #0
    bra @__return
@if_end6:
    .loc "test_func_identifier.c", 10
    lda #2
    ldx #0
    sta $20
    stx $21
    lda _check_func_name__local_0
    ldx _check_func_name__local_0+1
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
    lda #101
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
    bne @if_then7
    bra @if_end9
@if_then7:
    lda #3
    ldx #0
    bra @__return
@if_end9:
    .loc "test_func_identifier.c", 11
    lda #3
    ldx #0
    sta $20
    stx $21
    lda _check_func_name__local_0
    ldx _check_func_name__local_0+1
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
    lda #99
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
    bne @if_then10
    bra @if_end12
@if_then10:
    lda #4
    ldx #0
    bra @__return
@if_end12:
    .loc "test_func_identifier.c", 12
    lda #4
    ldx #0
    sta $20
    stx $21
    lda _check_func_name__local_0
    ldx _check_func_name__local_0+1
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
    lda #107
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
    bne @if_then13
    bra @if_end15
@if_then13:
    lda #5
    ldx #0
    bra @__return
@if_end15:
    .loc "test_func_identifier.c", 15
    lda #0
    sta _check_func_name__local_37
    sta _check_func_name__local_37+1
@while_cond16_ph:
    .loc "test_func_identifier.c", 16
    lda #0
    sta $20
    lda $20
    ldx #0
    ldx #0
    sta $22
    stx $23
@while_cond16:
    lda _check_func_name__local_0
    ldx _check_func_name__local_0+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _check_func_name__local_37
    ldx _check_func_name__local_37+1
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
    lda #0
    sta $26
    lda $24
    ldx #0
    ldx #0
    sta $28
    stx $29
    lda $28
    ldx $29
    cmp.16 .AX, $22
    bne @while_body17
    bra @while_end18
@while_body17:
    .loc "test_func_identifier.c", 17
    inc.16f __vr37
    bra @while_cond16
@while_end18:
    .loc "test_func_identifier.c", 20
    lda _check_func_name__local_37
    ldx _check_func_name__local_37+1
    cmp.16 .AX, #16
    bne @if_then19
    bra @if_end21
@if_then19:
    lda #6
    ldx #0
    bra @__return
@if_end21:
    .loc "test_func_identifier.c", 22
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _verify_main
; SAC inline storage: 4 bytes
    _verify_main__local_0: .word 0
    _verify_main__local_9: .word 0
    proc _verify_main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_func_identifier.c", 25
    .local @_l_len = 2
    .local @_l_name = 0
; .debug_var: __verify_main @_l_len offset=2 size=2 type=int16 scope=local
; .debug_var: __verify_main @_l_name offset=0 size=2 type=ptr scope=local

@entry:
    .loc "test_func_identifier.c", 26
    ldax #__str_22
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _verify_main__local_0
    stx _verify_main__local_0+1
    .loc "test_func_identifier.c", 28
    lda #0
    sta $20
    sta $21
    lda _verify_main__local_0
    ldx _verify_main__local_0+1
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
    lda #118
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
    bne @if_then23
    bra @if_end25
@if_then23:
    lda #10
    ldx #0
    bra @__return
@if_end25:
    .loc "test_func_identifier.c", 30
    lda #0
    sta _verify_main__local_9
    sta _verify_main__local_9+1
@while_cond26_ph:
    .loc "test_func_identifier.c", 31
    lda #0
    sta $20
    lda $20
    ldx #0
    ldx #0
    sta $22
    stx $23
@while_cond26:
    lda _verify_main__local_0
    ldx _verify_main__local_0+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _verify_main__local_9
    ldx _verify_main__local_9+1
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
    lda #0
    sta $26
    lda $24
    ldx #0
    ldx #0
    sta $28
    stx $29
    lda $28
    ldx $29
    cmp.16 .AX, $22
    bne @while_body27
    bra @while_end28
@while_body27:
    .loc "test_func_identifier.c", 32
    inc.16f __vr9
    bra @while_cond26
@while_end28:
    .loc "test_func_identifier.c", 35
    lda _verify_main__local_9
    ldx _verify_main__local_9+1
    cmp.16 .AX, #11
    bne @if_then29
    bra @if_end31
@if_then29:
    lda #11
    ldx #0
    bra @__return
@if_end31:
    .loc "test_func_identifier.c", 37
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _test_file_macro
; SAC inline storage: 2 bytes
    _test_file_macro__local_0: .word 0
    proc _test_file_macro
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_func_identifier.c", 40
    .local @_l_file = 0
; .debug_var: __test_file_macro @_l_file offset=0 size=2 type=ptr scope=local

@entry:
    .loc "test_func_identifier.c", 41
    ldax #__str_32
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _test_file_macro__local_0
    stx _test_file_macro__local_0+1
    .loc "test_func_identifier.c", 43
    lda #0
    sta $20
    sta $21
    lda _test_file_macro__local_0
    ldx _test_file_macro__local_0+1
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
    lda #0
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
    beq @if_then33
    bra @if_end35
@if_then33:
    lda #20
    ldx #0
    bra @__return
@if_end35:
    .loc "test_func_identifier.c", 44
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _test_line_macro
; SAC inline storage: 2 bytes
    _test_line_macro__local_0: .word 0
    proc _test_line_macro
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_func_identifier.c", 47
    .local @_l_line = 0
; .debug_var: __test_line_macro @_l_line offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_func_identifier.c", 48
    lda #48
    sta _test_line_macro__local_0
    lda #0
    sta _test_line_macro__local_0+1
    .loc "test_func_identifier.c", 50
    lda _test_line_macro__local_0
    ldx _test_line_macro__local_0+1
    cmp.16 .AX, #1
    bcc @if_then36
    bra @if_end38
@if_then36:
    lda #30
    ldx #0
    bra @__return
@if_end38:
    .loc "test_func_identifier.c", 51
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _main
; SAC inline storage: 10 bytes
    _main__local_0: .word 0
    _main__local_5: .word 0
    _main__local_15: .word 0
    _main__local_32: .word 0
    _main__local_46: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_func_identifier.c", 54
    .local @_l_file = 6
    .local @_l_len = 4
    .local @_l_line = 8
    .local @_l_name = 2
    .local @_l_result = 0
; .debug_var: __main @_l_file offset=6 size=2 type=ptr scope=local
; .debug_var: __main @_l_len offset=4 size=2 type=int16 scope=local
; .debug_var: __main @_l_line offset=8 size=2 type=int16 scope=local
; .debug_var: __main @_l_name offset=2 size=2 type=ptr scope=local
; .debug_var: __main @_l_result offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_func_identifier.c", 57
    jsr _check_func_name
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_0
    stx _main__local_0+1
    .loc "test_func_identifier.c", 58
    lda _main__local_0
    ldx _main__local_0+1
    stx __zp_scratch
    ora __zp_scratch
    bne @if_then39
    bra @if_end41
@if_then39:
    lda _main__local_0
    ldx _main__local_0+1
    bra @__return
@if_end41:
    .loc "test_func_identifier.c", 26
    ldax #__str_43
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_5
    stx _main__local_5+1
    .loc "test_func_identifier.c", 28
    lda #0
    sta $20
    sta $21
    lda _main__local_5
    ldx _main__local_5+1
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
    lda #118
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
    bne @if_then44
    bra @if_end46
@if_then44:
    lda #10
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end42
@if_end46:
    .loc "test_func_identifier.c", 30
    lda #0
    sta _main__local_15
    sta _main__local_15+1
@while_cond48_ph:
    .loc "test_func_identifier.c", 31
    lda #0
    sta $20
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
@while_cond48:
    lda _main__local_5
    ldx _main__local_5+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_15
    ldx _main__local_15+1
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
    sta $26
    lda #0
    sta $28
    lda $26
    ldx #0
    ldx #0
    sta $2A
    stx $2B
    lda $2A
    ldx $2B
    cmp.16 .AX, $24
    bne @while_body49
    bra @while_end50
@while_body49:
    .loc "test_func_identifier.c", 32
    inc.16f __vr15
    bra @while_cond48
@while_end50:
    .loc "test_func_identifier.c", 35
    lda _main__local_15
    ldx _main__local_15+1
    cmp.16 .AX, #11
    bne @if_then51
    bra @if_end53
@if_then51:
    lda #11
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end42
@if_end53:
    .loc "test_func_identifier.c", 37
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end42:
    .loc "test_func_identifier.c", 60
    lda $22
    ldx $23
    sta _main__local_0
    stx _main__local_0+1
    .loc "test_func_identifier.c", 61
    lda _main__local_0
    ldx _main__local_0+1
    stx __zp_scratch
    ora __zp_scratch
    bne @if_then56
    bra @if_end58
@if_then56:
    lda _main__local_0
    ldx _main__local_0+1
    bra @__return
@if_end58:
    .loc "test_func_identifier.c", 41
    ldax #__str_60
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_32
    stx _main__local_32+1
    .loc "test_func_identifier.c", 43
    lda #0
    sta $20
    sta $21
    lda _main__local_32
    ldx _main__local_32+1
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
    lda #0
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
    beq @if_then61
    bra @if_end63
@if_then61:
    lda #20
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end59
@if_end63:
    .loc "test_func_identifier.c", 44
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end59:
    .loc "test_func_identifier.c", 63
    lda $22
    ldx $23
    sta _main__local_0
    stx _main__local_0+1
    .loc "test_func_identifier.c", 64
    lda _main__local_0
    ldx _main__local_0+1
    stx __zp_scratch
    ora __zp_scratch
    bne @if_then66
    bra @if_end68
@if_then66:
    lda _main__local_0
    ldx _main__local_0+1
    bra @__return
@if_end68:
    .loc "test_func_identifier.c", 48
    lda #48
    sta _main__local_46
    lda #0
    sta _main__local_46+1
    .loc "test_func_identifier.c", 50
    lda _main__local_46
    ldx _main__local_46+1
    cmp.16 .AX, #1
    bcc @if_then70
    bra @if_end72
@if_then70:
    lda #30
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @inline_end69
@if_end72:
    .loc "test_func_identifier.c", 51
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end69:
    .loc "test_func_identifier.c", 66
    lda $22
    ldx $23
    sta _main__local_0
    stx _main__local_0+1
    .loc "test_func_identifier.c", 67
    lda _main__local_0
    ldx _main__local_0+1
    stx __zp_scratch
    ora __zp_scratch
    bne @if_then75
    bra @if_end77
@if_then75:
    lda _main__local_0
    ldx _main__local_0+1
    bra @__return
@if_end77:
    .loc "test_func_identifier.c", 69
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 10
    endproc


    .segment "data"
__str_0:
    .text "check_func_name"
    .byte 0
__str_22:
    .text "verify_main"
    .byte 0
__str_32:
    .text "test_func_identifier.c"
    .byte 0
__str_43:
    .text "verify_main"
    .byte 0
__str_60:
    .text "test_func_identifier.c"
    .byte 0

__zp_save_buf:

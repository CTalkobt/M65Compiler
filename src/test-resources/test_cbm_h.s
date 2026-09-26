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

    .global _ch
    .global _test_global_single_if
    .global _test_local_single_if
    .global _test_multiple_ifs
    .global _test_all_parameterized
    .global _test_no_arg
    .global _main

    .segment "bss"
_ch:
; .debug_var: @global _ch offset=0 size=2 type=int16 scope=global
    .res 2

    .segment "code"

; function _cbm_chrout
; SAC inline storage: 2 bytes
    .global _cbm_chrout__param_c
    _cbm_chrout__param_c: .word 0
    _cbm_chrout__local_0: .word 0
    proc _cbm_chrout, B#@_p_c
    .sac
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/cbm.h", 54
    .var @_p_c = 2
; .debug_var: __cbm_chrout @_p_c offset=2 size=2 type=int8 scope=parameter

@entry:
    .loc "test_cbm_h.c", 60
    lda.sp @_p_c
    .loc "test_cbm_h.c", 61
    jsr $FFD2
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _cbm_setlfs
; SAC inline storage: 6 bytes
    .global _cbm_setlfs__param_lfn
    _cbm_setlfs__param_lfn: .word 0
    .global _cbm_setlfs__param_device
    _cbm_setlfs__param_device: .word 0
    .global _cbm_setlfs__param_sa
    _cbm_setlfs__param_sa: .word 0
    _cbm_setlfs__local_0: .word 0
    _cbm_setlfs__local_1: .word 0
    _cbm_setlfs__local_2: .word 0
    proc _cbm_setlfs, B#@_p_lfn, B#@_p_device, B#@_p_sa
    .sac
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/cbm.h", 87
    .var @_p_lfn = 2
    .var @_p_device = 4
    .var @_p_sa = 6
; .debug_var: __cbm_setlfs @_p_lfn offset=2 size=2 type=int8 scope=parameter
; .debug_var: __cbm_setlfs @_p_device offset=4 size=2 type=int8 scope=parameter
; .debug_var: __cbm_setlfs @_p_sa offset=6 size=2 type=int8 scope=parameter

@entry:
    .loc "test_cbm_h.c", 93
    lda.sp @_p_sa
    .loc "test_cbm_h.c", 94
    tay
    .loc "test_cbm_h.c", 95
    lda.sp @_p_device
    .loc "test_cbm_h.c", 96
    tax
    .loc "test_cbm_h.c", 97
    lda.sp @_p_lfn
    .loc "test_cbm_h.c", 98
    jsr $FFBA
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc

; function _cbm_setnam
; SAC inline storage: 4 bytes
    .global _cbm_setnam__param_name
    _cbm_setnam__param_name: .word 0
    .global _cbm_setnam__param_len
    _cbm_setnam__param_len: .word 0
    _cbm_setnam__local_0: .word 0
    _cbm_setnam__local_1: .word 0
    proc _cbm_setnam, W#@_p_name, B#@_p_len
    .sac
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/cbm.h", 99
    .var @_p_name = 2
    .var @_p_len = 4
; .debug_var: __cbm_setnam @_p_name offset=2 size=2 type=ptr scope=parameter
; .debug_var: __cbm_setnam @_p_len offset=4 size=2 type=int8 scope=parameter

@entry:
    .loc "test_cbm_h.c", 105
    ldxy @_p_name, sp
    .loc "test_cbm_h.c", 106
    lda.sp @_p_len
    .loc "test_cbm_h.c", 107
    jsr $FFBD
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _cbm_close
; SAC inline storage: 2 bytes
    .global _cbm_close__param_lfn
    _cbm_close__param_lfn: .word 0
    _cbm_close__local_0: .word 0
    proc _cbm_close, B#@_p_lfn
    .sac
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/cbm.h", 117
    .var @_p_lfn = 2
; .debug_var: __cbm_close @_p_lfn offset=2 size=2 type=int8 scope=parameter

@entry:
    .loc "test_cbm_h.c", 123
    lda.sp @_p_lfn
    .loc "test_cbm_h.c", 124
    jsr $FFC3
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _cbm_chkin
; SAC inline storage: 2 bytes
    .global _cbm_chkin__param_lfn
    _cbm_chkin__param_lfn: .word 0
    _cbm_chkin__local_0: .word 0
    proc _cbm_chkin, B#@_p_lfn
    .sac
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/cbm.h", 123
    .var @_p_lfn = 2
; .debug_var: __cbm_chkin @_p_lfn offset=2 size=2 type=int8 scope=parameter

@entry:
    .loc "test_cbm_h.c", 129
    lda.sp @_p_lfn
    .loc "test_cbm_h.c", 130
    tax
    .loc "test_cbm_h.c", 131
    jsr $FFC6
    .loc "test_cbm_h.c", 132
    lda #$00
    .loc "test_cbm_h.c", 133
    tax
    .loc "test_cbm_h.c", 134
    adc #$00
    .loc "test_cbm_h.c", 135
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _cbm_chkout
; SAC inline storage: 2 bytes
    .global _cbm_chkout__param_lfn
    _cbm_chkout__param_lfn: .word 0
    _cbm_chkout__local_0: .word 0
    proc _cbm_chkout, B#@_p_lfn
    .sac
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/cbm.h", 134
    .var @_p_lfn = 2
; .debug_var: __cbm_chkout @_p_lfn offset=2 size=2 type=int8 scope=parameter

@entry:
    .loc "test_cbm_h.c", 140
    lda.sp @_p_lfn
    .loc "test_cbm_h.c", 141
    tax
    .loc "test_cbm_h.c", 142
    jsr $FFC9
    .loc "test_cbm_h.c", 143
    lda #$00
    .loc "test_cbm_h.c", 144
    tax
    .loc "test_cbm_h.c", 145
    adc #$00
    .loc "test_cbm_h.c", 146
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _cbm_gotoxy
; SAC inline storage: 4 bytes
    .global _cbm_gotoxy__param_col
    _cbm_gotoxy__param_col: .word 0
    .global _cbm_gotoxy__param_row
    _cbm_gotoxy__param_row: .word 0
    _cbm_gotoxy__local_0: .word 0
    _cbm_gotoxy__local_1: .word 0
    proc _cbm_gotoxy, B#@_p_col, B#@_p_row
    .sac
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/cbm.h", 174
    .var @_p_col = 2
    .var @_p_row = 4
; .debug_var: __cbm_gotoxy @_p_col offset=2 size=2 type=int8 scope=parameter
; .debug_var: __cbm_gotoxy @_p_row offset=4 size=2 type=int8 scope=parameter

@entry:
    .loc "test_cbm_h.c", 180
    clc
    .loc "test_cbm_h.c", 181
    lda.sp @_p_row
    .loc "test_cbm_h.c", 182
    tax
    .loc "test_cbm_h.c", 183
    lda.sp @_p_col
    .loc "test_cbm_h.c", 184
    tay
    .loc "test_cbm_h.c", 185
    jsr $FFF0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _cbm_listen
; SAC inline storage: 2 bytes
    .global _cbm_listen__param_device
    _cbm_listen__param_device: .word 0
    _cbm_listen__local_0: .word 0
    proc _cbm_listen, B#@_p_device
    .sac
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/cbm.h", 223
    .var @_p_device = 2
; .debug_var: __cbm_listen @_p_device offset=2 size=2 type=int8 scope=parameter

@entry:
    .loc "test_cbm_h.c", 229
    lda.sp @_p_device
    .loc "test_cbm_h.c", 230
    jsr $FFB1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _cbm_talk
; SAC inline storage: 2 bytes
    .global _cbm_talk__param_device
    _cbm_talk__param_device: .word 0
    _cbm_talk__local_0: .word 0
    proc _cbm_talk, B#@_p_device
    .sac
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/cbm.h", 234
    .var @_p_device = 2
; .debug_var: __cbm_talk @_p_device offset=2 size=2 type=int8 scope=parameter

@entry:
    .loc "test_cbm_h.c", 240
    lda.sp @_p_device
    .loc "test_cbm_h.c", 241
    jsr $FFB4
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _cbm_second
; SAC inline storage: 2 bytes
    .global _cbm_second__param_sa
    _cbm_second__param_sa: .word 0
    _cbm_second__local_0: .word 0
    proc _cbm_second, B#@_p_sa
    .sac
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/cbm.h", 245
    .var @_p_sa = 2
; .debug_var: __cbm_second @_p_sa offset=2 size=2 type=int8 scope=parameter

@entry:
    .loc "test_cbm_h.c", 251
    lda.sp @_p_sa
    .loc "test_cbm_h.c", 252
    jsr $FF93
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _cbm_tksa
; SAC inline storage: 2 bytes
    .global _cbm_tksa__param_sa
    _cbm_tksa__param_sa: .word 0
    _cbm_tksa__local_0: .word 0
    proc _cbm_tksa, B#@_p_sa
    .sac
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/cbm.h", 251
    .var @_p_sa = 2
; .debug_var: __cbm_tksa @_p_sa offset=2 size=2 type=int8 scope=parameter

@entry:
    .loc "test_cbm_h.c", 257
    lda.sp @_p_sa
    .loc "test_cbm_h.c", 258
    jsr $FF96
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _cbm_ciout
; SAC inline storage: 2 bytes
    .global _cbm_ciout__param_data
    _cbm_ciout__param_data: .word 0
    _cbm_ciout__local_0: .word 0
    proc _cbm_ciout, B#@_p_data
    .sac
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/cbm.h", 257
    .var @_p_data = 2
; .debug_var: __cbm_ciout @_p_data offset=2 size=2 type=int8 scope=parameter

@entry:
    .loc "test_cbm_h.c", 263
    lda.sp @_p_data
    .loc "test_cbm_h.c", 264
    jsr $FFA8
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _test_global_single_if
; SAC inline storage: 0 bytes
    _test_global_single_if__local_5: .word 0
    proc _test_global_single_if
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/cbm.h", 6

@entry:
    .loc "test_cbm_h.c", 73
    jsr $FFE4
    .loc "test_cbm_h.c", 74
    ldx #$00
@inline_end0:
    .loc "test_cbm_h.c", 12
    lda $20
    ldx $21
    sta _ch
    stx _ch+1
    .loc "test_cbm_h.c", 13
    lda _ch
    ldx _ch+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #81
    beq @if_then2
    bra @if_end4
@if_then2:
    .loc "test_cbm_h.c", 14
    lda _ch
    ldx _ch+1
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _test_global_single_if__local_5
    lda _test_global_single_if__local_5
    sta _cbm_chrout__param_c
    stx _cbm_chrout__param_c+1
    jsr _cbm_chrout
@if_end4:
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 0
    endproc

; function _test_local_single_if
; SAC inline storage: 2 bytes
    _test_local_single_if__local_0: .word 0
    _test_local_single_if__local_4: .word 0
    proc _test_local_single_if
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/cbm.h", 14
    .local @_l_ch = 0
; .debug_var: __test_local_single_if @_l_ch offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_cbm_h.c", 73
    jsr $FFE4
    .loc "test_cbm_h.c", 74
    ldx #$00
@inline_end5:
    .loc "test_cbm_h.c", 20
    lda $20
    ldx $21
    sta _test_local_single_if__local_0
    stx _test_local_single_if__local_0+1
    .loc "test_cbm_h.c", 21
    lda _test_local_single_if__local_0
    ldx _test_local_single_if__local_0+1
    cmp.16 .AX, #81
    beq @if_then7
    bra @if_end9
@if_then7:
    lda _test_local_single_if__local_0
    ldx _test_local_single_if__local_0+1
    sta _test_local_single_if__local_4
    .loc "test_cbm_h.c", 22
    lda _test_local_single_if__local_4
    sta _cbm_chrout__param_c
    stx _cbm_chrout__param_c+1
    jsr _cbm_chrout
@if_end9:
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _test_multiple_ifs
; SAC inline storage: 0 bytes
    _test_multiple_ifs__local_0: .word 0
    _test_multiple_ifs__local_3: .word 0
    _test_multiple_ifs__local_6: .word 0
    proc _test_multiple_ifs
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/cbm.h", 22

@entry:
    .loc "test_cbm_h.c", 73
    jsr $FFE4
    .loc "test_cbm_h.c", 74
    ldx #$00
@inline_end10:
    .loc "test_cbm_h.c", 28
    lda _test_multiple_ifs__local_0
    ldx _test_multiple_ifs__local_0+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _test_multiple_ifs__local_0
    ldx _test_multiple_ifs__local_0+1
    ldy #0
    sta (__zp_scratch2),y
    txa
    iny
    sta (__zp_scratch2),y
    .loc "test_cbm_h.c", 29
    lda _test_multiple_ifs__local_0
    ldx _test_multiple_ifs__local_0+1
    cmp.16 .AX, #81
    beq @if_then12
    bra @if_end14
@if_then12:
    lda _test_multiple_ifs__local_0
    ldx _test_multiple_ifs__local_0+1
    sta _test_multiple_ifs__local_3
    .loc "test_cbm_h.c", 30
    lda _test_multiple_ifs__local_3
    sta _cbm_chrout__param_c
    stx _cbm_chrout__param_c+1
    jsr _cbm_chrout
@if_end14:
    .loc "test_cbm_h.c", 32
    lda _test_multiple_ifs__local_0
    ldx _test_multiple_ifs__local_0+1
    cmp.16 .AX, #87
    beq @if_then15
    bra @if_end17
@if_then15:
    lda _test_multiple_ifs__local_0
    ldx _test_multiple_ifs__local_0+1
    sta _test_multiple_ifs__local_6
    .loc "test_cbm_h.c", 33
    lda _test_multiple_ifs__local_6
    sta _cbm_chrout__param_c
    stx _cbm_chrout__param_c+1
    jsr _cbm_chrout
@if_end17:
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 0
    endproc

; function _test_all_parameterized
; SAC inline storage: 0 bytes
    _test_all_parameterized__local_1: .word 0
    _test_all_parameterized__local_5: .word 0
    _test_all_parameterized__local_6: .word 0
    _test_all_parameterized__local_7: .word 0
    _test_all_parameterized__local_8: .word 0
    _test_all_parameterized__local_10: .word 0
    _test_all_parameterized__local_12: .word 0
    _test_all_parameterized__local_14: .word 0
    _test_all_parameterized__local_17: .word 0
    _test_all_parameterized__local_21: .word 0
    _test_all_parameterized__local_22: .word 0
    _test_all_parameterized__local_24: .word 0
    _test_all_parameterized__local_26: .word 0
    _test_all_parameterized__local_28: .word 0
    _test_all_parameterized__local_30: .word 0
    _test_all_parameterized__local_32: .word 0
    proc _test_all_parameterized
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/cbm.h", 33

@entry:
    .loc "test_cbm_h.c", 39
    lda #97
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _test_all_parameterized__local_1
    lda _test_all_parameterized__local_1
    sta _cbm_chrout__param_c
    stx _cbm_chrout__param_c+1
    jsr _cbm_chrout
    .loc "test_cbm_h.c", 40
    lda #1
    ldx #0
    sta $20
    stx $21
    lda #8
    ldx #0
    sta $22
    stx $23
    lda #15
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx $21
    sta _test_all_parameterized__local_5
    lda $22
    ldx $23
    sta _test_all_parameterized__local_6
    lda $24
    ldx $25
    sta _test_all_parameterized__local_7
    lda _test_all_parameterized__local_7
    sta _cbm_setlfs__param_sa
    stx _cbm_setlfs__param_sa+1
    lda _test_all_parameterized__local_6
    sta _cbm_setlfs__param_device
    stx _cbm_setlfs__param_device+1
    lda _test_all_parameterized__local_5
    sta _cbm_setlfs__param_lfn
    stx _cbm_setlfs__param_lfn+1
    jsr _cbm_setlfs
    .loc "test_cbm_h.c", 41
    ldax #__str_18
    sta _test_all_parameterized__local_8
    stx _test_all_parameterized__local_8+1
    lda #4
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _test_all_parameterized__local_10
    lda _test_all_parameterized__local_10
    sta _cbm_setnam__param_len
    stx _cbm_setnam__param_len+1
    lda _test_all_parameterized__local_8
    ldx _test_all_parameterized__local_8+1
    sta _cbm_setnam__param_name
    stx _cbm_setnam__param_name+1
    jsr _cbm_setnam
    .loc "test_cbm_h.c", 42
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _test_all_parameterized__local_12
    lda _test_all_parameterized__local_12
    sta _cbm_close__param_lfn
    stx _cbm_close__param_lfn+1
    jsr _cbm_close
    .loc "test_cbm_h.c", 43
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _test_all_parameterized__local_14
    lda _test_all_parameterized__local_14
    sta _cbm_chkin__param_lfn
    stx _cbm_chkin__param_lfn+1
    jsr _cbm_chkin
    .loc "test_cbm_h.c", 44
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _test_all_parameterized__local_17
    lda _test_all_parameterized__local_17
    sta _cbm_chkout__param_lfn
    stx _cbm_chkout__param_lfn+1
    jsr _cbm_chkout
    .loc "test_cbm_h.c", 45
    lda #0
    sta $20
    sta $21
    lda #0
    sta $22
    sta $23
    lda $20
    ldx $21
    sta _test_all_parameterized__local_21
    lda $22
    ldx $23
    sta _test_all_parameterized__local_22
    lda _test_all_parameterized__local_22
    sta _cbm_gotoxy__param_row
    stx _cbm_gotoxy__param_row+1
    lda _test_all_parameterized__local_21
    sta _cbm_gotoxy__param_col
    stx _cbm_gotoxy__param_col+1
    jsr _cbm_gotoxy
    .loc "test_cbm_h.c", 46
    lda #8
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _test_all_parameterized__local_24
    lda _test_all_parameterized__local_24
    sta _cbm_listen__param_device
    stx _cbm_listen__param_device+1
    jsr _cbm_listen
    .loc "test_cbm_h.c", 47
    lda #8
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _test_all_parameterized__local_26
    lda _test_all_parameterized__local_26
    sta _cbm_talk__param_device
    stx _cbm_talk__param_device+1
    jsr _cbm_talk
    .loc "test_cbm_h.c", 48
    lda #15
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _test_all_parameterized__local_28
    lda _test_all_parameterized__local_28
    sta _cbm_second__param_sa
    stx _cbm_second__param_sa+1
    jsr _cbm_second
    .loc "test_cbm_h.c", 49
    lda #15
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _test_all_parameterized__local_30
    lda _test_all_parameterized__local_30
    sta _cbm_tksa__param_sa
    stx _cbm_tksa__param_sa+1
    jsr _cbm_tksa
    .loc "test_cbm_h.c", 50
    lda #65
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _test_all_parameterized__local_32
    lda _test_all_parameterized__local_32
    sta _cbm_ciout__param_data
    stx _cbm_ciout__param_data+1
    jsr _cbm_ciout
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 0
    endproc

; function _test_no_arg
; SAC inline storage: 0 bytes
    _test_no_arg__local_0: .word 0
    _test_no_arg__local_1: .word 0
    _test_no_arg__local_2: .word 0
    _test_no_arg__local_3: .word 0
    _test_no_arg__local_4: .word 0
    _test_no_arg__local_5: .word 0
    _test_no_arg__local_6: .word 0
    _test_no_arg__local_7: .word 0
    _test_no_arg__local_8: .word 0
    proc _test_no_arg
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/cbm.h", 49

@entry:
    .loc "test_cbm_h.c", 73
    jsr $FFE4
    .loc "test_cbm_h.c", 74
    ldx #$00
@inline_end19:
    .loc "test_cbm_h.c", 66
    jsr $FFCF
    .loc "test_cbm_h.c", 67
    ldx #$00
@inline_end21:
    .loc "test_cbm_h.c", 82
    jsr $FFB7
    .loc "test_cbm_h.c", 83
    ldx #$00
@inline_end23:
    .loc "test_cbm_h.c", 114
    jsr $FFC0
    .loc "test_cbm_h.c", 115
    lda #$00
    .loc "test_cbm_h.c", 116
    tax
    .loc "test_cbm_h.c", 117
    adc #$00
@inline_end25:
    .loc "test_cbm_h.c", 151
    jsr $FFCC
@inline_end27:
    .loc "test_cbm_h.c", 156
    jsr $FFE7
@inline_end28:
    .loc "test_cbm_h.c", 163
    jsr $FFE1
    .loc "test_cbm_h.c", 165
    php
    .loc "test_cbm_h.c", 166
    ldx #$00
    .loc "test_cbm_h.c", 167
    plp
    .loc "test_cbm_h.c", 168
    bne *+3
    .loc "test_cbm_h.c", 169
    inx
    .loc "test_cbm_h.c", 170
    txa
@inline_end29:
    .loc "test_cbm_h.c", 190
    sec
    .loc "test_cbm_h.c", 191
    jsr $FFF0
    .loc "test_cbm_h.c", 193
    tya
@inline_end31:
    .loc "test_cbm_h.c", 202
    jsr $FFED
    .loc "test_cbm_h.c", 204
    txa
    .loc "test_cbm_h.c", 205
    taz
    .loc "test_cbm_h.c", 206
    tya
    .loc "test_cbm_h.c", 207
    tax
    .loc "test_cbm_h.c", 208
    tza
@inline_end33:
    .loc "test_cbm_h.c", 218
    jsr $FFDE
    .loc "test_cbm_h.c", 220
    tya
@inline_end35:
    .loc "test_cbm_h.c", 235
    jsr $FFAE
@inline_end37:
    .loc "test_cbm_h.c", 246
    jsr $FFAB
@inline_end38:
    .loc "test_cbm_h.c", 269
    jsr $FF13
    .loc "test_cbm_h.c", 270
    ldx #$00
@inline_end39:
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 0
    endproc

; function _main
; SAC inline storage: 2 bytes
    _main__local_0: .word 0
    _main__local_3: .word 0
    _main__local_4: .word 0
    _main__local_8: .word 0
    _main__local_12: .word 0
    _main__local_15: .word 0
    _main__local_17: .word 0
    _main__local_21: .word 0
    _main__local_22: .word 0
    _main__local_23: .word 0
    _main__local_24: .word 0
    _main__local_26: .word 0
    _main__local_28: .word 0
    _main__local_30: .word 0
    _main__local_33: .word 0
    _main__local_37: .word 0
    _main__local_38: .word 0
    _main__local_40: .word 0
    _main__local_42: .word 0
    _main__local_44: .word 0
    _main__local_46: .word 0
    _main__local_48: .word 0
    _main__local_49: .word 0
    _main__local_50: .word 0
    _main__local_51: .word 0
    _main__local_52: .word 0
    _main__local_53: .word 0
    _main__local_54: .word 0
    _main__local_55: .word 0
    _main__local_56: .word 0
    _main__local_57: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/cbm.h", 65
    .local @_l_ch = 0
; .debug_var: __main @_l_ch offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_cbm_h.c", 73
    jsr $FFE4
    .loc "test_cbm_h.c", 74
    ldx #$00
@inline_end42:
    .loc "test_cbm_h.c", 12
    lda _main__local_0
    ldx _main__local_0+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_0
    ldx _main__local_0+1
    ldy #0
    sta (__zp_scratch2),y
    txa
    iny
    sta (__zp_scratch2),y
    .loc "test_cbm_h.c", 13
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, #81
    beq @if_then44
    bra @if_end46
@if_then44:
    lda _main__local_0
    ldx _main__local_0+1
    sta _main__local_3
    .loc "test_cbm_h.c", 14
    lda _main__local_3
    sta _cbm_chrout__param_c
    stx _cbm_chrout__param_c+1
    jsr _cbm_chrout
@if_end46:
@inline_end41:
    .loc "test_cbm_h.c", 73
    jsr $FFE4
    .loc "test_cbm_h.c", 74
    ldx #$00
@inline_end48:
    .loc "test_cbm_h.c", 20
    lda $20
    ldx $21
    sta _main__local_4
    stx _main__local_4+1
    .loc "test_cbm_h.c", 21
    lda _main__local_4
    ldx _main__local_4+1
    cmp.16 .AX, #81
    beq @if_then50
    bra @if_end52
@if_then50:
    lda _main__local_4
    ldx _main__local_4+1
    sta _main__local_8
    .loc "test_cbm_h.c", 22
    lda _main__local_8
    sta _cbm_chrout__param_c
    stx _cbm_chrout__param_c+1
    jsr _cbm_chrout
@if_end52:
@inline_end47:
    .loc "test_cbm_h.c", 73
    jsr $FFE4
    .loc "test_cbm_h.c", 74
    ldx #$00
@inline_end54:
    .loc "test_cbm_h.c", 28
    lda _main__local_0
    ldx _main__local_0+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx $21
    ldy #0
    sta (__zp_scratch2),y
    txa
    iny
    sta (__zp_scratch2),y
    .loc "test_cbm_h.c", 29
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, #81
    beq @if_then56
    bra @if_end58
@if_then56:
    lda _main__local_0
    ldx _main__local_0+1
    sta _main__local_12
    .loc "test_cbm_h.c", 30
    lda _main__local_12
    sta _cbm_chrout__param_c
    stx _cbm_chrout__param_c+1
    jsr _cbm_chrout
@if_end58:
    .loc "test_cbm_h.c", 32
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, #87
    beq @if_then59
    bra @if_end61
@if_then59:
    lda _main__local_0
    ldx _main__local_0+1
    sta _main__local_15
    .loc "test_cbm_h.c", 33
    lda _main__local_15
    sta _cbm_chrout__param_c
    stx _cbm_chrout__param_c+1
    jsr _cbm_chrout
@if_end61:
@inline_end53:
    .loc "test_cbm_h.c", 39
    lda #97
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_17
    lda _main__local_17
    sta _cbm_chrout__param_c
    stx _cbm_chrout__param_c+1
    jsr _cbm_chrout
    .loc "test_cbm_h.c", 40
    lda #1
    ldx #0
    sta $20
    stx $21
    lda #8
    ldx #0
    sta $22
    stx $23
    lda #15
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx $21
    sta _main__local_21
    lda $22
    ldx $23
    sta _main__local_22
    lda $24
    ldx $25
    sta _main__local_23
    lda _main__local_23
    sta _cbm_setlfs__param_sa
    stx _cbm_setlfs__param_sa+1
    lda _main__local_22
    sta _cbm_setlfs__param_device
    stx _cbm_setlfs__param_device+1
    lda _main__local_21
    sta _cbm_setlfs__param_lfn
    stx _cbm_setlfs__param_lfn+1
    jsr _cbm_setlfs
    .loc "test_cbm_h.c", 41
    ldax #__str_63
    sta _main__local_24
    stx _main__local_24+1
    lda #4
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_26
    lda _main__local_26
    sta _cbm_setnam__param_len
    stx _cbm_setnam__param_len+1
    lda _main__local_24
    ldx _main__local_24+1
    sta _cbm_setnam__param_name
    stx _cbm_setnam__param_name+1
    jsr _cbm_setnam
    .loc "test_cbm_h.c", 42
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_28
    lda _main__local_28
    sta _cbm_close__param_lfn
    stx _cbm_close__param_lfn+1
    jsr _cbm_close
    .loc "test_cbm_h.c", 43
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_30
    lda _main__local_30
    sta _cbm_chkin__param_lfn
    stx _cbm_chkin__param_lfn+1
    jsr _cbm_chkin
    .loc "test_cbm_h.c", 44
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_33
    lda _main__local_33
    sta _cbm_chkout__param_lfn
    stx _cbm_chkout__param_lfn+1
    jsr _cbm_chkout
    .loc "test_cbm_h.c", 45
    lda #0
    sta $20
    sta $21
    lda #0
    sta $22
    sta $23
    lda $20
    ldx $21
    sta _main__local_37
    lda $22
    ldx $23
    sta _main__local_38
    lda _main__local_38
    sta _cbm_gotoxy__param_row
    stx _cbm_gotoxy__param_row+1
    lda _main__local_37
    sta _cbm_gotoxy__param_col
    stx _cbm_gotoxy__param_col+1
    jsr _cbm_gotoxy
    .loc "test_cbm_h.c", 46
    lda #8
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_40
    lda _main__local_40
    sta _cbm_listen__param_device
    stx _cbm_listen__param_device+1
    jsr _cbm_listen
    .loc "test_cbm_h.c", 47
    lda #8
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_42
    lda _main__local_42
    sta _cbm_talk__param_device
    stx _cbm_talk__param_device+1
    jsr _cbm_talk
    .loc "test_cbm_h.c", 48
    lda #15
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_44
    lda _main__local_44
    sta _cbm_second__param_sa
    stx _cbm_second__param_sa+1
    jsr _cbm_second
    .loc "test_cbm_h.c", 49
    lda #15
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_46
    lda _main__local_46
    sta _cbm_tksa__param_sa
    stx _cbm_tksa__param_sa+1
    jsr _cbm_tksa
    .loc "test_cbm_h.c", 50
    lda #65
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_48
    lda _main__local_48
    sta _cbm_ciout__param_data
    stx _cbm_ciout__param_data+1
    jsr _cbm_ciout
@inline_end62:
    .loc "test_cbm_h.c", 73
    jsr $FFE4
    .loc "test_cbm_h.c", 74
    ldx #$00
@inline_end65:
    .loc "test_cbm_h.c", 66
    jsr $FFCF
    .loc "test_cbm_h.c", 67
    ldx #$00
@inline_end67:
    .loc "test_cbm_h.c", 82
    jsr $FFB7
    .loc "test_cbm_h.c", 83
    ldx #$00
@inline_end69:
    .loc "test_cbm_h.c", 114
    jsr $FFC0
    .loc "test_cbm_h.c", 115
    lda #$00
    .loc "test_cbm_h.c", 116
    tax
    .loc "test_cbm_h.c", 117
    adc #$00
@inline_end71:
    .loc "test_cbm_h.c", 151
    jsr $FFCC
@inline_end73:
    .loc "test_cbm_h.c", 156
    jsr $FFE7
@inline_end74:
    .loc "test_cbm_h.c", 163
    jsr $FFE1
    .loc "test_cbm_h.c", 165
    php
    .loc "test_cbm_h.c", 166
    ldx #$00
    .loc "test_cbm_h.c", 167
    plp
    .loc "test_cbm_h.c", 168
    bne *+3
    .loc "test_cbm_h.c", 169
    inx
    .loc "test_cbm_h.c", 170
    txa
@inline_end75:
    .loc "test_cbm_h.c", 190
    sec
    .loc "test_cbm_h.c", 191
    jsr $FFF0
    .loc "test_cbm_h.c", 193
    tya
@inline_end77:
    .loc "test_cbm_h.c", 202
    jsr $FFED
    .loc "test_cbm_h.c", 204
    txa
    .loc "test_cbm_h.c", 205
    taz
    .loc "test_cbm_h.c", 206
    tya
    .loc "test_cbm_h.c", 207
    tax
    .loc "test_cbm_h.c", 208
    tza
@inline_end79:
    .loc "test_cbm_h.c", 218
    jsr $FFDE
    .loc "test_cbm_h.c", 220
    tya
@inline_end81:
    .loc "test_cbm_h.c", 235
    jsr $FFAE
@inline_end83:
    .loc "test_cbm_h.c", 246
    jsr $FFAB
@inline_end84:
    .loc "test_cbm_h.c", 269
    jsr $FF13
    .loc "test_cbm_h.c", 270
    ldx #$00
@inline_end85:
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc


    .segment "data"
__str_18:
    .text "TEST"
    .byte 0
__str_63:
    .text "TEST"
    .byte 0

__zp_save_buf:

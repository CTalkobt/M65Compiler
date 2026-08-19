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

    .global _opaque
    .global _main

    .segment "code"

; function _opaque
; SAC inline storage: 2 bytes
    .global _opaque__param_val
    _opaque__param_val: .word 0
    _opaque__local_0: .word 0
    proc _opaque, W#@_p_val
    .sac
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdbool.h", 4
    .var @_p_val = 2
; .debug_var: __opaque @_p_val offset=2 size=2 type=int16 scope=parameter

@entry:
    .loc "test_bool.c", 5
    lda _opaque__param_val
    ldx _opaque__param_val+1
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers N, Z
    .frame_size 2
    endproc

; function _main
; SAC inline storage: 50 bytes
    _main__local_0: .word 0
    _main__local_4: .word 0
    _main__local_8: .word 0
    _main__local_12: .word 0
    _main__local_16: .word 0
    _main__local_20: .word 0
    _main__local_24: .word 0
    _main__local_26: .word 0
    _main__local_30: .word 0
    _main__local_33: .word 0
    _main__local_35: .word 0
    _main__local_37: .word 0
    _main__local_41: .word 0
    _main__local_49: .word 0
    _main__local_56: .word 0
    _main__local_58: .word 0
    _main__local_64: .word 0
    _main__local_66: .word 0
    _main__local_72: .word 0
    _main__local_76: .word 0
    _main__local_80: .word 0
    _main__local_84: .word 0
    _main__local_95: .word 0
    _main__local_99: .word 0
    _main__local_107: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdbool.h", 8
    .local @_l_a = 0
    .local @_l_b = 2
    .local @_l_big = 28
    .local @_l_c = 4
    .local @_l_cb = 30
    .local @_l_cmp = 26
    .local @_l_d = 6
    .local @_l_e = 8
    .local @_l_f = 10
    .local @_l_flag = 32
    .local @_l_g = 14
    .local @_l_h = 16
    .local @_l_land = 36
    .local @_l_lnot = 40
    .local @_l_lor = 38
    .local @_l_r = 34
    .local @_l_rb = 42
    .local @_l_rc = 46
    .local @_l_rd = 48
    .local @_l_rv = 44
    .local @_l_sum = 22
    .local @_l_t = 18
    .local @_l_u = 20
    .local @_l_v = 24
    .local @_l_x = 12
; .debug_var: __main @_l_a offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_b offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_big offset=28 size=2 type=int16 scope=local
; .debug_var: __main @_l_c offset=4 size=2 type=int16 scope=local
; .debug_var: __main @_l_cb offset=30 size=2 type=int16 scope=local
; .debug_var: __main @_l_cmp offset=26 size=2 type=int16 scope=local
; .debug_var: __main @_l_d offset=6 size=2 type=int16 scope=local
; .debug_var: __main @_l_e offset=8 size=2 type=int16 scope=local
; .debug_var: __main @_l_f offset=10 size=2 type=int16 scope=local
; .debug_var: __main @_l_flag offset=32 size=2 type=int16 scope=local
; .debug_var: __main @_l_g offset=14 size=2 type=int16 scope=local
; .debug_var: __main @_l_h offset=16 size=2 type=int16 scope=local
; .debug_var: __main @_l_land offset=36 size=2 type=int16 scope=local
; .debug_var: __main @_l_lnot offset=40 size=2 type=int16 scope=local
; .debug_var: __main @_l_lor offset=38 size=2 type=int16 scope=local
; .debug_var: __main @_l_r offset=34 size=2 type=int16 scope=local
; .debug_var: __main @_l_rb offset=42 size=2 type=int16 scope=local
; .debug_var: __main @_l_rc offset=46 size=2 type=int16 scope=local
; .debug_var: __main @_l_rd offset=48 size=2 type=int16 scope=local
; .debug_var: __main @_l_rv offset=44 size=2 type=int16 scope=local
; .debug_var: __main @_l_sum offset=22 size=2 type=int16 scope=local
; .debug_var: __main @_l_t offset=18 size=2 type=int16 scope=local
; .debug_var: __main @_l_u offset=20 size=2 type=int16 scope=local
; .debug_var: __main @_l_v offset=24 size=2 type=int16 scope=local
; .debug_var: __main @_l_x offset=12 size=2 type=int16 scope=local

@entry:
    .loc "test_bool.c", 10
    lda #1
    sta _main__local_0
    lda #0
    sta _main__local_0+1
    .loc "test_bool.c", 11
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, #1
    bne @if_then0
    bra @if_end2
@if_then0:
    lda #1
    ldx #0
    bra @__return
@if_end2:
    .loc "test_bool.c", 13
    lda #0
    sta _main__local_4
    sta _main__local_4+1
    .loc "test_bool.c", 14
    lda _main__local_4
    ldx _main__local_4+1
    stx __zp_scratch
    ora __zp_scratch
    bne @if_then3
    bra @if_end5
@if_then3:
    lda #2
    ldx #0
    bra @__return
@if_end5:
    .loc "test_bool.c", 17
    lda #1
    sta _main__local_8
    lda #0
    sta _main__local_8+1
    .loc "test_bool.c", 18
    lda _main__local_8
    ldx _main__local_8+1
    cmp.16 .AX, #1
    bne @if_then6
    bra @if_end8
@if_then6:
    lda #3
    ldx #0
    bra @__return
@if_end8:
    .loc "test_bool.c", 20
    lda #1
    sta _main__local_12
    lda #0
    sta _main__local_12+1
    .loc "test_bool.c", 21
    lda _main__local_12
    ldx _main__local_12+1
    cmp.16 .AX, #1
    bne @if_then9
    bra @if_end11
@if_then9:
    lda #4
    ldx #0
    bra @__return
@if_end11:
    .loc "test_bool.c", 24
    lda #1
    sta _main__local_16
    lda #0
    sta _main__local_16+1
    .loc "test_bool.c", 25
    lda _main__local_16
    ldx _main__local_16+1
    cmp.16 .AX, #1
    bne @if_then12
    bra @if_end14
@if_then12:
    lda #5
    ldx #0
    bra @__return
@if_end14:
    .loc "test_bool.c", 27
    lda #0
    sta _main__local_20
    sta _main__local_20+1
    .loc "test_bool.c", 28
    lda _main__local_20
    ldx _main__local_20+1
    stx __zp_scratch
    ora __zp_scratch
    bne @if_then15
    bra @if_end17
@if_then15:
    lda #6
    ldx #0
    bra @__return
@if_end17:
    .loc "test_bool.c", 31
    lda #100
    sta _main__local_24
    lda #0
    sta _main__local_24+1
    .loc "test_bool.c", 32
    lda _main__local_24
    ldx _main__local_24+1
    sta _main__local_26
    stx _main__local_26+1
    .loc "test_bool.c", 33
    lda _main__local_26
    ldx _main__local_26+1
    cmp.16 .AX, #1
    bne @if_then18
    bra @if_end20
@if_then18:
    lda #7
    ldx #0
    bra @__return
@if_end20:
    .loc "test_bool.c", 35
    lda #0
    sta _main__local_24
    sta _main__local_24+1
    .loc "test_bool.c", 36
    lda _main__local_24
    ldx _main__local_24+1
    sta _main__local_30
    stx _main__local_30+1
    .loc "test_bool.c", 37
    lda _main__local_30
    ldx _main__local_30+1
    stx __zp_scratch
    ora __zp_scratch
    bne @if_then21
    bra @if_end23
@if_then21:
    lda #8
    ldx #0
    bra @__return
@if_end23:
    .loc "test_bool.c", 44
    lda #1
    sta _main__local_33
    lda #0
    sta _main__local_33+1
    .loc "test_bool.c", 45
    lda #1
    sta _main__local_35
    lda #0
    sta _main__local_35+1
    .loc "test_bool.c", 46
    lda _main__local_35
    ldx _main__local_35+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _main__local_33
    ldx _main__local_33+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    sta _main__local_37
    stx _main__local_37+1
    .loc "test_bool.c", 47
    lda _main__local_37
    ldx _main__local_37+1
    cmp.16 .AX, #2
    bne @if_then24
    bra @if_end26
@if_then24:
    lda #11
    ldx #0
    bra @__return
@if_end26:
    .loc "test_bool.c", 50
    lda #0
    sta _main__local_41
    sta _main__local_41+1
    .loc "test_bool.c", 51
    lda #1
    sta _main__local_41
    lda #0
    sta _main__local_41+1
    .loc "test_bool.c", 52
    lda _main__local_41
    ldx _main__local_41+1
    cmp.16 .AX, #1
    bne @if_then27
    bra @if_end29
@if_then27:
    lda #12
    ldx #0
    bra @__return
@if_end29:
    .loc "test_bool.c", 53
    lda #0
    sta _main__local_41
    sta _main__local_41+1
    .loc "test_bool.c", 54
    lda _main__local_41
    ldx _main__local_41+1
    stx __zp_scratch
    ora __zp_scratch
    bne @if_then30
    bra @if_end32
@if_then30:
    lda #13
    ldx #0
    bra @__return
@if_end32:
    .loc "test_bool.c", 57
    lda #1
    sta _main__local_49
    lda #0
    sta _main__local_49+1
    .loc "test_bool.c", 58
    lda _main__local_49
    ldx _main__local_49+1
    cmp.16 .AX, #1
    bne @if_then33
    bra @if_end35
@if_then33:
    lda #14
    ldx #0
    bra @__return
@if_end35:
    .loc "test_bool.c", 59
    lda #0
    sta _main__local_49
    sta _main__local_49+1
    .loc "test_bool.c", 60
    lda _main__local_49
    ldx _main__local_49+1
    stx __zp_scratch
    ora __zp_scratch
    bne @if_then36
    bra @if_end38
@if_then36:
    lda #15
    ldx #0
    bra @__return
@if_end38:
    .loc "test_bool.c", 63
    lda #0
    sta _main__local_56
    lda #2
    sta _main__local_56+1
    .loc "test_bool.c", 64
    lda _main__local_56
    ldx _main__local_56+1
    sta _main__local_58
    stx _main__local_58+1
    .loc "test_bool.c", 65
    lda _main__local_58
    ldx _main__local_58+1
    cmp.16 .AX, #1
    bne @if_then39
    bra @if_end41
@if_then39:
    lda #16
    ldx #0
    bra @__return
@if_end41:
    .loc "test_bool.c", 66
    lda #0
    sta _main__local_58
    sta _main__local_58+1
    .loc "test_bool.c", 67
    lda _main__local_58
    ldx _main__local_58+1
    stx __zp_scratch
    ora __zp_scratch
    bne @if_then42
    bra @if_end44
@if_then42:
    lda #17
    ldx #0
    bra @__return
@if_end44:
    .loc "test_bool.c", 70
    lda #1
    sta _main__local_64
    lda #0
    sta _main__local_64+1
    .loc "test_bool.c", 71
    lda #0
    sta _main__local_66
    sta _main__local_66+1
    .loc "test_bool.c", 72
    lda _main__local_64
    ldx _main__local_64+1
    bne @if_then45
    cmp #$00
    bne @if_then45
    bra @if_end47
@if_then45:
    lda #1
    sta _main__local_66
    lda #0
    sta _main__local_66+1
@if_end47:
    .loc "test_bool.c", 73
    lda _main__local_66
    ldx _main__local_66+1
    cmp.16 .AX, #1
    bne @if_then48
    bra @if_end50
@if_then48:
    lda #18
    ldx #0
    bra @__return
@if_end50:
    .loc "test_bool.c", 75
    lda #0
    sta _main__local_64
    sta _main__local_64+1
    .loc "test_bool.c", 76
    lda _main__local_64
    ldx _main__local_64+1
    bne @if_then51
    cmp #$00
    bne @if_then51
    bra @if_end53
@if_then51:
    lda #19
    ldx #0
    bra @__return
@if_end53:
    .loc "test_bool.c", 79
    lda #1
    sta _main__local_72
    lda #0
    sta _main__local_72+1
    .loc "test_bool.c", 80
    lda _main__local_72
    ldx _main__local_72+1
    cmp.16 .AX, #1
    bne @if_then54
    bra @if_end56
@if_then54:
    lda #20
    ldx #0
    bra @__return
@if_end56:
    .loc "test_bool.c", 81
    lda #0
    sta _main__local_76
    sta _main__local_76+1
    .loc "test_bool.c", 82
    lda _main__local_76
    ldx _main__local_76+1
    stx __zp_scratch
    ora __zp_scratch
    bne @if_then57
    bra @if_end59
@if_then57:
    lda #21
    ldx #0
    bra @__return
@if_end59:
    .loc "test_bool.c", 83
    lda #1
    sta _main__local_80
    lda #0
    sta _main__local_80+1
    .loc "test_bool.c", 84
    lda _main__local_80
    ldx _main__local_80+1
    cmp.16 .AX, #1
    bne @if_then60
    bra @if_end62
@if_then60:
    lda #22
    ldx #0
    bra @__return
@if_end62:
    .loc "test_bool.c", 87
    lda #42
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_bool.c", 5
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end63:
    .loc "test_bool.c", 87
    lda $22
    ldx $23
    sta _main__local_84
    stx _main__local_84+1
    .loc "test_bool.c", 88
    lda _main__local_84
    ldx _main__local_84+1
    cmp.16 .AX, #1
    bne @if_then65
    bra @if_end67
@if_then65:
    lda #23
    ldx #0
    bra @__return
@if_end67:
    .loc "test_bool.c", 90
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_bool.c", 5
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end68:
    .loc "test_bool.c", 90
    lda $22
    ldx $23
    sta _main__local_84
    stx _main__local_84+1
    .loc "test_bool.c", 91
    lda _main__local_84
    ldx _main__local_84+1
    stx __zp_scratch
    ora __zp_scratch
    bne @if_then70
    bra @if_end72
@if_then70:
    lda #24
    ldx #0
    bra @__return
@if_end72:
    .loc "test_bool.c", 94
    lda #231
    ldx #3
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_bool.c", 5
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end73:
    .loc "test_bool.c", 94
    lda $22
    ldx $23
    sta _main__local_95
    stx _main__local_95+1
    .loc "test_bool.c", 95
    lda _main__local_95
    ldx _main__local_95+1
    sta _main__local_99
    stx _main__local_99+1
    .loc "test_bool.c", 96
    lda _main__local_99
    ldx _main__local_99+1
    cmp.16 .AX, #1
    bne @if_then75
    bra @if_end77
@if_then75:
    lda #25
    ldx #0
    bra @__return
@if_end77:
    .loc "test_bool.c", 98
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_bool.c", 5
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end78:
    .loc "test_bool.c", 98
    lda $22
    ldx $23
    sta _main__local_95
    stx _main__local_95+1
    .loc "test_bool.c", 99
    lda _main__local_95
    ldx _main__local_95+1
    sta _main__local_99
    stx _main__local_99+1
    .loc "test_bool.c", 100
    lda _main__local_99
    ldx _main__local_99+1
    stx __zp_scratch
    ora __zp_scratch
    bne @if_then80
    bra @if_end82
@if_then80:
    lda #26
    ldx #0
    bra @__return
@if_end82:
    .loc "test_bool.c", 103
    lda #0
    sta _main__local_107
    sta _main__local_107+1
    .loc "test_bool.c", 104
    lda #200
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_bool.c", 5
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end83:
    .loc "test_bool.c", 104
    lda $22
    ldx $23
    sta _main__local_107
    stx _main__local_107+1
    .loc "test_bool.c", 105
    lda _main__local_107
    ldx _main__local_107+1
    cmp.16 .AX, #1
    bne @if_then85
    bra @if_end87
@if_then85:
    lda #27
    ldx #0
    bra @__return
@if_end87:
    .loc "test_bool.c", 106
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_bool.c", 5
    lda $20
    ldx $21
    sta $22
    stx $23
@inline_end88:
    .loc "test_bool.c", 106
    lda $22
    ldx $23
    sta _main__local_107
    stx _main__local_107+1
    .loc "test_bool.c", 107
    lda _main__local_107
    ldx _main__local_107+1
    stx __zp_scratch
    ora __zp_scratch
    bne @if_then90
    bra @if_end92
@if_then90:
    lda #28
    ldx #0
    bra @__return
@if_end92:
    .loc "test_bool.c", 109
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 50
    endproc


__zp_save_buf:

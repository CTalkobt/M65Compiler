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

    .extern __float_a
    .extern __float_b
    .extern __float_add
    .extern __float_sub
    .extern __float_mul
    .extern __float_div
    .extern __float_neg
    .extern __float_cmp
    .extern __float_itof
    .extern __float_ftoi
    .global _g_float
    .global _test_pointer
    .global _test_struct
    .global _test_struct_ptr
    .global _test_array
    .global _test_array_ptr
    .global _main

    .segment "data"
    .byte 0
_g_float:
; .debug_var: @global _g_float offset=0 size=5 type=float32 scope=global
    .byte $82, $48, $F5, $C2, $8F

    .segment "code"

; function _test_pointer
; SAC inline storage: 17 bytes
    _test_pointer__local_0: .fill 5
    _test_pointer__local_2: .word 0
    _test_pointer__local_4: .fill 5
    _test_pointer__local_8: .fill 5
    proc _test_pointer
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_float_indirect.c", 6
    .local @_l_p = 0
    .local @_l_x = 2
    .local @_l_y = 7
    .local @_l_z = 12
; .debug_var: __test_pointer @_l_p offset=0 size=2 type=ptr scope=local
; .debug_var: __test_pointer @_l_x offset=2 size=5 type=float32 scope=local
; .debug_var: __test_pointer @_l_y offset=7 size=5 type=float32 scope=local
; .debug_var: __test_pointer @_l_z offset=12 size=5 type=float32 scope=local

@entry:
    .loc "test_float_indirect.c", 7
    lda #$81
    sta $20+0
    lda #$40
    sta $20+1
    lda #$00
    sta $20+2
    lda #$00
    sta $20+3
    lda #$00
    sta $20+4
    lda $20+0
    sta $02+0
    lda $20+1
    sta $02+1
    lda $20+2
    sta $02+2
    lda $20+3
    sta $02+3
    lda $20+4
    sta $02+4
    .loc "test_float_indirect.c", 8
    leax.local 2
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _test_pointer__local_2
    stx _test_pointer__local_2+1
    .loc "test_float_indirect.c", 9
    lda _test_pointer__local_2
    ldx _test_pointer__local_2+1
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    sta $20+0
    ldy #1
    lda (__zp_scratch),y
    sta $20+1
    ldy #2
    lda (__zp_scratch),y
    sta $20+2
    ldy #3
    lda (__zp_scratch),y
    sta $20+3
    ldy #4
    lda (__zp_scratch),y
    sta $20+4
    lda $20+0
    sta $07+0
    lda $20+1
    sta $07+1
    lda $20+2
    sta $07+2
    lda $20+3
    sta $07+3
    lda $20+4
    sta $07+4
    .loc "test_float_indirect.c", 10
    lda #$82
    sta $20+0
    lda #$20
    sta $20+1
    lda #$00
    sta $20+2
    lda #$00
    sta $20+3
    lda #$00
    sta $20+4
    lda _test_pointer__local_2
    ldx _test_pointer__local_2+1
    sta $26
    stx $27
    ldy #0
    lda $20+0
    sta ($26),y
    ldy #1
    lda $20+1
    sta ($26),y
    ldy #2
    lda $20+2
    sta ($26),y
    ldy #3
    lda $20+3
    sta ($26),y
    ldy #4
    lda $20+4
    sta ($26),y
    .loc "test_float_indirect.c", 11
    lda _test_pointer__local_2
    ldx _test_pointer__local_2+1
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    sta $20+0
    ldy #1
    lda (__zp_scratch),y
    sta $20+1
    ldy #2
    lda (__zp_scratch),y
    sta $20+2
    ldy #3
    lda (__zp_scratch),y
    sta $20+3
    ldy #4
    lda (__zp_scratch),y
    sta $20+4
    lda $20+0
    sta $0C+0
    lda $20+1
    sta $0C+1
    lda $20+2
    sta $0C+2
    lda $20+3
    sta $0C+3
    lda $20+4
    sta $0C+4
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z
    .frame_size 17
    endproc

; function _test_struct
; SAC inline storage: 14 bytes
    _test_struct__local_0: .word 0
    _test_struct__local_10: .fill 5
    proc _test_struct
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_float_indirect.c", 21
    .local @_l_s = 5
    .local @_l_x = 0
; .debug_var: __test_struct @_l_s offset=5 size=2 type=int16 scope=local
; .debug_var: __test_struct @_l_x offset=0 size=5 type=float32 scope=local

@entry:
    .loc "test_float_indirect.c", 23
    lda #42
    ldx #0
    sta $20
    stx $21
    leax.local 5
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($22),y
    txa
    iny
    sta ($22),y
    .loc "test_float_indirect.c", 24
    lda #$82
    sta $20+0
    lda #$48
    sta $20+1
    lda #$F5
    sta $20+2
    lda #$C2
    sta $20+3
    lda #$8F
    sta $20+4
    leax.local 5
    sta $26
    stx $27
    lda $26
    ldx $27
    add.16 .AX, #2
    sta $28
    stx $29
    ldy #0
    lda $20+0
    sta ($28),y
    ldy #1
    lda $20+1
    sta ($28),y
    ldy #2
    lda $20+2
    sta ($28),y
    ldy #3
    lda $20+3
    sta ($28),y
    ldy #4
    lda $20+4
    sta ($28),y
    .loc "test_float_indirect.c", 25
    lda #99
    ldx #0
    sta $20
    stx $21
    leax.local 5
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #7
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_float_indirect.c", 26
    leax.local 5
    sta $20
    stx $21
    lda $20
    ldx $21
    add.16 .AX, #2
    sta $22
    stx $23
    ldy #0
    lda ($22),y
    sta $24+0
    ldy #1
    lda ($22),y
    sta $24+1
    ldy #2
    lda ($22),y
    sta $24+2
    ldy #3
    lda ($22),y
    sta $24+3
    ldy #4
    lda ($22),y
    sta $24+4
    lda $24+0
    sta $00+0
    lda $24+1
    sta $00+1
    lda $24+2
    sta $00+2
    lda $24+3
    sta $00+3
    lda $24+4
    sta $00+4
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 14
    endproc

; function _test_struct_ptr
; SAC inline storage: 16 bytes
    _test_struct_ptr__local_0: .word 0
    _test_struct_ptr__local_1: .word 0
    _test_struct_ptr__local_5: .fill 5
    proc _test_struct_ptr
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_float_indirect.c", 30
    .local @_l_p = 0
    .local @_l_s = 7
    .local @_l_x = 2
; .debug_var: __test_struct_ptr @_l_p offset=0 size=2 type=ptr scope=local
; .debug_var: __test_struct_ptr @_l_s offset=7 size=2 type=int16 scope=local
; .debug_var: __test_struct_ptr @_l_x offset=2 size=5 type=float32 scope=local

@entry:
    .loc "test_float_indirect.c", 32
    leax.local 7
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _test_struct_ptr__local_1
    stx _test_struct_ptr__local_1+1
    .loc "test_float_indirect.c", 33
    lda #$83
    sta $20+0
    lda #$48
    sta $20+1
    lda #$F5
    sta $20+2
    lda #$C2
    sta $20+3
    lda #$8F
    sta $20+4
    lda _test_struct_ptr__local_1
    ldx _test_struct_ptr__local_1+1
    add.16 .AX, #2
    sta $26
    stx $27
    ldy #0
    lda $20+0
    sta ($26),y
    ldy #1
    lda $20+1
    sta ($26),y
    ldy #2
    lda $20+2
    sta ($26),y
    ldy #3
    lda $20+3
    sta ($26),y
    ldy #4
    lda $20+4
    sta ($26),y
    .loc "test_float_indirect.c", 34
    lda $26
    ldx $27
    sta $20
    stx $21
    ldy #0
    lda ($26),y
    sta $20+0
    ldy #1
    lda ($26),y
    sta $20+1
    ldy #2
    lda ($26),y
    sta $20+2
    ldy #3
    lda ($26),y
    sta $20+3
    ldy #4
    lda ($26),y
    sta $20+4
    lda $20+0
    sta $02+0
    lda $20+1
    sta $02+1
    lda $20+2
    sta $02+2
    lda $20+3
    sta $02+3
    lda $20+4
    sta $02+4
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 16
    endproc

; function _test_array
; SAC inline storage: 20 bytes
    _test_array__local_0: .fill 5
    _test_array__local_13: .fill 5
    proc _test_array
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_float_indirect.c", 38
    .local @_l_arr = 5
    .local @_l_x = 0
; .debug_var: __test_array @_l_arr offset=5 size=5 type=float32 scope=local
; .debug_var: __test_array @_l_x offset=0 size=5 type=float32 scope=local

@entry:
    .loc "test_float_indirect.c", 40
    lda #$81
    sta $20+0
    lda #$00
    sta $20+1
    lda #$00
    sta $20+2
    lda #$00
    sta $20+3
    lda #$00
    sta $20+4
    leax.local 5
    sta $26
    stx $27
    lda #0
    sta $28
    sta $29
    ldy #0
    lda $20+0
    sta ($2A),y
    ldy #1
    lda $20+1
    sta ($2A),y
    ldy #2
    lda $20+2
    sta ($2A),y
    ldy #3
    lda $20+3
    sta ($2A),y
    ldy #4
    lda $20+4
    sta ($2A),y
    .loc "test_float_indirect.c", 41
    lda #$82
    sta $20+0
    lda #$00
    sta $20+1
    lda #$00
    sta $20+2
    lda #$00
    sta $20+3
    lda #$00
    sta $20+4
    leax.local 5
    sta $26
    stx $27
    lda #1
    ldx #0
    sta $28
    stx $29
    ldy #0
    lda $20+0
    sta ($2A),y
    ldy #1
    lda $20+1
    sta ($2A),y
    ldy #2
    lda $20+2
    sta ($2A),y
    ldy #3
    lda $20+3
    sta ($2A),y
    ldy #4
    lda $20+4
    sta ($2A),y
    .loc "test_float_indirect.c", 42
    lda #$82
    sta $20+0
    lda #$40
    sta $20+1
    lda #$00
    sta $20+2
    lda #$00
    sta $20+3
    lda #$00
    sta $20+4
    leax.local 5
    sta $26
    stx $27
    lda #2
    ldx #0
    sta $28
    stx $29
    ldy #0
    lda $20+0
    sta ($2A),y
    ldy #1
    lda $20+1
    sta ($2A),y
    ldy #2
    lda $20+2
    sta ($2A),y
    ldy #3
    lda $20+3
    sta ($2A),y
    ldy #4
    lda $20+4
    sta ($2A),y
    .loc "test_float_indirect.c", 43
    leax.local 5
    sta $20
    stx $21
    lda #1
    ldx #0
    sta $22
    stx $23
    ldy #0
    lda ($24),y
    sta $26+0
    ldy #1
    lda ($24),y
    sta $26+1
    ldy #2
    lda ($24),y
    sta $26+2
    ldy #3
    lda ($24),y
    sta $26+3
    ldy #4
    lda ($24),y
    sta $26+4
    lda $26+0
    sta $00+0
    lda $26+1
    sta $00+1
    lda $26+2
    sta $00+2
    lda $26+3
    sta $00+3
    lda $26+4
    sta $00+4
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z
    .frame_size 20
    endproc

; function _test_array_ptr
; SAC inline storage: 22 bytes
    _test_array_ptr__local_0: .fill 5
    _test_array_ptr__local_1: .word 0
    _test_array_ptr__local_9: .fill 5
    proc _test_array_ptr
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_float_indirect.c", 47
    .local @_l_arr = 7
    .local @_l_p = 0
    .local @_l_x = 2
; .debug_var: __test_array_ptr @_l_arr offset=7 size=5 type=float32 scope=local
; .debug_var: __test_array_ptr @_l_p offset=0 size=2 type=ptr scope=local
; .debug_var: __test_array_ptr @_l_x offset=2 size=5 type=float32 scope=local

@entry:
    .loc "test_float_indirect.c", 49
    leax.local 7
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _test_array_ptr__local_1
    stx _test_array_ptr__local_1+1
    .loc "test_float_indirect.c", 50
    lda #$84
    sta $20+0
    lda #$20
    sta $20+1
    lda #$00
    sta $20+2
    lda #$00
    sta $20+3
    lda #$00
    sta $20+4
    lda #0
    sta $26
    sta $27
    ldy #0
    lda $20+0
    sta ($28),y
    ldy #1
    lda $20+1
    sta ($28),y
    ldy #2
    lda $20+2
    sta ($28),y
    ldy #3
    lda $20+3
    sta ($28),y
    ldy #4
    lda $20+4
    sta ($28),y
    .loc "test_float_indirect.c", 51
    lda #$85
    sta $20+0
    lda #$20
    sta $20+1
    lda #$00
    sta $20+2
    lda #$00
    sta $20+3
    lda #$00
    sta $20+4
    lda #1
    ldx #0
    sta $26
    stx $27
    ldy #0
    lda $20+0
    sta ($28),y
    ldy #1
    lda $20+1
    sta ($28),y
    ldy #2
    lda $20+2
    sta ($28),y
    ldy #3
    lda $20+3
    sta ($28),y
    ldy #4
    lda $20+4
    sta ($28),y
    .loc "test_float_indirect.c", 52
    lda #1
    ldx #0
    sta $20
    stx $21
    ldy #0
    lda ($22),y
    sta $24+0
    ldy #1
    lda ($22),y
    sta $24+1
    ldy #2
    lda ($22),y
    sta $24+2
    ldy #3
    lda ($22),y
    sta $24+3
    ldy #4
    lda ($22),y
    sta $24+4
    lda $24+0
    sta $02+0
    lda $24+1
    sta $02+1
    lda $24+2
    sta $02+2
    lda $24+3
    sta $02+3
    lda $24+4
    sta $02+4
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z
    .frame_size 22
    endproc

; function _main
; SAC inline storage: 94 bytes
    _main__local_0: .word 0
    _main__local_4: .fill 5
    _main__local_8: .fill 5
    _main__local_10: .word 0
    _main__local_11: .word 0
    _main__local_12: .word 0
    _main__local_13: .word 0
    _main__local_14: .word 0
    _main__local_15: .word 0
    _main__local_16: .word 0
    _main__local_17: .word 0
    _main__local_18: .word 0
    _main__local_19: .word 0
    _main__local_20: .word 0
    _main__local_21: .word 0
    _main__local_22: .word 0
    _main__local_23: .word 0
    _main__local_24: .word 0
    _main__local_25: .word 0
    _main__local_26: .word 0
    _main__local_27: .word 0
    _main__local_28: .word 0
    _main__local_29: .word 0
    _main__local_30: .word 0
    _main__local_31: .word 0
    _main__local_32: .word 0
    _main__local_33: .word 0
    _main__local_34: .word 0
    _main__local_35: .word 0
    _main__local_36: .word 0
    _main__local_37: .word 0
    _main__local_38: .word 0
    _main__local_39: .word 0
    _main__local_40: .word 0
    _main__local_41: .word 0
    _main__local_42: .word 0
    _main__local_43: .word 0
    _main__local_44: .word 0
    _main__local_45: .word 0
    _main__local_46: .word 0
    _main__local_47: .word 0
    _main__local_48: .word 0
    _main__local_49: .word 0
    _main__local_50: .fill 5
    _main__local_51: .word 0
    _main__local_52: .word 0
    _main__local_53: .word 0
    _main__local_54: .word 0
    _main__local_55: .word 0
    _main__local_56: .word 0
    _main__local_57: .word 0
    _main__local_58: .word 0
    _main__local_59: .fill 5
    _main__local_60: .word 0
    _main__local_61: .word 0
    _main__local_62: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_float_indirect.c", 55
    .local @_l_arr = 74
    .local @_l_p = 4
    .local @_l_s = 50
    .local @_l_x = 36
    .local @_l_y = 11
    .local @_l_z = 16
; .debug_var: __main @_l_arr offset=74 size=5 type=float32 scope=local
; .debug_var: __main @_l_p offset=4 size=2 type=ptr scope=local
; .debug_var: __main @_l_s offset=50 size=2 type=int16 scope=local
; .debug_var: __main @_l_x offset=36 size=5 type=float32 scope=local
; .debug_var: __main @_l_y offset=11 size=5 type=float32 scope=local
; .debug_var: __main @_l_z offset=16 size=5 type=float32 scope=local

@entry:
    .loc "test_float_indirect.c", 7
    lda #$81
    sta $20+0
    lda #$40
    sta $20+1
    lda #$00
    sta $20+2
    lda #$00
    sta $20+3
    lda #$00
    sta $20+4
    lda $20+0
    sta $59+0
    lda $20+1
    sta $59+1
    lda $20+2
    sta $59+2
    lda $20+3
    sta $59+3
    lda $20+4
    sta $59+4
    .loc "test_float_indirect.c", 8
    leax.local 89
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_float_indirect.c", 9
    ldy #0
    lda ($22),y
    sta $24+0
    ldy #1
    lda ($22),y
    sta $24+1
    ldy #2
    lda ($22),y
    sta $24+2
    ldy #3
    lda ($22),y
    sta $24+3
    ldy #4
    lda ($22),y
    sta $24+4
    lda $24+0
    sta $0B+0
    lda $24+1
    sta $0B+1
    lda $24+2
    sta $0B+2
    lda $24+3
    sta $0B+3
    lda $24+4
    sta $0B+4
    .loc "test_float_indirect.c", 10
    lda #$82
    sta $24+0
    lda #$20
    sta $24+1
    lda #$00
    sta $24+2
    lda #$00
    sta $24+3
    lda #$00
    sta $24+4
    lda $22
    ldx $23
    sta $20
    stx $21
    ldy #0
    lda $24+0
    sta ($20),y
    ldy #1
    lda $24+1
    sta ($20),y
    ldy #2
    lda $24+2
    sta ($20),y
    ldy #3
    lda $24+3
    sta ($20),y
    ldy #4
    lda $24+4
    sta ($20),y
    .loc "test_float_indirect.c", 11
    ldy #0
    lda ($22),y
    sta $24+0
    ldy #1
    lda ($22),y
    sta $24+1
    ldy #2
    lda ($22),y
    sta $24+2
    ldy #3
    lda ($22),y
    sta $24+3
    ldy #4
    lda ($22),y
    sta $24+4
    lda $24+0
    sta $10+0
    lda $24+1
    sta $10+1
    lda $24+2
    sta $10+2
    lda $24+3
    sta $10+3
    lda $24+4
    sta $10+4
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z
    .frame_size 94
    endproc


__zp_save_buf:

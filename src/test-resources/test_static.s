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

    .global _use_statics
    .global _counter
    .global _other_counter
    .global _main

    .segment "data"
    .byte 0
_sg_count:
; .debug_var: @global _sg_count offset=0 size=2 type=int16 scope=global
    .word 10

    .segment "bss"
_sg_flag:
; .debug_var: @global _sg_flag offset=0 size=2 type=int8 scope=global
    .res 1
    .segment "data"
__counter__local_calls:
; .debug_var: @global __counter__local_calls offset=0 size=2 type=int16 scope=global
    .word 0
__other_counter__local_calls:
; .debug_var: @global __other_counter__local_calls offset=0 size=2 type=int16 scope=global
    .word 0
__main__local_calls:
; .debug_var: @global __main__local_calls offset=0 size=2 type=int16 scope=global
    .word 0

    .segment "code"

; function _use_statics
; SAC inline storage: 0 bytes
    _use_statics__local_5: .word 0
    _use_statics__local_6: .word 0
    proc _use_statics
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_static.c", 14

@entry:
    .loc "test_static.c", 15
    lda _sg_count
    ldx _sg_count+1
    sta $20
    stx $21
    lda $20
    clc
    adc #5
    sta $24
    lda $21
    adc #0
    sta $25
    lda $24
    ldx $25
    sta _sg_count
    stx _sg_count+1
    .loc "test_static.c", 16
    lda #1
    sta $20
    lda $20
    sta _sg_flag
    .loc "test_static.c", 17
    lda _sg_count
    ldx _sg_count+1
    sta $20
    stx $21
    .loc "test_static.c", 10
    lda $20
    clc
    adc #1
    sta $24
    lda $21
    adc #0
    sta $25
@inline_end0:
    .loc "test_static.c", 17
    lda $24
    ldx $25
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 0
    endproc

; function _counter
; SAC zero-alloc leaf: no storage overhead
    proc _counter
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_static.c", 21

@entry:
    .loc "test_static.c", 23
    ldax #__counter__local_calls
    add.16 .AX, #1
    sta $22
    stx $23
    lda $22
    ldx $23
    sta __counter__local_calls
    stx __counter__local_calls+1
    .loc "test_static.c", 24
    ldax #__counter__local_calls
@__return:
    rts
    .func_flags stack_call, static_alloc, zeroalloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 0
    endproc

; function _other_counter
; SAC zero-alloc leaf: no storage overhead
    proc _other_counter
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_static.c", 28

@entry:
    .loc "test_static.c", 30
    ldax #__other_counter__local_calls
    add.16 .AX, #1
    sta $22
    stx $23
    lda $22
    ldx $23
    sta __other_counter__local_calls
    stx __other_counter__local_calls+1
    .loc "test_static.c", 31
    ldax #__other_counter__local_calls
@__return:
    rts
    .func_flags stack_call, static_alloc, zeroalloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 0
    endproc

; function _main
; SAC inline storage: 6 bytes
    _main__local_0: .word 0
    _main__local_1: .word 0
    _main__local_2: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_static.c", 34
    .local @_l_a = 0
    .local @_l_b = 2
    .local @_l_c = 4
; .debug_var: __main @_l_a offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_b offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_c offset=4 size=2 type=int16 scope=local

@entry:
    .loc "test_static.c", 40
    .loc "test_static.c", 43
    .loc "test_static.c", 10
@inline_end2:
    .loc "test_static.c", 43
    .loc "test_static.c", 15
    lda _sg_count
    ldx _sg_count+1
    sta $20
    stx $21
    lda $20
    clc
    adc #5
    sta $24
    lda $21
    adc #0
    sta $25
    lda $24
    ldx $25
    sta _sg_count
    stx _sg_count+1
    .loc "test_static.c", 16
    lda #1
    sta $20
    lda $20
    sta _sg_flag
    .loc "test_static.c", 17
    .loc "test_static.c", 10
@inline_end5:
    .loc "test_static.c", 17
@inline_end4:
    .loc "test_static.c", 46
    .loc "test_static.c", 23
    ldax #__main__local_calls
    add.16 .AX, #1
    sta $22
    stx $23
    lda $22
    ldx $23
    sta __main__local_calls
    stx __main__local_calls+1
    .loc "test_static.c", 24
@inline_end8:
    .loc "test_static.c", 49
    .loc "test_static.c", 23
    ldax #__main__local_calls
    add.16 .AX, #1
    sta $22
    stx $23
    lda $22
    ldx $23
    sta __main__local_calls
    stx __main__local_calls+1
    .loc "test_static.c", 24
@inline_end10:
    .loc "test_static.c", 50
    .loc "test_static.c", 30
    ldax #__main__local_calls
    add.16 .AX, #1
    sta $22
    stx $23
    lda $22
    ldx $23
    sta __main__local_calls
    stx __main__local_calls+1
    .loc "test_static.c", 31
@inline_end12:
    .loc "test_static.c", 51
    .loc "test_static.c", 53
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 6
    endproc


__zp_save_buf:

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

    .extern _printf

    .global _add_stuff
    .global _main

    .segment "code"

; function _add_stuff
; SAC inline storage: 4 bytes
    .global _add_stuff__param_val
    _add_stuff__param_val: .word 0
    _add_stuff__local_0: .word 0
    _add_stuff__local_1: .word 0
    proc _add_stuff, W#@_p_val
    .sac
    .var _fp = 0
    .loc "test_proc.c", 3
    .local @_l_x = 2
; .debug_var: __add_stuff @_l_x offset=2 size=2 type=int16 scope=local
    .var @_p_val = 2
; .debug_var: __add_stuff @_p_val offset=2 size=2 type=int16 scope=parameter

@entry:
    .loc "test_proc.c", 4
    lda #1
    sta _add_stuff__local_1
    lda #0
    sta _add_stuff__local_1+1
    .loc "test_proc.c", 5
    lda _add_stuff__local_1
    ldx _add_stuff__local_1+1
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda _add_stuff__param_val
    ldx _add_stuff__param_val+1
    add.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    clc
    adc #2
    sta $24
    lda $21
    adc #0
    sta $25
    lda $24
    ldx $25
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 4
    endproc

; function _main
; SAC inline storage: 2 bytes
    _main__local_0: .word 0
    _main__local_3: .word 0
    _main__local_4: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_proc.c", 8
    .local @_l_x = 0
; .debug_var: __main @_l_x offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_proc.c", 9
    ldax #__str_0
    sta _main__local_0
    stx _main__local_0+1
    lda #10
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    .loc "test_proc.c", 4
    lda #1
    sta _main__local_4
    lda #0
    sta _main__local_4+1
    .loc "test_proc.c", 5
    lda _main__local_4
    ldx _main__local_4+1
    add.16 .AX, $20
    sta $22
    stx $23
    lda $22
    clc
    adc #2
    sta $24
    lda $23
    adc #0
    sta $25
    lda $24
    ldx $25
    sta _main__local_3
    stx _main__local_3+1
@inline_end1:
    .loc "test_proc.c", 9
    lda _main__local_3
    ldx _main__local_3+1
    sta $28
    stx $29
    lda _main__local_0
    ldx _main__local_0+1
    sta $2A
    stx $2B
    lda $28
    ldx $29
    push .ax
    lda $2A
    ldx $2B
    push .ax
    jsr _printf
    sta __zp_scratch4
    stx __zp_scratch4+1
    plz
    lda __zp_scratch4
    ldx __zp_scratch4+1
    .loc "test_proc.c", 10
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc


    .segment "data"
__str_0:
    .text "Result: %d"
    .byte 0

__zp_save_buf:

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

    .global _get_big
    .global _add_big
    .global _main

    .segment "code"

; function _get_big

    ; Static buffer for struct return from _get_big
    _get_big__struct_buf:
    .byte 0, 0, 0, 0

; SAC inline storage: 4 bytes
    _get_big__local_0: .long 0
    proc _get_big
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_literal_promotion.c", 3
    .local @_l_x = 0
; .debug_var: __get_big @_l_x offset=0 size=4 type=int32 scope=local

@entry:
    .loc "test_literal_promotion.c", 4
    lda #160
    ldx #134
    ldy #1
    ldz #0
    sta $20
    stx $21
    sty $22
    stz $23
    sta _get_big__local_0
    stx _get_big__local_0+1
    sty _get_big__local_0+2
    stz _get_big__local_0+3
    .loc "test_literal_promotion.c", 5
    lda _get_big__local_0
    ldx _get_big__local_0+1
    ldy _get_big__local_0+2
    ldz _get_big__local_0+3
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers N, Z
    .frame_size 4
    endproc

; function _add_big

    ; Static buffer for struct return from _add_big
    _add_big__struct_buf:
    .byte 0, 0, 0, 0

; SAC inline storage: 8 bytes
    _add_big__local_0: .long 0
    _add_big__local_2: .long 0
    proc _add_big
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_literal_promotion.c", 8
    .local @_l_a = 0
    .local @_l_b = 4
; .debug_var: __add_big @_l_a offset=0 size=4 type=int32 scope=local
; .debug_var: __add_big @_l_b offset=4 size=4 type=int32 scope=local

@entry:
    .loc "test_literal_promotion.c", 9
    lda #80
    sta _add_big__local_0
    lda #195
    sta _add_big__local_0+1
    .loc "test_literal_promotion.c", 10
    lda #112
    ldx #17
    ldy #1
    ldz #0
    sta $20
    stx $21
    sty $22
    stz $23
    sta _add_big__local_2
    stx _add_big__local_2+1
    sty _add_big__local_2+2
    stz _add_big__local_2+3
    .loc "test_literal_promotion.c", 11
    lda _add_big__local_2
    ldx _add_big__local_2+1
    ldy _add_big__local_2+2
    ldz _add_big__local_2+3
    sta $10
    stx $11
    sty $12
    stz $13
    lda _add_big__local_0
    ldx _add_big__local_0+1
    ldy _add_big__local_0+2
    ldz _add_big__local_0+3
    add.32 .AXYZ, $10
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 8
    endproc

; function _main
; SAC inline storage: 20 bytes
    _main__local_0: .long 0
    _main__local_2: .long 0
    _main__local_7: .long 0
    _main__local_9: .long 0
    _main__local_11: .long 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_literal_promotion.c", 14
    .local @_l_a = 12
    .local @_l_b = 16
    .local @_l_s = 8
    .local @_l_v = 0
    .local @_l_x = 4
; .debug_var: __main @_l_a offset=12 size=4 type=int32 scope=local
; .debug_var: __main @_l_b offset=16 size=4 type=int32 scope=local
; .debug_var: __main @_l_s offset=8 size=4 type=int32 scope=local
; .debug_var: __main @_l_v offset=0 size=4 type=int32 scope=local
; .debug_var: __main @_l_x offset=4 size=4 type=int32 scope=local

@entry:
    .loc "test_literal_promotion.c", 4
    lda #160
    ldx #134
    ldy #1
    ldz #0
    sta $20
    stx $21
    sty $22
    stz $23
    sta _main__local_2
    stx _main__local_2+1
    sty _main__local_2+2
    stz _main__local_2+3
    lda _main__local_2
    ldx _main__local_2+1
    ldy _main__local_2+2
    ldz _main__local_2+3
    sta $20
    stx $21
    .loc "test_literal_promotion.c", 5
    lda $20
    ldx $21
    sta $22
    stx $23
    sty $24
    stz $25
@inline_end0:
    .loc "test_literal_promotion.c", 16
    lda $22
    ldx $23
    ldy $24
    ldz $25
    sta _main__local_0
    stx _main__local_0+1
    sty _main__local_0+2
    stz _main__local_0+3
    .loc "test_literal_promotion.c", 17
    lda _main__local_0
    ldx _main__local_0+1
    ldy _main__local_0+2
    ldz _main__local_0+3
    cmp.32 .AXYZ, #100000
    bne @if_then2
    bra @if_end4
@if_then2:
    lda #1
    ldx #0
    bra @__return
@if_end4:
    .loc "test_literal_promotion.c", 9
    lda #80
    sta _main__local_9
    lda #195
    sta _main__local_9+1
    .loc "test_literal_promotion.c", 10
    lda #112
    ldx #17
    ldy #1
    ldz #0
    sta $20
    stx $21
    sty $22
    stz $23
    sta _main__local_11
    stx _main__local_11+1
    sty _main__local_11+2
    stz _main__local_11+3
    .loc "test_literal_promotion.c", 11
    lda _main__local_11
    ldx _main__local_11+1
    ldy _main__local_11+2
    ldz _main__local_11+3
    sta $10
    stx $11
    sty $12
    stz $13
    lda _main__local_9
    ldx _main__local_9+1
    ldy _main__local_9+2
    ldz _main__local_9+3
    add.32 .AXYZ, $10
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    sta $24
    stx $25
    lda $24
    ldx $25
    sta $20
    stx $21
    sty $22
    stz $23
@inline_end5:
    .loc "test_literal_promotion.c", 20
    lda $20
    ldx $21
    ldy $22
    ldz $23
    sta _main__local_7
    stx _main__local_7+1
    sty _main__local_7+2
    stz _main__local_7+3
    .loc "test_literal_promotion.c", 21
    lda _main__local_7
    ldx _main__local_7+1
    ldy _main__local_7+2
    ldz _main__local_7+3
    cmp.32 .AXYZ, #120000
    bne @if_then7
    bra @if_end9
@if_then7:
    lda #2
    ldx #0
    bra @__return
@if_end9:
    .loc "test_literal_promotion.c", 23
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 20
    endproc


__zp_save_buf:

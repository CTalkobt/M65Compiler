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

    .global _global_aligned
    .global _global_big
    .global _test_alignas_local_4byte
    .global _test_alignas_local_16byte
    .global _test_alignas_struct_member
    .global _test_alignas_scope_reuse
    .global _test_alignas_global

    .segment "data"
    .byte 0
_global_aligned:
; .debug_var: @global _global_aligned offset=0 size=2 type=int16 scope=global
    .word 42

    .segment "bss"
_global_big:
; .debug_var: @global _global_big offset=0 size=2 type=int8 scope=global
    .res 8

    .segment "code"

; function _test_alignas_local_4byte
; SAC inline storage: 4 bytes
    _test_alignas_local_4byte__local_0: .word 0
    _test_alignas_local_4byte__local_1: .word 0
    proc _test_alignas_local_4byte
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_alignas_local.c", 2
    .local @_l_before = 0
    .local @_l_x = 2
; .debug_var: __test_alignas_local_4byte @_l_before offset=0 size=2 type=int8 scope=local
; .debug_var: __test_alignas_local_4byte @_l_x offset=2 size=2 type=int16 scope=local

@entry:
    .loc "test_alignas_local.c", 5
    leax.local 2
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda #3
    ldx #0
    sta $20
    stx $21
    lda $22
    ldx $23
    and.16 .AX, $20
    sta $24
    stx $25
    lda $24
    ldx $25
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z
    .frame_size 4
    endproc

; function _test_alignas_local_16byte
; SAC inline storage: 10 bytes
    _test_alignas_local_16byte__local_0: .word 0
    _test_alignas_local_16byte__local_1: .word 0
    proc _test_alignas_local_16byte
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_alignas_local.c", 8
    .local @_l_before = 0
    .local @_l_buf = 2
; .debug_var: __test_alignas_local_16byte @_l_before offset=0 size=2 type=int8 scope=local
; .debug_var: __test_alignas_local_16byte @_l_buf offset=2 size=2 type=int8 scope=local

@entry:
    .loc "test_alignas_local.c", 11
    leax.local 2
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    lda #15
    ldx #0
    sta $20
    stx $21
    lda $22
    ldx $23
    and.16 .AX, $20
    sta $24
    stx $25
    lda $24
    ldx $25
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z
    .frame_size 10
    endproc

; function _test_alignas_struct_member
; SAC inline storage: 12 bytes
    _test_alignas_struct_member__local_0: .word 0
    _test_alignas_struct_member__local_1: .word 0
    _test_alignas_struct_member__local_8: .word 0
    proc _test_alignas_struct_member
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_alignas_local.c", 22
    .local @_l_offset_arr = 2
    .local @_l_offset_b = 0
    .local @_l_s = 4
; .debug_var: __test_alignas_struct_member @_l_offset_arr offset=2 size=2 type=int16 scope=local
; .debug_var: __test_alignas_struct_member @_l_offset_b offset=0 size=2 type=int16 scope=local
; .debug_var: __test_alignas_struct_member @_l_s offset=4 size=2 type=int16 scope=local

@entry:
    .loc "test_alignas_local.c", 24
    leax.local 4
    sta $20
    stx $21
    lda $20
    ldx $21
    add.16 .AX, #1
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
    leax.local 4
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    lda $20
    sec
    sbc $24
    sta $22
    lda $21
    sbc $24+1
    sta $23
    lda $22
    ldx $23
    sta _test_alignas_struct_member__local_1
    stx _test_alignas_struct_member__local_1+1
    .loc "test_alignas_local.c", 25
    leax.local 4
    sta $20
    stx $21
    lda $20
    ldx $21
    add.16 .AX, #4
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $20
    stx $21
    leax.local 4
    sta $22
    stx $23
    lda $22
    ldx $23
    sta $24
    stx $25
    lda $20
    sec
    sbc $24
    sta $22
    lda $21
    sbc $24+1
    sta $23
    lda $22
    ldx $23
    sta _test_alignas_struct_member__local_8
    stx _test_alignas_struct_member__local_8+1
    .loc "test_alignas_local.c", 28
    lda _test_alignas_struct_member__local_1
    ldx _test_alignas_struct_member__local_1+1
    cmp.16 .AX, #4
    bne @if_then0
    bra @if_end2
@if_then0:
    lda #1
    ldx #0
    bra @__return
@if_end2:
    .loc "test_alignas_local.c", 30
    lda _test_alignas_struct_member__local_8
    ldx _test_alignas_struct_member__local_8+1
    cmp.16 .AX, #16
    bne @if_then3
    bra @if_end5
@if_then3:
    lda #2
    ldx #0
    bra @__return
@if_end5:
    .loc "test_alignas_local.c", 32
    lda #3
    ldx #0
    sta $20
    stx $21
    lda _test_alignas_struct_member__local_1
    ldx _test_alignas_struct_member__local_1+1
    and.16 .AX, $20
    sta $22
    stx $23
    lda $22
    ldx $23
    bne @if_then6
    cmp #$00
    bne @if_then6
    bra @if_end8
@if_then6:
    lda #3
    ldx #0
    bra @__return
@if_end8:
    .loc "test_alignas_local.c", 34
    lda #15
    ldx #0
    sta $20
    stx $21
    lda _test_alignas_struct_member__local_8
    ldx _test_alignas_struct_member__local_8+1
    and.16 .AX, $20
    sta $22
    stx $23
    lda $22
    ldx $23
    bne @if_then9
    cmp #$00
    bne @if_then9
    bra @if_end11
@if_then9:
    lda #4
    ldx #0
    bra @__return
@if_end11:
    .loc "test_alignas_local.c", 36
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 12
    endproc

; function _test_alignas_scope_reuse
; SAC inline storage: 18 bytes
    _test_alignas_scope_reuse__local_0: .word 0
    _test_alignas_scope_reuse__local_1: .word 0
    _test_alignas_scope_reuse__local_2: .word 0
    proc _test_alignas_scope_reuse
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_alignas_local.c", 40
    .local @_l_buf1 = 2
    .local @_l_buf2 = 10
    .local @_l_unused = 0
; .debug_var: __test_alignas_scope_reuse @_l_buf1 offset=2 size=2 type=int8 scope=local
; .debug_var: __test_alignas_scope_reuse @_l_buf2 offset=10 size=2 type=int8 scope=local
; .debug_var: __test_alignas_scope_reuse @_l_unused offset=0 size=2 type=int8 scope=local

@entry:
    .loc "test_alignas_local.c", 49
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X
    .flag_clobbers N, Z
    .frame_size 18
    endproc

; function _test_alignas_global
; SAC zero-alloc leaf: no storage overhead
    proc _test_alignas_global
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_alignas_local.c", 56

@entry:
    .loc "test_alignas_local.c", 58
    lda _global_aligned
    ldx _global_aligned+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #42
    beq @tern_then12
    bra @tern_else13
@tern_then12:
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @tern_end14
@tern_else13:
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@tern_end14:
    lda $22
    ldx $23
@__return:
    rts
    .func_flags stack_call, static_alloc, zeroalloc, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    .frame_size 0
    endproc


__zp_save_buf:

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

    .global _parent
    .global _parent_nested
    .global _main
    .global _parent_nested

    .segment "code"

; function _parent
; SAC inline storage: 2 bytes
    _parent__local_0: .word 0
    _parent__local_2: .word 0
    proc _parent
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdio.h", 3
    .local @_l_x = 0
; .debug_var: __parent @_l_x offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_nested_direct.c", 4
    lda #42
    sta _parent__local_0
    lda #0
    sta _parent__local_0+1
    .loc "test_nested_direct.c", 8
    lda $FD
    ldx $FE
    sta $20
    stx $21
    lda $20
    ldx $21
    sta __static_chain
    stx __static_chain+1
    jsr _parent_nested
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _parent_nested
; SAC inline storage: 0 bytes
    _parent_nested__local_0: .word 0
    _parent_nested__local_2: .word 0
    proc _parent_nested
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdio.h", 5
    .local @_l___static_link = 0
; .debug_var: __parent_nested @_l___static_link offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_nested_direct.c", 5
    lda __static_chain
    ldx __static_chain+1
    sta _parent_nested__local_2
    stx _parent_nested__local_2+1
    .loc "test_nested_direct.c", 6
    ldax #__str_0
    sta _parent_nested__local_0
    stx _parent_nested__local_0+1
    lda _parent_nested__local_2
    ldx _parent_nested__local_2+1
    sta $20
    stx $21
    ldy #0
    lda ($20),y
    pha
    iny
    lda ($20),y
    tax
    pla
    sta _parent_nested__local_2
    stx _parent_nested__local_2+1
    lda _parent_nested__local_2
    ldx _parent_nested__local_2+1
    sta $28
    stx $29
    lda _parent_nested__local_0
    ldx _parent_nested__local_0+1
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
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 0
    endproc

; function _main
; SAC inline storage: 2 bytes
    _main__local_0: .word 0
    _main__local_2: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdio.h", 11
    .local @_l_x = 0
; .debug_var: __main @_l_x offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_nested_direct.c", 4
    lda #42
    sta _main__local_0
    lda #0
    sta _main__local_0+1
    .loc "test_nested_direct.c", 8
    lda _main__local_0
    ldx _main__local_0+1
    sta __static_chain
    stx __static_chain+1
    jsr _parent_nested
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 2
    endproc

; function _parent_nested
; SAC inline storage: 0 bytes
    _parent_nested__local_0: .word 0
    _parent_nested__local_2: .word 0
    proc _parent_nested
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdio.h", 5
    .local @_l___static_link = 0
; .debug_var: __parent_nested @_l___static_link offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_nested_direct.c", 5
    lda __static_chain
    ldx __static_chain+1
    sta _parent_nested__local_2
    stx _parent_nested__local_2+1
    .loc "test_nested_direct.c", 6
    ldax #__str_2
    sta _parent_nested__local_0
    stx _parent_nested__local_0+1
    lda _parent_nested__local_2
    ldx _parent_nested__local_2+1
    sta $20
    stx $21
    ldy #0
    lda ($20),y
    pha
    iny
    lda ($20),y
    tax
    pla
    sta _parent_nested__local_2
    stx _parent_nested__local_2+1
    lda _parent_nested__local_2
    ldx _parent_nested__local_2+1
    sta $28
    stx $29
    lda _parent_nested__local_0
    ldx _parent_nested__local_0+1
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
@__return:
    rts
    .func_flags stack_call, static_alloc
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    .frame_size 0
    endproc


    .segment "data"
__str_0:
    .text "Nested: x = %d
"
    .byte 0
__str_2:
    .text "Nested: x = %d
"
    .byte 0

__zp_save_buf:

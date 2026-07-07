* = $2000
    __sp_base = $0101
; Save ZP $08-$FF to BSS buffer
    ldx #0
@__zp_save_loop:
    lda $08,x
    sta __zp_save_buf,x
    inx
    cpx #248
    bne @__zp_save_loop
    jsr _main
    sta $02
    stx $03
; Restore ZP $08-$FF from BSS buffer
    ldx #0
@__zp_restore_loop:
    lda __zp_save_buf,x
    sta $08,x
    inx
    cpx #248
    bne @__zp_restore_loop
    lda $02
    ldx $03
__halt:
    jmp __halt
    .global __static_chain
    .global __zp_scratch
    .global __zp_scratch2
    .global __zp_scratch3
    .global __zp_scratch4
    __static_chain = $06
    __zp_scratch = $08
    __zp_scratch2 = $0A
    __zp_scratch3 = $0C
    __zp_scratch4 = $0E

_results:
    .word 0
_g_value:
    .word 0
_g_flag:
    .byte 0


; function _store_to_global
    proc _store_to_global, W#@_p_val
    .var _fp = 0
    .loc "src/test-resources/test_inline_asm.c", 11
    tsx
    txa
    clc
    adc #1
    sta $FD
    lda #$01
    adc #0
    sta $FE
    .var @_p_val = 2

    ldax.fp @_p_val
    sta $20
    stx $21
@entry:
    .loc "src/test-resources/test_inline_asm.c", 13
    ldax @_p_val, sp
    .loc "src/test-resources/test_inline_asm.c", 14
    stax _g_value
@__return:
    .func_flags stack_call, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc

; function _set_flag
    proc _set_flag, B#@_p_f
    .var _fp = 0
    .loc "src/test-resources/test_inline_asm.c", 17
    tsx
    txa
    clc
    adc #1
    sta $FD
    lda #$01
    adc #0
    sta $FE
    .var @_p_f = 2

    ldax.fp @_p_f
    sta $20
@entry:
    .loc "src/test-resources/test_inline_asm.c", 19
    lda @_p_f, sp
    .loc "src/test-resources/test_inline_asm.c", 20
    sta _g_flag
@__return:
    .func_flags stack_call, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc

; function _copy_via_asm
    proc _copy_via_asm, W#@_p_val
    .var _fp = 0
    .loc "src/test-resources/test_inline_asm.c", 23
; frame: 2 bytes (frame-allocated vRegs only)
    phw #0
    tsx
    txa
    clc
    adc #1
    sta $FD
    lda #$01
    adc #0
    sta $FE
    .local __vr1 = 0
    .local @_l_result = 0
    .var @_p_val = 4

    ldax.fp @_p_val
    sta $20
    stx $21
@entry:
    .loc "src/test-resources/test_inline_asm.c", 24
    lda #0
    taz
    staz.fp __vr1
    .loc "src/test-resources/test_inline_asm.c", 26
    ldax @_p_val, sp
    .loc "src/test-resources/test_inline_asm.c", 27
    stax @_l_result, sp
    .loc "src/test-resources/test_inline_asm.c", 28
    ldax.fp __vr1
@__return:
    plz
    plz
    .func_flags stack_call, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc

; function _test_local_access
    proc _test_local_access
    .var _fp = 0
    .loc "src/test-resources/test_inline_asm.c", 31
; frame: 4 bytes (frame-allocated vRegs only)
    phw #0
    phw #0
    tsx
    txa
    clc
    adc #1
    sta $FD
    lda #$01
    adc #0
    sta $FE
    .local __vr0 = 0
    .local __vr3 = 2
    .local @_l_d = 0
    .local @_l_x = 2

@entry:
    .loc "src/test-resources/test_inline_asm.c", 33
    lda #50
    ldx #0
    push .ax
    .var _fp = _fp + 2
    jsr _copy_via_asm
    tsx
    txa
    clc
    adc #1
    sta $FD
    lda #$01
    adc #0
    sta $FE
    plz
    plz
    .var _fp = _fp - 2
    sta $20
    stx $21
    .loc "src/test-resources/test_inline_asm.c", 36
    lda #100
    ldx #0
    push .ax
    .var _fp = _fp + 2
    jsr _copy_via_asm
    tsx
    txa
    clc
    adc #1
    sta $FD
    lda #$01
    adc #0
    sta $FE
    plz
    plz
    .var _fp = _fp - 2
    sta $20
    stx $21
    lda $20
    ldx $21
    stax.fp __vr3
    .loc "src/test-resources/test_inline_asm.c", 37
    ldax.fp __vr3
    cmp.16 .AX, #100
    beq @if_then0
    bra @if_end2
@if_then0:
    .loc "src/test-resources/test_inline_asm.c", 38
    lda #1
    sta $20
    lda _results
    ldx _results+1
    sta $22
    stx $23
    lda #3
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx #0
    pha
    addr_elem.16 __zp_scratch, $22, $24, #1
    pla
    ldy #0
    sta (__zp_scratch),y
@if_end2:
@__return:
    plz
    plz
    plz
    plz
    .func_flags stack_call
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc

; function _main
    proc _main
    .var _fp = 0
    .loc "src/test-resources/test_inline_asm.c", 42
; frame: 2 bytes (frame-allocated vRegs only)
    phw #0
    tsx
    txa
    clc
    adc #1
    sta $FD
    lda #$01
    adc #0
    sta $FE
    .local __vr10 = 0

@entry:
    .loc "src/test-resources/test_inline_asm.c", 43
    lda #0
    ldx #64
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _results
    stx _results+1
    .loc "src/test-resources/test_inline_asm.c", 46
    lda #171
    ldx #0
    push .ax
    .var _fp = _fp + 2
    jsr _store_to_global
    tsx
    txa
    clc
    adc #1
    sta $FD
    lda #$01
    adc #0
    sta $FE
    plz
    plz
    .var _fp = _fp - 2
    .loc "src/test-resources/test_inline_asm.c", 47
    lda _g_value
    ldx _g_value+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #171
    beq @if_then3
    bra @if_end5
@if_then3:
    .loc "src/test-resources/test_inline_asm.c", 48
    lda #1
    sta $20
    lda _results
    ldx _results+1
    sta $22
    stx $23
    lda #0
    sta $24
    sta $25
    lda $20
    ldx #0
    pha
    addr_elem.16 __zp_scratch, $22, $24, #1
    pla
    ldy #0
    sta (__zp_scratch),y
@if_end5:
    .loc "src/test-resources/test_inline_asm.c", 52
    lda #66
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta.fp __vr10
    lda.fp __vr10
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _set_flag
    tsx
    txa
    clc
    adc #1
    sta $FD
    lda #$01
    adc #0
    sta $FE
    plz
    plz
    .var _fp = _fp - 2
    .loc "src/test-resources/test_inline_asm.c", 53
    lda _g_flag
    ldx #0
    sta $20
    lda #66
    sta $22
    lda $20
    ldx #0
    sxt.8
    sta $24
    stx $25
    lda $22
    ldx #0
    ldx #0
    sta $20
    stx $21
    lda $24
    ldx $25
    cmp.16 .AX, $20
    beq @if_then6
    bra @if_end8
@if_then6:
    .loc "src/test-resources/test_inline_asm.c", 54
    lda #1
    sta $20
    lda _results
    ldx _results+1
    sta $22
    stx $23
    lda #1
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx #0
    pha
    addr_elem.16 __zp_scratch, $22, $24, #1
    pla
    ldy #0
    sta (__zp_scratch),y
@if_end8:
    .loc "src/test-resources/test_inline_asm.c", 58
    lda #0
    ldx #0
    push .ax
    .var _fp = _fp + 2
    jsr _store_to_global
    tsx
    txa
    clc
    adc #1
    sta $FD
    lda #$01
    adc #0
    sta $FE
    plz
    plz
    .var _fp = _fp - 2
    .loc "src/test-resources/test_inline_asm.c", 59
    lda _g_value
    ldx _g_value+1
    sta $20
    stx $21
    lda $20
    ora $21
    beq @if_then9
    bra @if_end11
@if_then9:
    .loc "src/test-resources/test_inline_asm.c", 60
    lda #1
    sta $20
    lda _results
    ldx _results+1
    sta $22
    stx $23
    lda #2
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx #0
    pha
    addr_elem.16 __zp_scratch, $22, $24, #1
    pla
    ldy #0
    sta (__zp_scratch),y
@if_end11:
    .loc "src/test-resources/test_inline_asm.c", 64
    jsr _test_local_access
    tsx
    txa
    clc
    adc #1
    sta $FD
    lda #$01
    adc #0
    sta $FE
    .loc "src/test-resources/test_inline_asm.c", 67
    lda #170
    sta $20
    lda _results
    ldx _results+1
    sta $22
    stx $23
    lda #4
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx #0
    pha
    addr_elem.16 __zp_scratch, $22, $24, #1
    pla
    ldy #0
    sta (__zp_scratch),y
@__return:
    plz
    plz
    .func_flags stack_call
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc


__zp_save_buf:

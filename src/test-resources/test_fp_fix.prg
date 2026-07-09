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

_test_result:
    .word 0


; function _my_func
    proc _my_func, W#@_p_x
    .var _fp = 0
    .loc "test_fp_fix.c", 4
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
    .local __vr0 = 0
    .var @_p_x = 4

    ldax.fp @_p_x
    stax.fp __vr0
@entry:
    .loc "test_fp_fix.c", 5
    lda #85
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _test_result
    stx _test_result+1
    .loc "test_fp_fix.c", 6
    ldax.fp __vr0
    add.16 .AX, #1
    sta $22
    stx $23
    lda $22
    ldx $23
@__return:
    plz
    plz
    .func_flags stack_call, leaf
    .reg_clobbers A, X
    .flag_clobbers C, N, Z, V
    endproc

; function _call_fp
    proc _call_fp, W#@_p_fp, W#@_p_val
    .var _fp = 0
    .loc "test_fp_fix.c", 9
; frame: 6 bytes (frame-allocated vRegs only)
    phw #0
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
    .local __vr1 = 4
    .local __vr3 = 2
    .var @_p_fp = 8
    .var @_p_val = 10

    ldax.fp @_p_fp
    stax.fp __vr0
    ldax.fp @_p_val
    stax.fp __vr1
@entry:
    .loc "test_fp_fix.c", 10
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta _test_result
    stx _test_result+1
    .loc "test_fp_fix.c", 11
    ldax.fp __vr1
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    ldax.fp __vr0
    sta @__call_site_0+1
    stx @__call_site_0+2
@__call_site_0:
    jsr $0000
    phx
    pha
    tsx
    txa
    clc
    adc #1
    sta $FD
    lda #$01
    adc #0
    sta $FE
    pla
    plx
    plz
    plz
    .var _fp = _fp - 2
    sta $20
    stx $21
    lda $20
    ldx $21
    stax.fp __vr3
    .loc "test_fp_fix.c", 12
    ldax.fp __vr3
@__return:
    plz
    plz
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
    .loc "test_fp_fix.c", 15
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
    .local __vr0 = 0

@entry:
    .loc "test_fp_fix.c", 16
    lda #42
    ldx #0
    push .ax
    .var _fp = _fp + 2
    ldax #_my_func
    push .ax
    .var _fp = _fp + 2
    jsr _call_fp
    phx
    pha
    tsx
    txa
    clc
    adc #1
    sta $FD
    lda #$01
    adc #0
    sta $FE
    pla
    plx
    plz
    plz
    plz
    plz
    .var _fp = _fp - 4
    sta $20
    stx $21
    lda $20
    ldx $21
    stax.fp __vr0
    .loc "test_fp_fix.c", 19
    lda _test_result
    ldx _test_result+1
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #85
    beq @sc_merge0
    bra @sc_short1
@sc_merge0:
    ldax.fp __vr0
    cmp.16 .AX, #43
    beq @__cmp_set_0
    bra @__cmp_zero_0
@__cmp_set_0:
    lda #1
    ldx #0
    bra @__cmp_done_0
@__cmp_zero_0:
    lda #0
    ldx #0
@__cmp_done_0:
    sta $22
    bra @sc_done2
@sc_short1:
@sc_done2:
    lda $22
    bne @tern_then3
    bra @tern_else4
@tern_then3:
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @tern_end5
@tern_else4:
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@tern_end5:
    lda $22
    ldx $23
@__return:
    plz
    plz
    .func_flags stack_call
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc


__zp_save_buf:
    .res 248

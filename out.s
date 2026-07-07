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

    .extern _bsearch

_test_result:
    .word 16384
_sorted:
    .word 10
    .word 20
    .word 30
    .word 40
    .word 50


; function _cmp
    proc _cmp, W#@_p_a, W#@_p_b
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler.dev_v1.0.4/bin/../lib/include/stddef.h", 4
; frame: 8 bytes (frame-allocated vRegs only)
    phw #0
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
    .local __vr1 = 2
    .local __vr2 = 4
    .local __vr4 = 6
    .local @_l_va = 4
    .local @_l_vb = 6
    .var @_p_a = 10
    .var @_p_b = 12

    ldax.fp @_p_a
    stax.fp __vr0
    ldax.fp @_p_b
    stax.fp __vr1
@entry:
    .loc "src/test-resources/bug183_bsearch.c", 13
    ldax.fp __vr0
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
    stax.fp __vr2
    .loc "src/test-resources/bug183_bsearch.c", 14
    ldax.fp __vr1
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
    stax.fp __vr4
    .loc "src/test-resources/bug183_bsearch.c", 15
    ldax.fp __vr4
    sta __zp_scratch2
    stx __zp_scratch2+1
    ldax.fp __vr2
    sub.16 .AX, __zp_scratch2
    sta $20
    stx $21
    lda $20
    ldx $21
@__return:
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    .func_flags stack_call, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    endproc

; function _test_bsearch
    proc _test_bsearch
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler.dev_v1.0.4/bin/../lib/include/stddef.h", 10
; frame: 28 bytes (frame-allocated vRegs only)
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
    phw #0
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
    .local __vr0 = 2
    .local __vr2 = 0
    .local __vr3 = 4
    .local __vr4 = 6
    .local __vr5 = 8
    .local __vr41 = 10
    .local __vr42 = 12
    .local __vr43 = 14
    .local __vr59 = 16
    .local __vr60 = 18
    .local __vr61 = 20
    .local __vr77 = 22
    .local __vr78 = 24
    .local __vr79 = 26
    .local @_l_found = 0
    .local @_l_key = 2

@entry:
    .loc "src/test-resources/bug183_bsearch.c", 20
    lda #30
    ldz #0
    staz.fp __vr0
    .loc "src/test-resources/bug183_bsearch.c", 21
    leax.fp 2
    stax.fp __vr3
    lda #5
    ldx #0
    stax.fp __vr4
    lda #2
    ldx #0
    stax.fp __vr5
    ldax #_cmp
    sta $28
    stx $29
    ldax.fp __vr5
    sta $2A
    stx $2B
    ldax.fp __vr4
    sta $2C
    stx $2D
    ldax #_sorted
    sta $2E
    stx $2F
    ldax.fp __vr3
    sta $30
    stx $31
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    lda $2A
    ldx $2B
    push .ax
    .var _fp = _fp + 2
    lda $2C
    ldx $2D
    push .ax
    .var _fp = _fp + 2
    lda $2E
    ldx $2F
    push .ax
    .var _fp = _fp + 2
    lda $30
    ldx $31
    push .ax
    .var _fp = _fp + 2
    jsr _bsearch
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
    plz
    plz
    plz
    plz
    plz
    plz
    .var _fp = _fp - 10
    sta $20
    stx $21
    lda $20
    ldx $21
    stax.fp __vr2
    .loc "src/test-resources/bug183_bsearch.c", 24
    ldax.fp __vr2
    stx __zp_scratch
    ora __zp_scratch
    bne @tern_then0
    bra @tern_else1
@tern_then0:
    lda #170
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @tern_end2
@tern_else1:
    lda #1
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@tern_end2:
    lda $22
    ldx $23
    sta $20
    lda _test_result
    ldx _test_result+1
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
    .loc "src/test-resources/bug183_bsearch.c", 25
    lda #2
    ldx #0
    sta $20
    stx $21
    addr_elem.16 $22, #_sorted, $20, #2
    ldax.fp __vr2
    cmp.16 .AX, $22
    beq @tern_then3
    bra @tern_else4
@tern_then3:
    lda #187
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @tern_end5
@tern_else4:
    lda #2
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
    sta $20
    lda _test_result
    ldx _test_result+1
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
    .loc "src/test-resources/bug183_bsearch.c", 26
    ldax.fp __vr2
    bne @if_then6
    cmp #$00
    bne @if_then6
    bra @if_else7
@if_then6:
    .loc "src/test-resources/bug183_bsearch.c", 27
    ldax.fp __vr2
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #30
    beq @tern_then9
    bra @tern_else10
@tern_then9:
    lda #204
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @tern_end11
@tern_else10:
    lda #3
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@tern_end11:
    lda $22
    ldx $23
    sta $20
    lda _test_result
    ldx _test_result+1
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
    bra @if_end8
@if_else7:
    .loc "src/test-resources/bug183_bsearch.c", 29
    lda #153
    sta $20
    lda _test_result
    ldx _test_result+1
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
@if_end8:
    .loc "src/test-resources/bug183_bsearch.c", 33
    lda #10
    ldz #0
    staz.fp __vr0
    .loc "src/test-resources/bug183_bsearch.c", 34
    leax.fp 2
    stax.fp __vr41
    lda #5
    ldx #0
    stax.fp __vr42
    lda #2
    ldx #0
    stax.fp __vr43
    ldax #_cmp
    sta $28
    stx $29
    ldax.fp __vr43
    sta $2A
    stx $2B
    ldax.fp __vr42
    sta $2C
    stx $2D
    ldax #_sorted
    sta $2E
    stx $2F
    ldax.fp __vr41
    sta $30
    stx $31
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    lda $2A
    ldx $2B
    push .ax
    .var _fp = _fp + 2
    lda $2C
    ldx $2D
    push .ax
    .var _fp = _fp + 2
    lda $2E
    ldx $2F
    push .ax
    .var _fp = _fp + 2
    lda $30
    ldx $31
    push .ax
    .var _fp = _fp + 2
    jsr _bsearch
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
    plz
    plz
    plz
    plz
    plz
    plz
    .var _fp = _fp - 10
    sta $20
    stx $21
    lda $20
    ldx $21
    stax.fp __vr2
    .loc "src/test-resources/bug183_bsearch.c", 35
    ldax.fp __vr2
    stx __zp_scratch
    ora __zp_scratch
    bne @sc_merge12
    bra @sc_short13
@sc_merge12:
    ldax.fp __vr2
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #10
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
    sta $24
    bra @sc_done14
@sc_short13:
@sc_done14:
    lda $24
    bne @tern_then15
    bra @tern_else16
@tern_then15:
    lda #170
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @tern_end17
@tern_else16:
    lda #4
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@tern_end17:
    lda $22
    ldx $23
    sta $20
    lda _test_result
    ldx _test_result+1
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
    .loc "src/test-resources/bug183_bsearch.c", 38
    lda #50
    ldz #0
    staz.fp __vr0
    .loc "src/test-resources/bug183_bsearch.c", 39
    leax.fp 2
    stax.fp __vr59
    lda #5
    ldx #0
    stax.fp __vr60
    lda #2
    ldx #0
    stax.fp __vr61
    ldax #_cmp
    sta $28
    stx $29
    ldax.fp __vr61
    sta $2A
    stx $2B
    ldax.fp __vr60
    sta $2C
    stx $2D
    ldax #_sorted
    sta $2E
    stx $2F
    ldax.fp __vr59
    sta $30
    stx $31
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    lda $2A
    ldx $2B
    push .ax
    .var _fp = _fp + 2
    lda $2C
    ldx $2D
    push .ax
    .var _fp = _fp + 2
    lda $2E
    ldx $2F
    push .ax
    .var _fp = _fp + 2
    lda $30
    ldx $31
    push .ax
    .var _fp = _fp + 2
    jsr _bsearch
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
    plz
    plz
    plz
    plz
    plz
    plz
    .var _fp = _fp - 10
    sta $20
    stx $21
    lda $20
    ldx $21
    stax.fp __vr2
    .loc "src/test-resources/bug183_bsearch.c", 40
    ldax.fp __vr2
    stx __zp_scratch
    ora __zp_scratch
    bne @sc_merge18
    bra @sc_short19
@sc_merge18:
    ldax.fp __vr2
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #50
    beq @__cmp_set_1
    bra @__cmp_zero_1
@__cmp_set_1:
    lda #1
    ldx #0
    bra @__cmp_done_1
@__cmp_zero_1:
    lda #0
    ldx #0
@__cmp_done_1:
    sta $24
    bra @sc_done20
@sc_short19:
@sc_done20:
    lda $24
    bne @tern_then21
    bra @tern_else22
@tern_then21:
    lda #170
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @tern_end23
@tern_else22:
    lda #5
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@tern_end23:
    lda $22
    ldx $23
    sta $20
    lda _test_result
    ldx _test_result+1
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
    .loc "src/test-resources/bug183_bsearch.c", 43
    lda #35
    ldz #0
    staz.fp __vr0
    .loc "src/test-resources/bug183_bsearch.c", 44
    leax.fp 2
    stax.fp __vr77
    lda #5
    ldx #0
    stax.fp __vr78
    lda #2
    ldx #0
    stax.fp __vr79
    ldax #_cmp
    sta $28
    stx $29
    ldax.fp __vr79
    sta $2A
    stx $2B
    ldax.fp __vr78
    sta $2C
    stx $2D
    ldax #_sorted
    sta $2E
    stx $2F
    ldax.fp __vr77
    sta $30
    stx $31
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    lda $2A
    ldx $2B
    push .ax
    .var _fp = _fp + 2
    lda $2C
    ldx $2D
    push .ax
    .var _fp = _fp + 2
    lda $2E
    ldx $2F
    push .ax
    .var _fp = _fp + 2
    lda $30
    ldx $31
    push .ax
    .var _fp = _fp + 2
    jsr _bsearch
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
    plz
    plz
    plz
    plz
    plz
    plz
    .var _fp = _fp - 10
    sta $20
    stx $21
    lda $20
    ldx $21
    stax.fp __vr2
    .loc "src/test-resources/bug183_bsearch.c", 45
    ldax.fp __vr2
    stx __zp_scratch
    ora __zp_scratch
    beq @tern_then24
    bra @tern_else25
@tern_then24:
    lda #170
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
    bra @tern_end26
@tern_else25:
    lda #6
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    stx $23
@tern_end26:
    lda $22
    ldx $23
    sta $20
    lda _test_result
    ldx _test_result+1
    sta $22
    stx $23
    lda #5
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
    .loc "src/test-resources/bug183_bsearch.c", 48
    lda #255
    sta $20
    lda _test_result
    ldx _test_result+1
    sta $22
    stx $23
    lda #6
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
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
    plz
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
    .loc "/home/duck/m65/inpg/m65compiler.dev_v1.0.4/bin/../lib/include/stdlib.h", 35
    tsx
    txa
    clc
    adc #1
    sta $FD
    lda #$01
    adc #0
    sta $FE

@entry:
    .loc "src/test-resources/bug183_bsearch.c", 52
    jsr _test_bsearch
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
@__return:
    .func_flags stack_call
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc


__zp_save_buf:

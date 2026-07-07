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

    .extern _mktime

_test_result:
    .word 16384


; function _test_mktime
    proc _test_mktime
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler.dev_v1.0.4/bin/../lib/include/time.h", 6
; frame: 36 bytes (frame-allocated vRegs only)
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
    .local __vr0 = 12
    .local __vr28 = 0
    .local __vr29 = 30
    .local __vr49 = 4
    .local __vr50 = 32
    .local __vr61 = 8
    .local __vr62 = 34
    .local @_l_result1 = 0
    .local @_l_result2 = 4
    .local @_l_result3 = 8
    .local @_l_t = 12

@entry:
    .loc "src/test-resources/bug179_validation.c", 14
    lda #126
    ldx #0
    sta $20
    stx $21
    leax.fp 12
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #10
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "src/test-resources/bug179_validation.c", 15
    lda #6
    ldx #0
    sta $20
    stx $21
    leax.fp 12
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #8
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "src/test-resources/bug179_validation.c", 16
    lda #6
    ldx #0
    sta $20
    stx $21
    leax.fp 12
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #6
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "src/test-resources/bug179_validation.c", 17
    lda #12
    ldx #0
    sta $20
    stx $21
    leax.fp 12
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #4
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "src/test-resources/bug179_validation.c", 18
    lda #0
    sta $20
    sta $21
    leax.fp 12
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #2
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "src/test-resources/bug179_validation.c", 19
    lda #0
    sta $20
    sta $21
    leax.fp 12
    sta $22
    stx $23
    lda $20
    ldx $21
    ldy #0
    sta ($22),y
    txa
    iny
    sta ($22),y
    .loc "src/test-resources/bug179_validation.c", 20
    lda #0
    sta $20
    sta $21
    leax.fp 12
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #12
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "src/test-resources/bug179_validation.c", 21
    lda #0
    sta $20
    sta $21
    leax.fp 12
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #14
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "src/test-resources/bug179_validation.c", 22
    lda #0
    sta $20
    sta $21
    leax.fp 12
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #16
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "src/test-resources/bug179_validation.c", 24
    leax.fp 12
    stax.fp __vr29
    ldax.fp __vr29
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _mktime
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
    stz @__restore_caller_z_0+1
    plz
    plz
@__restore_caller_z_0:
    ldz #0
    .var _fp = _fp - 2
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    staxyz.fp __vr28
    .loc "src/test-resources/bug179_validation.c", 27
    lda #125
    ldx #0
    sta $20
    stx $21
    leax.fp 12
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #10
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "src/test-resources/bug179_validation.c", 28
    lda #0
    sta $20
    sta $21
    leax.fp 12
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #8
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "src/test-resources/bug179_validation.c", 29
    lda #1
    ldx #0
    sta $20
    stx $21
    leax.fp 12
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #6
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "src/test-resources/bug179_validation.c", 30
    lda #0
    sta $20
    sta $21
    leax.fp 12
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #4
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "src/test-resources/bug179_validation.c", 31
    lda #0
    sta $20
    sta $21
    leax.fp 12
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #2
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "src/test-resources/bug179_validation.c", 32
    lda #0
    sta $20
    sta $21
    leax.fp 12
    sta $22
    stx $23
    lda $20
    ldx $21
    ldy #0
    sta ($22),y
    txa
    iny
    sta ($22),y
    .loc "src/test-resources/bug179_validation.c", 34
    leax.fp 12
    stax.fp __vr50
    ldax.fp __vr50
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _mktime
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
    stz @__restore_caller_z_1+1
    plz
    plz
@__restore_caller_z_1:
    ldz #0
    .var _fp = _fp - 2
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    staxyz.fp __vr49
    .loc "src/test-resources/bug179_validation.c", 37
    lda #124
    ldx #0
    sta $20
    stx $21
    leax.fp 12
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #10
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "src/test-resources/bug179_validation.c", 38
    lda #1
    ldx #0
    sta $20
    stx $21
    leax.fp 12
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #8
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "src/test-resources/bug179_validation.c", 39
    lda #29
    ldx #0
    sta $20
    stx $21
    leax.fp 12
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #6
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "src/test-resources/bug179_validation.c", 41
    leax.fp 12
    stax.fp __vr62
    ldax.fp __vr62
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _mktime
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
    stz @__restore_caller_z_2+1
    plz
    plz
@__restore_caller_z_2:
    ldz #0
    .var _fp = _fp - 2
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    staxyz.fp __vr61
    .loc "src/test-resources/bug179_validation.c", 44
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    ldy #0
    ldz #0
    sta $22
    stx $23
    sty $24
    stz $25
    ldaxyz.fp __vr28
    cmp.32 .AXYZ, $22
    bne @tern_then0
    bra @tern_else1
@tern_then0:
    lda #170
    ldx #0
    sta $20
    stx $21
    bra @tern_end2
@tern_else1:
    lda #1
    ldx #0
    sta $20
    stx $21
@tern_end2:
    lda $20
    ldx $21
    sta $22
    lda _test_result
    ldx _test_result+1
    sta $20
    stx $21
    lda #0
    sta $24
    sta $25
    lda $22
    ldx #0
    pha
    addr_elem.16 __zp_scratch, $20, $24, #1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "src/test-resources/bug179_validation.c", 45
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    ldy #0
    ldz #0
    sta $22
    stx $23
    sty $24
    stz $25
    ldaxyz.fp __vr49
    cmp.32 .AXYZ, $22
    bne @tern_then3
    bra @tern_else4
@tern_then3:
    lda #170
    ldx #0
    sta $20
    stx $21
    bra @tern_end5
@tern_else4:
    lda #2
    ldx #0
    sta $20
    stx $21
@tern_end5:
    lda $20
    ldx $21
    sta $22
    lda _test_result
    ldx _test_result+1
    sta $20
    stx $21
    lda #1
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx #0
    pha
    addr_elem.16 __zp_scratch, $20, $24, #1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "src/test-resources/bug179_validation.c", 46
    lda #0
    sta $20
    sta $21
    lda $20
    ldx $21
    ldy #0
    ldz #0
    sta $22
    stx $23
    sty $24
    stz $25
    ldaxyz.fp __vr61
    cmp.32 .AXYZ, $22
    bne @tern_then6
    bra @tern_else7
@tern_then6:
    lda #170
    ldx #0
    sta $20
    stx $21
    bra @tern_end8
@tern_else7:
    lda #3
    ldx #0
    sta $20
    stx $21
@tern_end8:
    lda $20
    ldx $21
    sta $22
    lda _test_result
    ldx _test_result+1
    sta $20
    stx $21
    lda #2
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx #0
    pha
    addr_elem.16 __zp_scratch, $20, $24, #1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "src/test-resources/bug179_validation.c", 47
    ldaxyz.fp __vr49
    sta __zp_scratch2
    stx __zp_scratch2+1
    ldaxyz.fp __vr28
    cmp.32 .AXYZ, __zp_scratch2
    bne @tern_then9
    bra @tern_else10
@tern_then9:
    lda #170
    ldx #0
    sta $20
    stx $21
    bra @tern_end11
@tern_else10:
    lda #4
    ldx #0
    sta $20
    stx $21
@tern_end11:
    lda $20
    ldx $21
    sta $22
    lda _test_result
    ldx _test_result+1
    sta $20
    stx $21
    lda #3
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx #0
    pha
    addr_elem.16 __zp_scratch, $20, $24, #1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "src/test-resources/bug179_validation.c", 50
    lda #255
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
    .loc "/home/duck/m65/inpg/m65compiler.dev_v1.0.4/bin/../lib/include/stdio.h", 4
    tsx
    txa
    clc
    adc #1
    sta $FD
    lda #$01
    adc #0
    sta $FE

@entry:
    .loc "src/test-resources/bug179_validation.c", 54
    jsr _test_mktime
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

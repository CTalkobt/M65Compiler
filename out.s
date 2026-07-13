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

_res:
    .word 16384


; function _main
    proc _main
    .var _fp = 0
    .loc "src/test-resources/test_bitfield_mmemu.c", 15
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
    .frameptr_zp $FD
    .local __vr0 = 0
    .local __vr46 = 2
    .local @_l_f = 0
    .local @_l_wf = 2
; .debug_var: __main @_l_f offset=0 size=2 type=int16 scope=local
; .debug_var: __main @_l_wf offset=2 size=2 type=int16 scope=local

@entry:
    .loc "src/test-resources/test_bitfield_mmemu.c", 17
    lda #1
    sta $20
    ldax.fp __vr0
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx #0
    bfins __zp_scratch2, #0, #1
    sta $22
    lda $22
    ldx #0
    stax.fp __vr0
    .loc "src/test-resources/test_bitfield_mmemu.c", 18
    lda #5
    sta $20
    ldax.fp __vr0
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx #0
    bfins __zp_scratch2, #1, #3
    sta $22
    lda $22
    ldx #0
    stax.fp __vr0
    .loc "src/test-resources/test_bitfield_mmemu.c", 19
    lda #12
    sta $20
    ldax.fp __vr0
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx #0
    bfins __zp_scratch2, #4, #4
    sta $22
    lda $22
    ldx #0
    stax.fp __vr0
    .loc "src/test-resources/test_bitfield_mmemu.c", 21
    leax.fp 0
    sta $20
    stx $21
    ldy #0
    lda ($20),y
    ldx #0
    sta $22
    lda $22
    ldx #0
    bfext #0, #1
    sta $20
    lda _res
    ldx _res+1
    sta $22
    stx $23
    lda #0
    sta $24
    sta $25
    lda $20
    ldx #0
    pha
    lda $24
    ldx $25
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $22
    ldx $22+1
    add.16 .AX, __zp_scratch3
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "src/test-resources/test_bitfield_mmemu.c", 22
    leax.fp 0
    sta $20
    stx $21
    ldy #0
    lda ($20),y
    ldx #0
    sta $22
    lda $22
    ldx #0
    bfext #1, #3
    sta $20
    lda _res
    ldx _res+1
    sta $22
    stx $23
    lda #1
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx #0
    pha
    lda $24
    ldx $25
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $22
    ldx $22+1
    add.16 .AX, __zp_scratch3
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "src/test-resources/test_bitfield_mmemu.c", 23
    leax.fp 0
    sta $20
    stx $21
    ldy #0
    lda ($20),y
    ldx #0
    sta $22
    lda $22
    ldx #0
    bfext #4, #4
    sta $20
    lda _res
    ldx _res+1
    sta $22
    stx $23
    lda #2
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx #0
    pha
    lda $24
    ldx $25
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $22
    ldx $22+1
    add.16 .AX, __zp_scratch3
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "src/test-resources/test_bitfield_mmemu.c", 25
    leax.fp 0
    sta $20
    stx $21
    ldy #0
    lda ($20),y
    ldx #0
    sta $22
    lda $22
    ldx #0
    bfext #1, #3
    sta $20
    lda $20
    ldx #0
    sta $22
    lda $22
    inc a
    sta $20
    ldax.fp __vr0
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx #0
    bfins __zp_scratch2, #1, #3
    sta $22
    lda $22
    ldx #0
    stax.fp __vr0
    .loc "src/test-resources/test_bitfield_mmemu.c", 26
    leax.fp 0
    sta $20
    stx $21
    ldy #0
    lda ($20),y
    ldx #0
    sta $22
    lda $22
    ldx #0
    bfext #1, #3
    sta $20
    lda _res
    ldx _res+1
    sta $22
    stx $23
    lda #3
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx #0
    pha
    lda $24
    ldx $25
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $22
    ldx $22+1
    add.16 .AX, __zp_scratch3
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "src/test-resources/test_bitfield_mmemu.c", 29
    lda #244
    ldx #1
    sta $20
    stx $21
    ldax.fp __vr46
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx $21
    bfins16 __zp_scratch2, #0, #10
    sta $22
    stx $23
    lda $22
    ldx $23
    stax.fp __vr46
    .loc "src/test-resources/test_bitfield_mmemu.c", 30
    lda #30
    ldx #0
    sta $20
    stx $21
    ldax.fp __vr46
    sta __zp_scratch2
    stx __zp_scratch2+1
    lda $20
    ldx $21
    bfins16 __zp_scratch2, #10, #6
    sta $22
    stx $23
    lda $22
    ldx $23
    stax.fp __vr46
    .loc "src/test-resources/test_bitfield_mmemu.c", 31
    leax.fp 2
    sta $20
    stx $21
    ldy #0
    lda ($20),y
    pha
    iny
    lda ($20),y
    tax
    pla
    sta $22
    stx $23
    lda $22
    ldx $23
    bfext16 #0, #10
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda _res
    ldx _res+1
    sta $20
    stx $21
    lda #4
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx #0
    pha
    lda $24
    ldx $25
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
    add.16 .AX, __zp_scratch3
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "src/test-resources/test_bitfield_mmemu.c", 32
    leax.fp 2
    sta $20
    stx $21
    ldy #0
    lda ($20),y
    pha
    iny
    lda ($20),y
    tax
    pla
    sta $22
    stx $23
    lda $22
    ldx $23
    bfext16 #10, #6
    sta $20
    stx $21
    lda $20
    ldx $21
    sta $22
    lda _res
    ldx _res+1
    sta $20
    stx $21
    lda #5
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx #0
    pha
    lda $24
    ldx $25
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
    add.16 .AX, __zp_scratch3
    sta __zp_scratch
    stx __zp_scratch+1
    pla
    ldy #0
    sta (__zp_scratch),y
    .loc "src/test-resources/test_bitfield_mmemu.c", 34
    brk
@__return:
    plz
    plz
    plz
    plz
    .func_flags stack_call, leaf
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc


__zp_save_buf:

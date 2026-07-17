* = $2000
    __sp_base = $0101
    jsr _main
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

_result:
    .word 16384


; function _main
    proc _main
    .var _fp = 0
    .loc "test_offset0_simple.c", 6
; frame: 2 bytes (frame-allocated vRegs only)
    phw #0
    tsy
    tsx
    inx
    bne @__fp_no_carry_0
    iny
@__fp_no_carry_0:
    stx $FD
    sty $FE
    .frameptr_zp $FD
    leax.fp 0
    sta $08
    stx $08+1
    .local __vr0 = 0
    .local @_l_arr = 0
; .debug_var: __main @_l_arr offset=0 size=2 type=int8 scope=local

@entry:
    .loc "test_offset0_simple.c", 7
    lda $08
    ldx $08+1
    sta $20
    stx $21
    lda #170
    ldx #0
    sta $22
    stx $23
    struct_elem.16 $24, $20, #0
    lda $22
    ldy #0
    sta ($24),y
    lda #187
    ldx #0
    sta $22
    stx $23
    struct_elem.16 $24, $20, #1
    lda $22
    ldy #0
    sta ($24),y
    .loc "test_offset0_simple.c", 8
    lda $08
    ldx $08+1
    sta $20
    stx $21
    lda #0
    sta $22
    sta $23
    lda $22
    ldx $23
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
    add.16 .AX, __zp_scratch3
    sta $24
    stx $24+1
    ldy #0
    lda ($24),y
    ldx #0
    sta $20
    lda _result
    ldx _result+1
    sta $22
    stx $23
    lda #0
    sta $24
    sta $25
    lda $24
    ldx $25
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $22
    ldx $22+1
    add.16 .AX, __zp_scratch3
    sta $26
    stx $26+1
    lda $20
    ldy #0
    sta ($26),y
    .loc "test_offset0_simple.c", 9
    lda $08
    ldx $08+1
    sta $20
    stx $21
    lda #1
    ldx #0
    sta $22
    stx $23
    lda $22
    ldx $23
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
    add.16 .AX, __zp_scratch3
    sta $24
    stx $24+1
    ldy #0
    lda ($24),y
    ldx #0
    sta $20
    lda _result
    ldx _result+1
    sta $22
    stx $23
    lda #1
    ldx #0
    sta $24
    stx $25
    lda $24
    ldx $25
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $22
    ldx $22+1
    add.16 .AX, __zp_scratch3
    sta $26
    stx $26+1
    lda $20
    ldy #0
    sta ($26),y
    .loc "test_offset0_simple.c", 10
    lda #255
    sta $20
    lda _result
    ldx _result+1
    sta $22
    stx $23
    lda #2
    ldx #0
    sta $24
    stx $25
    lda $24
    ldx $25
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $22
    ldx $22+1
    add.16 .AX, __zp_scratch3
    sta $26
    stx $26+1
    lda $20
    ldy #0
    sta ($26),y
@__return:
    plz
    plz
    .func_flags stack_call, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    endproc


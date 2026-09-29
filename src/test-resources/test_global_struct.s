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

    .global _main

    .segment "code"

; function _main
; SAC inline storage: 7 bytes
    _main__local_0: .word 0
    _main__local_3: .word 0
    _main__local_11: .word 0
    _main__local_27: .word 0
    _main__local_40: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_global_struct.c", 10
    .local @_l_d = 2
    .local @_l_p = 0
; .debug_var: __main @_l_d offset=2 size=2 type=int16 scope=local
; .debug_var: __main @_l_p offset=0 size=2 type=ptr scope=local

@entry:
    .loc "test_global_struct.c", 12
    lda #5
    sta $20
    leax.local 2
    sta $22
    stx $23
    lda $20
    ldy #0
    sta ($22),y
    .loc "test_global_struct.c", 13
    lda #244
    ldx #1
    sta $20
    stx $21
    leax.local 2
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #1
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_global_struct.c", 14
    lda #99
    ldx #0
    sta $20
    stx $21
    leax.local 2
    sta $22
    stx $23
    lda $22
    ldx $23
    add.16 .AX, #3
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    .loc "test_global_struct.c", 16
    leax.local 2
    sta $20
    stx $21
    ldy #0
    lda ($20),y
    ldx #0
    sta $22
    lda #5
    sta $20
    lda $22
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $24
    ldx $25
    cmp.16 .AX, $22
    bne @if_then0
    bra @if_end2
@if_then0:
    lda #1
    ldx #0
    bra @__return
@if_end2:
    .loc "test_global_struct.c", 17
    leax.local 2
    sta $20
    stx $21
    lda $20
    ldx $21
    add.16 .AX, #1
    sta $22
    stx $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #500
    bne @if_then3
    bra @if_end5
@if_then3:
    lda #2
    ldx #0
    bra @__return
@if_end5:
    .loc "test_global_struct.c", 18
    leax.local 2
    sta $20
    stx $21
    lda $20
    ldx $21
    add.16 .AX, #3
    sta $22
    stx $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #99
    bne @if_then6
    bra @if_end8
@if_then6:
    lda #3
    ldx #0
    bra @__return
@if_end8:
    .loc "test_global_struct.c", 21
    leax.local 2
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_27
    stx _main__local_27+1
    .loc "test_global_struct.c", 22
    lda #232
    ldx #3
    sta $20
    stx $21
    lda _main__local_27
    ldx _main__local_27+1
    add.16 .AX, #1
    sta $22
    stx $23
    lda $20
    ldx $21
    ldy #0
    sta ($22),y
    txa
    iny
    sta ($22),y
    .loc "test_global_struct.c", 23
    leax.local 2
    sta $20
    stx $21
    lda $20
    ldx $21
    add.16 .AX, #1
    sta $22
    stx $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #1000
    bne @if_then9
    bra @if_end11
@if_then9:
    lda #4
    ldx #0
    bra @__return
@if_end11:
    .loc "test_global_struct.c", 26
    leax.local 2
    sta $20
    stx $21
    lda $20
    ldx $21
    add.16 .AX, #1
    sta $22
    stx $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    leax.local 2
    sta $22
    stx $23
    ldy #0
    lda ($22),y
    ldx #0
    sta $24
    lda $24
    ldx #0
    ldx #0
    sta $22
    stx $23
    lda $20
    clc
    adc $22
    sta $24
    lda $21
    adc $22+1
    sta $25
    leax.local 2
    sta $20
    stx $21
    lda $20
    ldx $21
    add.16 .AX, #3
    sta $22
    stx $23
    lda $24
    ldx $25
    ldy #0
    sta ($22),y
    txa
    iny
    sta ($22),y
    .loc "test_global_struct.c", 27
    leax.local 2
    sta $20
    stx $21
    lda $20
    ldx $21
    add.16 .AX, #3
    sta $22
    stx $23
    ldy #0
    lda ($22),y
    pha
    iny
    lda ($22),y
    tax
    pla
    sta $20
    stx $21
    lda $20
    ldx $21
    cmp.16 .AX, #1005
    bne @if_then12
    bra @if_end14
@if_then12:
    lda #5
    ldx #0
    bra @__return
@if_end14:
    .loc "test_global_struct.c", 29
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 7
    endproc


__zp_save_buf:

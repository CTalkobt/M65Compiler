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
; SAC inline storage: 30 bytes
    _main__local_0: .word 0
    _main__local_2: .word 0
    _main__local_4: .word 0
    _main__local_8: .word 0
    _main__local_17: .word 0
    _main__local_25: .word 0
    _main__local_34: .word 0
    _main__local_42: .word 0
    _main__local_55: .word 0
    _main__local_57: .word 0
    _main__local_82: .word 0
    _main__local_84: .word 0
    _main__local_94: .word 0
    proc _main
; Phase 51: zero-alloc leaf (all parameters constant)
    .var _fp = 0
    .loc "test_ptr_precedence.c", 4
    .local @_l_a = 4
    .local @_l_arr = 24
    .local @_l_b = 6
    .local @_l_c = 8
    .local @_l_cp = 20
    .local @_l_cv = 18
    .local @_l_d = 10
    .local @_l_e = 12
    .local @_l_f = 16
    .local @_l_g = 22
    .local @_l_p = 2
    .local @_l_q = 14
    .local @_l_val = 0
; .debug_var: __main @_l_a offset=4 size=2 type=int16 scope=local
; .debug_var: __main @_l_arr offset=24 size=2 type=int16 scope=local
; .debug_var: __main @_l_b offset=6 size=2 type=int16 scope=local
; .debug_var: __main @_l_c offset=8 size=2 type=int16 scope=local
; .debug_var: __main @_l_cp offset=20 size=2 type=ptr scope=local
; .debug_var: __main @_l_cv offset=18 size=2 type=int8 scope=local
; .debug_var: __main @_l_d offset=10 size=2 type=int16 scope=local
; .debug_var: __main @_l_e offset=12 size=2 type=int16 scope=local
; .debug_var: __main @_l_f offset=16 size=2 type=int16 scope=local
; .debug_var: __main @_l_g offset=22 size=2 type=int8 scope=local
; .debug_var: __main @_l_p offset=2 size=2 type=ptr scope=local
; .debug_var: __main @_l_q offset=14 size=2 type=ptr scope=local
; .debug_var: __main @_l_val offset=0 size=2 type=int16 scope=local

@entry:
    .loc "test_ptr_precedence.c", 5
    lda #42
    sta _main__local_0
    lda #0
    sta _main__local_0+1
    .loc "test_ptr_precedence.c", 6
    leax.local 0
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_2
    stx _main__local_2+1
    .loc "test_ptr_precedence.c", 9
    lda _main__local_2
    ldx _main__local_2+1
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
    sta _main__local_4
    stx _main__local_4+1
    .loc "test_ptr_precedence.c", 10
    lda _main__local_4
    ldx _main__local_4+1
    cmp.16 .AX, #42
    bne @if_then0
    bra @if_end2
@if_then0:
    lda #1
    ldx #0
    bra @__return
@if_end2:
    .loc "test_ptr_precedence.c", 13
    lda _main__local_2
    ldx _main__local_2+1
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
    sta $22
    stx $23
    lda $22
    clc
    adc #1
    sta $20
    lda $23
    adc #0
    sta $21
    lda _main__local_2
    ldx _main__local_2+1
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    lda $22
    ldx $23
    sta _main__local_8
    stx _main__local_8+1
    .loc "test_ptr_precedence.c", 14
    lda _main__local_8
    ldx _main__local_8+1
    cmp.16 .AX, #42
    bne @if_then3
    bra @if_end5
@if_then3:
    lda #2
    ldx #0
    bra @__return
@if_end5:
    .loc "test_ptr_precedence.c", 15
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, #43
    bne @if_then6
    bra @if_end8
@if_then6:
    lda #3
    ldx #0
    bra @__return
@if_end8:
    .loc "test_ptr_precedence.c", 18
    lda _main__local_2
    ldx _main__local_2+1
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
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    lda _main__local_2
    ldx _main__local_2+1
    sta $20
    stx $21
    lda $22
    ldx $23
    ldy #0
    sta ($20),y
    txa
    iny
    sta ($20),y
    lda $22
    ldx $23
    sta _main__local_17
    stx _main__local_17+1
    .loc "test_ptr_precedence.c", 19
    lda _main__local_17
    ldx _main__local_17+1
    cmp.16 .AX, #44
    bne @if_then9
    bra @if_end11
@if_then9:
    lda #4
    ldx #0
    bra @__return
@if_end11:
    .loc "test_ptr_precedence.c", 20
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, #44
    bne @if_then12
    bra @if_end14
@if_then12:
    lda #5
    ldx #0
    bra @__return
@if_end14:
    .loc "test_ptr_precedence.c", 23
    lda _main__local_2
    ldx _main__local_2+1
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
    sta $22
    stx $23
    lda $22
    sec
    sbc #1
    sta $20
    lda $23
    sbc #0
    sta $21
    lda _main__local_2
    ldx _main__local_2+1
    sta $24
    stx $25
    lda $20
    ldx $21
    ldy #0
    sta ($24),y
    txa
    iny
    sta ($24),y
    lda $22
    ldx $23
    sta _main__local_25
    stx _main__local_25+1
    .loc "test_ptr_precedence.c", 24
    lda _main__local_25
    ldx _main__local_25+1
    cmp.16 .AX, #44
    bne @if_then15
    bra @if_end17
@if_then15:
    lda #6
    ldx #0
    bra @__return
@if_end17:
    .loc "test_ptr_precedence.c", 25
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, #43
    bne @if_then18
    bra @if_end20
@if_then18:
    lda #7
    ldx #0
    bra @__return
@if_end20:
    .loc "test_ptr_precedence.c", 28
    lda _main__local_2
    ldx _main__local_2+1
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
    sec
    sbc #1
    sta $22
    lda $21
    sbc #0
    sta $23
    lda _main__local_2
    ldx _main__local_2+1
    sta $20
    stx $21
    lda $22
    ldx $23
    ldy #0
    sta ($20),y
    txa
    iny
    sta ($20),y
    lda $22
    ldx $23
    sta _main__local_34
    stx _main__local_34+1
    .loc "test_ptr_precedence.c", 29
    lda _main__local_34
    ldx _main__local_34+1
    cmp.16 .AX, #42
    bne @if_then21
    bra @if_end23
@if_then21:
    lda #8
    ldx #0
    bra @__return
@if_end23:
    .loc "test_ptr_precedence.c", 30
    lda _main__local_0
    ldx _main__local_0+1
    cmp.16 .AX, #42
    bne @if_then24
    bra @if_end26
@if_then24:
    lda #9
    ldx #0
    bra @__return
@if_end26:
    .loc "test_ptr_precedence.c", 34
    lda #10
    ldx #0
    sta $20
    stx $21
    leax.local 24
    sta $22
    stx $23
    lda #0
    sta $24
    sta $25
    lda $20
    ldx $21
    pha
    phx
    lda $24
    ldx $25
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $22
    ldx $22+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    .loc "test_ptr_precedence.c", 35
    lda #20
    ldx #0
    sta $20
    stx $21
    leax.local 24
    sta $22
    stx $23
    lda #1
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx $21
    pha
    phx
    lda $24
    ldx $25
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $22
    ldx $22+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    .loc "test_ptr_precedence.c", 36
    lda #30
    ldx #0
    sta $20
    stx $21
    leax.local 24
    sta $22
    stx $23
    lda #2
    ldx #0
    sta $24
    stx $25
    lda $20
    ldx $21
    pha
    phx
    lda $24
    ldx $25
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $22
    ldx $22+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    .loc "test_ptr_precedence.c", 37
    leax.local 24
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_55
    stx _main__local_55+1
    .loc "test_ptr_precedence.c", 38
    lda _main__local_55
    ldx _main__local_55+1
    sta $20
    stx $21
    lda $20
    ldx $21
    add.16 .AX, #2
    sta $22
    stx $23
    sta _main__local_55
    stx _main__local_55+1
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
    sta _main__local_57
    stx _main__local_57+1
    .loc "test_ptr_precedence.c", 39
    lda _main__local_57
    ldx _main__local_57+1
    cmp.16 .AX, #10
    bne @if_then27
    bra @if_end29
@if_then27:
    lda #10
    ldx #0
    bra @__return
@if_end29:
    .loc "test_ptr_precedence.c", 42
    lda #5
    ldx #0
    sta $20
    stx $21
    leax.local 24
    sta $22
    stx $23
    lda #0
    sta $24
    sta $25
    lda $20
    ldx $21
    pha
    phx
    lda $24
    ldx $25
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $22
    ldx $22+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    .loc "test_ptr_precedence.c", 43
    leax.local 24
    sta $20
    stx $21
    lda #0
    sta $22
    sta $23
    lda $22
    ldx $23
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $26
    stx $27
    lda $26
    ldx $27
    sta $20
    stx $21
    lda $20
    clc
    adc #1
    sta $22
    lda $21
    adc #0
    sta $23
    leax.local 24
    sta $20
    stx $21
    lda #0
    sta $24
    sta $25
    lda $22
    ldx $23
    pha
    phx
    lda $24
    ldx $25
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    plx
    pla
    ldy #0
    sta (__zp_scratch),y
    txa
    iny
    sta (__zp_scratch),y
    .loc "test_ptr_precedence.c", 44
    leax.local 24
    sta $20
    stx $21
    lda #0
    sta $22
    sta $23
    lda $22
    ldx $23
    mul.16 .AX, #2
    sta __zp_scratch3
    stx __zp_scratch3+1
    lda $20
    ldx $20+1
    clc
    adc __zp_scratch3
    pha
    txa
    adc __zp_scratch3+1
    tax
    pla
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    pha
    iny
    lda (__zp_scratch),y
    tax
    pla
    sta $26
    stx $27
    lda $26
    ldx $27
    cmp.16 .AX, #6
    bne @if_then30
    bra @if_end32
@if_then30:
    lda #11
    ldx #0
    bra @__return
@if_end32:
    .loc "test_ptr_precedence.c", 47
    lda #100
    sta _main__local_82
    lda #0
    sta _main__local_82+1
    .loc "test_ptr_precedence.c", 48
    leax.local 18
    sta $20
    stx $21
    lda $20
    ldx $21
    sta _main__local_84
    stx _main__local_84+1
    .loc "test_ptr_precedence.c", 49
    lda _main__local_84
    ldx _main__local_84+1
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $20
    lda $20
    ldx #0
    sta $22
    lda $22
    inc a
    sta $20
    lda _main__local_84
    ldx _main__local_84+1
    sta $22
    stx $23
    lda $20
    ldy #0
    sta ($22),y
    .loc "test_ptr_precedence.c", 50
    lda #101
    sta $20
    lda _main__local_82
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    cmp.16 .AX, $24
    bne @if_then33
    bra @if_end35
@if_then33:
    lda #12
    ldx #0
    bra @__return
@if_end35:
    .loc "test_ptr_precedence.c", 52
    lda _main__local_84
    ldx _main__local_84+1
    sta __zp_scratch
    stx __zp_scratch+1
    ldy #0
    lda (__zp_scratch),y
    ldx #0
    sta $20
    lda $20
    inc a
    sta $22
    lda _main__local_84
    ldx _main__local_84+1
    sta $20
    stx $21
    lda $22
    ldy #0
    sta ($20),y
    lda $22
    ldx #0
    sta _main__local_94
    .loc "test_ptr_precedence.c", 53
    lda #102
    sta $20
    lda _main__local_94
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    cmp.16 .AX, $24
    bne @if_then36
    bra @if_end38
@if_then36:
    lda #13
    ldx #0
    bra @__return
@if_end38:
    .loc "test_ptr_precedence.c", 54
    lda #102
    sta $20
    lda _main__local_82
    ldx #0
    sta $22
    stx $23
    lda $20
    ldx #0
    ldx #0
    sta $24
    stx $25
    lda $22
    ldx $23
    cmp.16 .AX, $24
    bne @if_then39
    bra @if_end41
@if_then39:
    lda #14
    ldx #0
    bra @__return
@if_end41:
    .loc "test_ptr_precedence.c", 56
    lda #0
    ldx #0
@__return:
    rts
    .func_flags stack_call, static_alloc, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    .frame_size 30
    endproc


__zp_save_buf:

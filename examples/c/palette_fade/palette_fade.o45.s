    .o45
    .extern __sp_base
    .weak __static_chain
    .weak __zp_scratch
    .weak __zp_scratch2
    .weak __zp_scratch3
    .weak __zp_scratch4
    __static_chain = $06
    __zp_scratch = $08
    __zp_scratch2 = $0A
    __zp_scratch3 = $0C
    __zp_scratch4 = $0E

    .extern _apply_fade
    .extern _save_palette
    .extern _unlock_viciv

    .global _delay_frame
    .global _main

    .segment "code"

; function _delay_frame
    proc _delay_frame
    .var _fp = 0
    .loc "palette_fade.c", 16
; frame: 4 bytes (frame-allocated vRegs only)
    phw #0
    phw #0
    .local __vr0 = 0
    .local __vr1 = 2
    .local @_l_i = 0
    .local @_l_j = 2

@entry:
    .loc "palette_fade.c", 19
    lda #0  ; [CONST → v2(I16/PTR,ZP:$20) val=0 → direct frame store]
    taz  ; [CONST → v2(I16/PTR,ZP:$20) val=0 → direct frame store]
    staz.fp __vr0  ; [CONST → v2(I16/PTR,ZP:$20) val=0 → direct frame store]
@for_cond0:
    ldax.fp __vr0  ; [loadVreg v0(I16/PTR,frame)]
    cmp.16 .AX, #10
    bcc @for_body1
    bra @for_end3
@for_body1:
    .loc "palette_fade.c", 20
    lda #0  ; [CONST → v5(I16/PTR,ZP:$24) val=0 → direct frame store]
    taz  ; [CONST → v5(I16/PTR,ZP:$24) val=0 → direct frame store]
    staz.fp __vr1  ; [CONST → v5(I16/PTR,ZP:$24) val=0 → direct frame store]
@for_cond4:
    ldax.fp __vr1  ; [loadVreg v1(I16/PTR,frame)]
    cmp.16 .AX, #3000
    bcc @for_body5
    bra @for_end7
@for_body5:
@for_inc6:
    ldax.fp __vr1  ; [loadVreg v1(I16/PTR,frame)]
    sta $2A
    stx $2B
    lda $2A  ; [ADD → v9(I16/PTR,ZP:$2C) store-fused add/sub.16]
    clc
    adc #1
    sta $2C
    lda $2B
    adc #0
    sta $2D
    lda $2C  ; [loadVreg v9(I16/PTR,ZP:$2C)]
    ldx $2D  ; [loadVreg v9(I16/PTR,ZP:$2C)]
    stax.fp __vr1
    bra @for_cond4
@for_end7:
@for_inc2:
    .loc "palette_fade.c", 19
    ldax.fp __vr0  ; [loadVreg v0(I16/PTR,frame)]
    sta $2E
    stx $2F
    lda $2E  ; [ADD → v11(I16/PTR,ZP:$30) store-fused add/sub.16]
    clc
    adc #1
    sta $30
    lda $2F
    adc #0
    sta $31
    lda $30  ; [loadVreg v11(I16/PTR,ZP:$30)]
    ldx $31  ; [loadVreg v11(I16/PTR,ZP:$30)]
    stax.fp __vr0
    bra @for_cond0
@for_end3:
@__return:
    plz
    plz
    plz
    plz
    .func_flags zp_call, leaf
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    endproc

; function _main
    proc _main
    .var _fp = 0
    .loc "palette_fade.c", 24
; frame: 64 bytes (frame-allocated vRegs only)
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
    .local __vr0 = 0
    .local __vr2 = 2
    .local __vr3 = 16
    .local __vr4 = 22
    .local __vr5 = 24
    .local __vr6 = 4
    .local __vr7 = 26
    .local __vr8 = 28
    .local __vr9 = 30
    .local __vr10 = 6
    .local __vr11 = 32
    .local __vr12 = 34
    .local __vr13 = 36
    .local __vr14 = 38
    .local __vr15 = 8
    .local __vr16 = 40
    .local __vr17 = 46
    .local __vr18 = 48
    .local __vr19 = 10
    .local __vr20 = 50
    .local __vr21 = 52
    .local __vr22 = 54
    .local __vr23 = 12
    .local __vr24 = 56
    .local __vr25 = 58
    .local __vr26 = 60
    .local __vr27 = 62
    .local __vr28 = 14
    .local __vr29 = 18
    .local __vr30 = 20
    .local __vr31 = 42
    .local __vr32 = 44
    .local @_l_border_color = 0
    .local @_l_level = 2

@entry:
    .loc "palette_fade.c", 25
    .loc "palette_fade.c", 28
    lda #1
    sta $D020  ; [STORE → $D020]
    .loc "palette_fade.c", 31
    lda #2
    sta $D020  ; [STORE → $D020]
    .loc "palette_fade.c", 34
    jsr _unlock_viciv
    .loc "palette_fade.c", 37
    lda #5
    sta $D020  ; [STORE → $D020]
    .loc "palette_fade.c", 40
    jsr _save_palette
    .loc "palette_fade.c", 43
    lda #7
    sta $D020  ; [STORE → $D020]
@while_body8_ph:
    .loc "palette_fade.c", 59
    lda #255  ; [CONST → v33(I16/PTR,ZP:$20) val=255]
    ldx #0  ; [CONST → v33(I16/PTR,ZP:$20) val=255]
    sta $20
    stx $21
    .loc "palette_fade.c", 75
    lda #0  ; [CONST → v34(I16/PTR,ZP:$22) val=0]
    sta $22
    sta $23
    .loc "palette_fade.c", 49
    lda #255  ; [CONST → v35(I8,ZP:$24) val=255]
    sta $24  ; [storeVreg v35(I8,ZP:$24)]
    .loc "palette_fade.c", 53
    lda #16  ; [CONST → v36(I8,ZP:$26) val=16]
    sta $26  ; [storeVreg v36(I8,ZP:$26)]
    .loc "palette_fade.c", 65
    lda #0  ; [CONST → v37(I8,ZP:$28) val=0]
    sta $28  ; [storeVreg v37(I8,ZP:$28)]
    .loc "palette_fade.c", 69
    lda #16  ; [CONST → v38(I8,ZP:$2A) val=16]
    sta $2A  ; [storeVreg v38(I8,ZP:$2A)]
    lda $24  ; [loadVreg v35(I8,ZP:$24)]
    ldx #0  ; [zext I8→I16]
    ldx #0
    stax.fp __vr6
    lda $26  ; [loadVreg v36(I8,ZP:$26)]
    ldx #0  ; [zext I8→I16]
    ldx #0
    stax.fp __vr10
    lda $20  ; [loadVreg v33(I16/PTR,ZP:$20)]
    ldx $21  ; [loadVreg v33(I16/PTR,ZP:$20)]
    sta.fp __vr15
    lda $28  ; [loadVreg v37(I8,ZP:$28)]
    ldx #0  ; [zext I8→I16]
    ldx #0
    stax.fp __vr19
    lda $2A  ; [loadVreg v38(I8,ZP:$2A)]
    ldx #0  ; [zext I8→I16]
    ldx #0
    stax.fp __vr23
    lda $22  ; [loadVreg v34(I16/PTR,ZP:$22)]
    ldx $23  ; [loadVreg v34(I16/PTR,ZP:$22)]
    sta.fp __vr28
@while_body8:
    .loc "palette_fade.c", 48
    lda #3
    sta $D020  ; [STORE → $D020]
    .loc "palette_fade.c", 49
    lda #0  ; [CONST → v3(I8,frame) val=0]
    sta.fp __vr3
    lda.fp __vr3  ; [loadVreg v3(I8,frame)]
    sta.fp __vr2
@for_cond10_ph:
    lda #255  ; [CONST → v29(I8,frame) val=255]
    sta.fp __vr29
    .loc "palette_fade.c", 53
    lda #16  ; [CONST → v30(I8,frame) val=16]
    sta.fp __vr30
@for_cond10:
    .loc "palette_fade.c", 49
    lda #255  ; [CONST → v4(I8,frame) val=255]
    sta.fp __vr4
    lda.fp __vr2  ; [loadVreg v2(I8,frame)]
    ldx #0
    stax.fp __vr5
    ldax.fp __vr6  ; [loadVreg v6(I16/PTR,frame)]
    sta __zp_scratch2
    stx __zp_scratch2+1
    ldax.fp __vr5  ; [loadVreg v5(I16/PTR,frame)]
    cmp.16 .AX, __zp_scratch2
    bcc @for_body11
    bra @for_end13
@for_body11:
    .loc "palette_fade.c", 50
    lda.fp __vr2  ; [loadVreg v2(I8,frame)]
    jsr _apply_fade
    .loc "palette_fade.c", 51
    jsr _delay_frame
    .loc "palette_fade.c", 53
    lda #16  ; [CONST → v8(I8,frame) val=16]
    sta.fp __vr8
    lda.fp __vr2  ; [loadVreg v2(I8,frame)]
    ldx #0
    stax.fp __vr9
    ldax.fp __vr10  ; [loadVreg v10(I16/PTR,frame)]
    sta __zp_scratch2
    stx __zp_scratch2+1
    ldax.fp __vr9  ; [loadVreg v9(I16/PTR,frame)]
    and.16 .AX, __zp_scratch2
    stax.fp __vr11
    ldax.fp __vr11  ; [loadVreg v11(I16/PTR,frame)]
    bne @if_then14
    cmp #$00
    bne @if_then14
    bra @if_else15
@if_then14:
    .loc "palette_fade.c", 54
    lda #3
    sta $D020  ; [STORE → $D020]
    bra @if_end16
@if_else15:
    .loc "palette_fade.c", 56
    lda #14
    sta $D020  ; [STORE → $D020]
@if_end16:
@for_inc12:
    .loc "palette_fade.c", 49
    lda.fp __vr2  ; [loadVreg v2(I8,frame)]
    sta.fp __vr12
    lda.fp __vr12  ; [loadVregA v12(I8,frame)]
    inc a
    sta.fp __vr13
    lda.fp __vr13  ; [loadVreg v13(I8,frame)]
    sta.fp __vr2
    bra @for_cond10
@for_end13:
    .loc "palette_fade.c", 59
    lda #255  ; [CONST → v14(I16/PTR,frame) val=255]
    ldx #0  ; [CONST → v14(I16/PTR,frame) val=255]
    stax.fp __vr14
    lda.fp __vr15  ; [loadVreg v15(I8,frame)]
    jsr _apply_fade
    .loc "palette_fade.c", 60
    jsr _delay_frame
    .loc "palette_fade.c", 61
    lda #3
    sta $D020  ; [STORE → $D020]
    .loc "palette_fade.c", 64
    lda #4
    sta $D020  ; [STORE → $D020]
    .loc "palette_fade.c", 65
    lda #255  ; [CONST → v16(I8,frame) val=255]
    sta.fp __vr16
    lda.fp __vr16  ; [loadVreg v16(I8,frame)]
    sta.fp __vr2
@for_cond17_ph:
    lda #0  ; [CONST → v31(I8,frame) val=0]
    sta.fp __vr31
    .loc "palette_fade.c", 69
    lda #16  ; [CONST → v32(I8,frame) val=16]
    sta.fp __vr32
@for_cond17:
    .loc "palette_fade.c", 65
    lda #0  ; [CONST → v17(I8,frame) val=0]
    sta.fp __vr17
    lda.fp __vr2  ; [loadVreg v2(I8,frame)]
    ldx #0
    stax.fp __vr18
    ldax.fp __vr19  ; [loadVreg v19(I16/PTR,frame)]
    sta __zp_scratch2
    stx __zp_scratch2+1
    ldax.fp __vr18  ; [loadVreg v18(I16/PTR,frame)]
    cmp.16 .AX, __zp_scratch2
    beq @for_end20
    bcs @for_body18
    bra @for_end20
@for_body18:
    .loc "palette_fade.c", 66
    lda.fp __vr2  ; [loadVreg v2(I8,frame)]
    jsr _apply_fade
    .loc "palette_fade.c", 67
    jsr _delay_frame
    .loc "palette_fade.c", 69
    lda #16  ; [CONST → v21(I8,frame) val=16]
    sta.fp __vr21
    lda.fp __vr2  ; [loadVreg v2(I8,frame)]
    ldx #0
    stax.fp __vr22
    ldax.fp __vr23  ; [loadVreg v23(I16/PTR,frame)]
    sta __zp_scratch2
    stx __zp_scratch2+1
    ldax.fp __vr22  ; [loadVreg v22(I16/PTR,frame)]
    and.16 .AX, __zp_scratch2
    stax.fp __vr24
    ldax.fp __vr24  ; [loadVreg v24(I16/PTR,frame)]
    bne @if_then21
    cmp #$00
    bne @if_then21
    bra @if_else22
@if_then21:
    .loc "palette_fade.c", 70
    lda #4
    sta $D020  ; [STORE → $D020]
    bra @if_end23
@if_else22:
    .loc "palette_fade.c", 72
    lda #14
    sta $D020  ; [STORE → $D020]
@if_end23:
@for_inc19:
    .loc "palette_fade.c", 65
    lda.fp __vr2  ; [loadVreg v2(I8,frame)]
    sta.fp __vr25
    lda.fp __vr25  ; [loadVregA v25(I8,frame)]
    dec a
    sta.fp __vr26
    lda.fp __vr26  ; [loadVreg v26(I8,frame)]
    sta.fp __vr2
    bra @for_cond17
@for_end20:
    .loc "palette_fade.c", 75
    lda #0  ; [CONST → v27(I16/PTR,frame) val=0]
    tax  ; [CONST → v27(I16/PTR,frame) val=0]
    stax.fp __vr27
    lda.fp __vr28  ; [loadVreg v28(I8,frame)]
    jsr _apply_fade
    .loc "palette_fade.c", 76
    jsr _delay_frame
    .loc "palette_fade.c", 77
    lda #4
    sta $D020  ; [STORE → $D020]
    bra @while_body8
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
    .func_flags zp_call
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc


__zp_save_buf:

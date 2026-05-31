    .o45
    .extern __sp_base
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
    .var @_l_i = $20
    .var @_l_j = $22

@entry:
    .loc "palette_fade.c", 19
    lda #0  ; [CONST → v2(I16/PTR,ZP:$24) val=0 → direct store]
    sta $20
    sta $21
@for_cond0:
    lda $20  ; [loadVreg v0(I16/PTR,ZP:$20)]
    ldx $21  ; [loadVreg v0(I16/PTR,ZP:$20)]
    cmp.16 .AX, #10
    bcc @for_body1
    bra @for_end3
@for_body1:
    .loc "palette_fade.c", 20
    lda #0  ; [CONST → v5(I16/PTR,ZP:$24) val=0 → direct store]
    sta $22
    sta $23
@for_cond4:
    lda $22  ; [loadVreg v1(I16/PTR,ZP:$22)]
    ldx $23  ; [loadVreg v1(I16/PTR,ZP:$22)]
    cmp.16 .AX, #3000
    bcc @for_body5
    bra @for_end7
@for_body5:
@for_inc6:
    inc $22  ; [ADD → v8(I16/PTR,ZP:$24) inc16 ZP]
    bne *+4
    inc $23
    bra @for_cond4
@for_end7:
@for_inc2:
    .loc "palette_fade.c", 19
    inc $20  ; [ADD → v9(I16/PTR,ZP:$24) inc16 ZP]
    bne *+4
    inc $21
    bra @for_cond0
@for_end3:
@__return:
    .func_flags zp_call, leaf, reentrant
    .reg_clobbers A, X, Y
    .flag_clobbers C, N, Z, V
    endproc

; function _main
    proc _main
    .var _fp = 0
    .loc "palette_fade.c", 24
; frame: 6 bytes (frame-allocated vRegs only)
    phw #0
    phw #0
    phw #0
    .local __vr2 = 0
    .local __vr10 = 2
    .local __vr18 = 4
    .var @_l_border_color = $22
    .local @_l_level = 0

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
@while_body8:
    .loc "palette_fade.c", 48
    lda #3
    sta $D020  ; [STORE → $D020]
    .loc "palette_fade.c", 49
    lda #0  ; [CONST → v3(I8,ZP:$20) val=0]
    sta $20  ; [storeVreg v3(I8,ZP:$20)]
    sta.fp __vr2
@for_cond10:
    lda.fp __vr2  ; [loadVreg v2(I8,frame)]
    cmp #255
    bcc @for_body11
    bra @for_end13
@for_body11:
    .loc "palette_fade.c", 50
    lda.fp __vr2  ; [loadVreg v2(I8,frame)]
    jsr _apply_fade
    .loc "palette_fade.c", 51
    jsr _delay_frame
    .loc "palette_fade.c", 53
    lda #16  ; [CONST → v6(I8,ZP:$20) val=16]
    sta $20  ; [storeVreg v6(I8,ZP:$20)]
    lda.fp __vr2  ; [loadVregA v2(I8,frame)]
    and $20  ; [AND → v7(I8,ZP:$24)]
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
    lda.fp __vr2  ; [loadVregA v2(I8,frame)]
    inc a
    sta $20  ; [storeVreg v8(I8,ZP:$20)]
    sta.fp __vr2
    bra @for_cond10
@for_end13:
    .loc "palette_fade.c", 59
    lda #255  ; [CONST → v9(I16/PTR,ZP:$20) val=255]
    ldx #0  ; [CONST → v9(I16/PTR,ZP:$20) val=255]
    sta $20
    stx $21
    lda $20  ; [loadVreg v9(I16/PTR,ZP:$20)]
    ldx $21  ; [loadVreg v9(I16/PTR,ZP:$20)]
    sta.fp __vr10
    lda.fp __vr10  ; [loadVreg v10(I8,frame)]
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
    lda #255  ; [CONST → v11(I8,ZP:$20) val=255]
    sta $20  ; [storeVreg v11(I8,ZP:$20)]
    sta.fp __vr2
@for_cond17:
    lda.fp __vr2  ; [loadVreg v2(I8,frame)]
    cmp #0
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
    lda #16  ; [CONST → v14(I8,ZP:$20) val=16]
    sta $20  ; [storeVreg v14(I8,ZP:$20)]
    lda.fp __vr2  ; [loadVregA v2(I8,frame)]
    and $20  ; [AND → v15(I8,ZP:$24)]
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
    lda.fp __vr2  ; [loadVregA v2(I8,frame)]
    dec a
    sta $20  ; [storeVreg v16(I8,ZP:$20)]
    sta.fp __vr2
    bra @for_cond17
@for_end20:
    .loc "palette_fade.c", 75
    lda #0  ; [CONST → v17(I16/PTR,ZP:$20) val=0]
    sta $20
    sta $21
    lda $20  ; [loadVreg v17(I16/PTR,ZP:$20)]
    ldx $21  ; [loadVreg v17(I16/PTR,ZP:$20)]
    sta.fp __vr18
    lda.fp __vr18  ; [loadVreg v18(I8,frame)]
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
    .func_flags zp_call
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc


__zp_save_buf:

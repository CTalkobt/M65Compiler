    .o45
    .extern __sp_base
    __zp_scratch = $08
    __zp_scratch2 = $0A
    __zp_scratch3 = $0C
    __zp_scratch4 = $0E

    .extern _puts
    .extern _sprintf

    .global _miles_to_km
    .global _km_to_miles
    .global _main

    .segment "code"

; function _miles_to_km
    proc _miles_to_km, D#@_p_miles
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdio.h", 5
    .var @_p_miles = 2

    ldaxyz.fp @_p_miles
    sta $20
    stx $21
    sty $22
    stz $23
@entry:
    .loc "convert.c", 18
    lda #73
    ldx #6
    sta $24
    stx $25
    lda $24
    ldx $25
    ldy #0
    ldz #0
    sta $26
    stx $27
    sty $28
    stz $29
    lda $20
    ldx $21
    ldy $22
    ldz $23
    mul.32 .AXYZ, $26
    sta $2A
    stx $2B
    sty $2C
    stz $2D
    lda #232
    ldx #3
    sta $24
    stx $25
    lda $24
    ldx $25
    ldy #0
    ldz #0
    sta $26
    stx $27
    sty $28
    stz $29
    lda $2A
    ldx $2B
    ldy $2C
    ldz $2D
    div.32 .AXYZ, $26
    sta $2E
    stx $2F
    sty $30
    stz $31
    lda $2E
    ldx $2F
    ldy $30
    ldz $31
@__return:
    .func_flags stack_call, leaf, reentrant
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc

; function _km_to_miles
    proc _km_to_miles, D#@_p_km
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdio.h", 10
    .var @_p_km = 2

    ldaxyz.fp @_p_km
    sta $20
    stx $21
    sty $22
    stz $23
@entry:
    .loc "convert.c", 23
    lda #109
    ldx #2
    sta $24
    stx $25
    lda $24
    ldx $25
    ldy #0
    ldz #0
    sta $26
    stx $27
    sty $28
    stz $29
    lda $20
    ldx $21
    ldy $22
    ldz $23
    mul.32 .AXYZ, $26
    sta $2A
    stx $2B
    sty $2C
    stz $2D
    lda #232
    ldx #3
    sta $24
    stx $25
    lda $24
    ldx $25
    ldy #0
    ldz #0
    sta $26
    stx $27
    sty $28
    stz $29
    lda $2A
    ldx $2B
    ldy $2C
    ldz $2D
    div.32 .AXYZ, $26
    sta $2E
    stx $2F
    sty $30
    stz $31
    lda $2E
    ldx $2F
    ldy $30
    ldz $31
@__return:
    .func_flags stack_call, leaf, reentrant
    .reg_clobbers A, X, Y, Z
    .flag_clobbers C, N, Z, V
    endproc

; function _main
    proc _main
    .var _fp = 0
    .loc "/home/duck/m65/inpg/m65compiler/bin/../lib/include/stdio.h", 14
; frame: 84 bytes (frame-allocated vRegs only)
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
    .local __vr3 = 42
    .local __vr4 = 46
    .local __vr8 = 50
    .local __vr10 = 52
    .local __vr11 = 54
    .local __vr13 = 56
    .local __vr15 = 58
    .local __vr20 = 60
    .local __vr21 = 62
    .local __vr23 = 64
    .local __vr27 = 66
    .local __vr28 = 68
    .local __vr30 = 70
    .local __vr35 = 72
    .local __vr36 = 74
    .local __vr38 = 76
    .local __vr42 = 78
    .local __vr43 = 80
    .local __vr45 = 82
    .local @_l_buf = 2
    .local @_l_result = 46
    .local @_l_val = 42

@entry:
    .loc "convert.c", 27
    ldax #__str_0
    stax.fp __vr0
    ldax.fp __vr0
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _puts
    .var _fp = _fp - 2
    .loc "convert.c", 35
    lda #26
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    ldy #0
    ldz #0
    sta $22
    stx $23
    sty $24
    stz $25
    lda $22
    ldx $23
    ldy $24
    ldz $25
    staxyz.fp __vr3
    .loc "convert.c", 36
    ldaxyz.fp __vr3
    sta $28
    stx $29
    sty $2A
    stz $2B
    lda $28
    ldx $29
    ldy $2A
    ldz $2B
    push .axyz
    .var _fp = _fp + 4
    jsr _miles_to_km
    .var _fp = _fp - 4
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    staxyz.fp __vr4
    .loc "convert.c", 37
    ldax #__str_1
    stax.fp __vr8
    ldax.fp __vr8
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _puts
    .var _fp = _fp - 2
    .loc "convert.c", 38
    ldax #__str_2
    stax.fp __vr10
    lda.fp __vr2
    ldx #0
    stax.fp __vr11
    ldaxyz.fp __vr4
    sta $28
    stx $29
    sty $2A
    stz $2B
    ldaxyz.fp __vr3
    sta $2C
    stx $2D
    sty $2E
    stz $2F
    ldax.fp __vr10
    sta $30
    stx $31
    ldax.fp __vr11
    sta $34
    stx $35
    lda $28
    ldx $29
    ldy $2A
    ldz $2B
    push .axyz
    .var _fp = _fp + 4
    lda $2C
    ldx $2D
    ldy $2E
    ldz $2F
    push .axyz
    .var _fp = _fp + 4
    lda $30
    ldx $31
    push .ax
    .var _fp = _fp + 2
    lda $34
    ldx $35
    push .ax
    .var _fp = _fp + 2
    jsr _sprintf
    .var _fp = _fp - 12
    .loc "convert.c", 39
    ldax #__str_3
    stax.fp __vr13
    ldax.fp __vr13
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _puts
    .var _fp = _fp - 2
    lda.fp __vr2
    ldx #0
    stax.fp __vr15
    .loc "convert.c", 40
    ldax.fp __vr15
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _puts
    .var _fp = _fp - 2
    .loc "convert.c", 42
    lda #100
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    ldy #0
    ldz #0
    sta $22
    stx $23
    sty $24
    stz $25
    lda $22
    ldx $23
    ldy $24
    ldz $25
    staxyz.fp __vr3
    .loc "convert.c", 43
    ldaxyz.fp __vr3
    sta $28
    stx $29
    sty $2A
    stz $2B
    lda $28
    ldx $29
    ldy $2A
    ldz $2B
    push .axyz
    .var _fp = _fp + 4
    jsr _miles_to_km
    .var _fp = _fp - 4
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    staxyz.fp __vr4
    .loc "convert.c", 44
    ldax #__str_4
    stax.fp __vr20
    lda.fp __vr2
    ldx #0
    stax.fp __vr21
    ldaxyz.fp __vr4
    sta $28
    stx $29
    sty $2A
    stz $2B
    ldaxyz.fp __vr3
    sta $2C
    stx $2D
    sty $2E
    stz $2F
    ldax.fp __vr20
    sta $30
    stx $31
    ldax.fp __vr21
    sta $34
    stx $35
    lda $28
    ldx $29
    ldy $2A
    ldz $2B
    push .axyz
    .var _fp = _fp + 4
    lda $2C
    ldx $2D
    ldy $2E
    ldz $2F
    push .axyz
    .var _fp = _fp + 4
    lda $30
    ldx $31
    push .ax
    .var _fp = _fp + 2
    lda $34
    ldx $35
    push .ax
    .var _fp = _fp + 2
    jsr _sprintf
    .var _fp = _fp - 12
    lda.fp __vr2
    ldx #0
    stax.fp __vr23
    .loc "convert.c", 45
    ldax.fp __vr23
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _puts
    .var _fp = _fp - 2
    .loc "convert.c", 47
    lda #64
    ldx #17
    ldy #139
    ldz #5
    sta $20
    stx $21
    sty $22
    stz $23
    staxyz.fp __vr3
    .loc "convert.c", 48
    ldaxyz.fp __vr3
    sta $28
    stx $29
    sty $2A
    stz $2B
    lda $28
    ldx $29
    ldy $2A
    ldz $2B
    push .axyz
    .var _fp = _fp + 4
    jsr _miles_to_km
    .var _fp = _fp - 4
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    staxyz.fp __vr4
    .loc "convert.c", 49
    ldax #__str_5
    stax.fp __vr27
    lda.fp __vr2
    ldx #0
    stax.fp __vr28
    ldaxyz.fp __vr4
    sta $28
    stx $29
    sty $2A
    stz $2B
    ldaxyz.fp __vr3
    sta $2C
    stx $2D
    sty $2E
    stz $2F
    ldax.fp __vr27
    sta $30
    stx $31
    ldax.fp __vr28
    sta $34
    stx $35
    lda $28
    ldx $29
    ldy $2A
    ldz $2B
    push .axyz
    .var _fp = _fp + 4
    lda $2C
    ldx $2D
    ldy $2E
    ldz $2F
    push .axyz
    .var _fp = _fp + 4
    lda $30
    ldx $31
    push .ax
    .var _fp = _fp + 2
    lda $34
    ldx $35
    push .ax
    .var _fp = _fp + 2
    jsr _sprintf
    .var _fp = _fp - 12
    lda.fp __vr2
    ldx #0
    stax.fp __vr30
    .loc "convert.c", 50
    ldax.fp __vr30
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _puts
    .var _fp = _fp - 2
    .loc "convert.c", 54
    lda #42
    ldx #0
    sta $20
    stx $21
    lda $20
    ldx $21
    ldy #0
    ldz #0
    sta $22
    stx $23
    sty $24
    stz $25
    lda $22
    ldx $23
    ldy $24
    ldz $25
    staxyz.fp __vr3
    .loc "convert.c", 55
    ldaxyz.fp __vr3
    sta $28
    stx $29
    sty $2A
    stz $2B
    lda $28
    ldx $29
    ldy $2A
    ldz $2B
    push .axyz
    .var _fp = _fp + 4
    jsr _km_to_miles
    .var _fp = _fp - 4
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    staxyz.fp __vr4
    .loc "convert.c", 56
    ldax #__str_6
    stax.fp __vr35
    lda.fp __vr2
    ldx #0
    stax.fp __vr36
    ldaxyz.fp __vr4
    sta $28
    stx $29
    sty $2A
    stz $2B
    ldaxyz.fp __vr3
    sta $2C
    stx $2D
    sty $2E
    stz $2F
    ldax.fp __vr35
    sta $30
    stx $31
    ldax.fp __vr36
    sta $34
    stx $35
    lda $28
    ldx $29
    ldy $2A
    ldz $2B
    push .axyz
    .var _fp = _fp + 4
    lda $2C
    ldx $2D
    ldy $2E
    ldz $2F
    push .axyz
    .var _fp = _fp + 4
    lda $30
    ldx $31
    push .ax
    .var _fp = _fp + 2
    lda $34
    ldx $35
    push .ax
    .var _fp = _fp + 2
    jsr _sprintf
    .var _fp = _fp - 12
    lda.fp __vr2
    ldx #0
    stax.fp __vr38
    .loc "convert.c", 57
    ldax.fp __vr38
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _puts
    .var _fp = _fp - 2
    .loc "convert.c", 59
    lda #144
    ldx #221
    ldy #5
    ldz #0
    sta $20
    stx $21
    sty $22
    stz $23
    staxyz.fp __vr3
    .loc "convert.c", 60
    ldaxyz.fp __vr3
    sta $28
    stx $29
    sty $2A
    stz $2B
    lda $28
    ldx $29
    ldy $2A
    ldz $2B
    push .axyz
    .var _fp = _fp + 4
    jsr _km_to_miles
    .var _fp = _fp - 4
    sta $20
    stx $21
    sty $22
    stz $23
    lda $20
    ldx $21
    ldy $22
    ldz $23
    staxyz.fp __vr4
    .loc "convert.c", 61
    ldax #__str_7
    stax.fp __vr42
    lda.fp __vr2
    ldx #0
    stax.fp __vr43
    ldaxyz.fp __vr4
    sta $28
    stx $29
    sty $2A
    stz $2B
    ldaxyz.fp __vr3
    sta $2C
    stx $2D
    sty $2E
    stz $2F
    ldax.fp __vr42
    sta $30
    stx $31
    ldax.fp __vr43
    sta $34
    stx $35
    lda $28
    ldx $29
    ldy $2A
    ldz $2B
    push .axyz
    .var _fp = _fp + 4
    lda $2C
    ldx $2D
    ldy $2E
    ldz $2F
    push .axyz
    .var _fp = _fp + 4
    lda $30
    ldx $31
    push .ax
    .var _fp = _fp + 2
    lda $34
    ldx $35
    push .ax
    .var _fp = _fp + 2
    jsr _sprintf
    .var _fp = _fp - 12
    lda.fp __vr2
    ldx #0
    stax.fp __vr45
    .loc "convert.c", 62
    ldax.fp __vr45
    sta $28
    stx $29
    lda $28
    ldx $29
    push .ax
    .var _fp = _fp + 2
    jsr _puts
    .var _fp = _fp - 2
    .loc "convert.c", 64
    lda #0
    ldx #0
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


    .segment "data"
__str_0:
    .text "unit converter starting"
    .byte 0
__str_1:
    .text "before sprintf"
    .byte 0
__str_2:
    .text "%ld miles = %ld km"
    .byte 0
__str_3:
    .text "after sprintf"
    .byte 0
__str_4:
    .text "%ld miles = %ld km"
    .byte 0
__str_5:
    .text "%ld miles = %ld km"
    .byte 0
__str_6:
    .text "%ld km = %ld miles"
    .byte 0
__str_7:
    .text "%ld km = %ld miles"
    .byte 0

__zp_save_buf:
